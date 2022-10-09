/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/*
 * @file eeprom.c
 * @brief Example application to write EEPROM image through Aries
 */

#include "../include/eeprom.h"
#include "../include/aspeed.h"
#include "../include/misc.h"

/*
 * Write and verify firmware image to EEPROM via Aries
 */
AriesErrorType writeEEPROMImage(
        AriesDeviceType* device,
        uint8_t* image)
{
    AriesErrorType rc;
    int arpHandle;
    int ariesHandle;
    int arpAddr = 0x61;
    int ariesArpSlave = 0x55;
    bool legacyMode = false;

    // Write FW image to Aries
    // If we get a negative error code, it could mean that the current FW
    // on Aries is bad (corrupted or invalid), and we need to update the FW
    // To do this, we need to enable ARP
    rc = ariesWriteEEPROMImage(device, image, legacyMode);
    if (rc != ARIES_SUCCESS)
    {
        // Connect to ARP
        ASTERA_WARN("EEPROM write attempt failed. Enabling ARP and re-trying");
        arpHandle = asteraI2COpenConnection(device->i2cBus, arpAddr);
        rc = ariesRunArp(arpHandle, ariesArpSlave);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Enabling ARP unsuccessful");
            return -1;
        }

        // Set up new connection since slave address has changed
        ariesHandle = asteraI2COpenConnection(device->i2cBus, ariesArpSlave);
        device->i2cDriver->handle = ariesHandle;
        device->i2cDriver->slaveAddr = ariesArpSlave;

        // Set legacy mode
        // This means you write entire EEPROM image in legacy mode (slow mode)
        legacyMode = true;

        // Write EEPROM image
        rc = ariesWriteEEPROMImage(device, image, legacyMode);
        CHECK_SUCCESS(rc);
        ASTERA_INFO("EEPROM write succeeded");
    }

    // Verify image that was programmed
    rc = ariesVerifyEEPROMImage(device, image, legacyMode);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("EEPROM verification succeeded");

    return ARIES_SUCCESS;
}


/*
 * Write an EEPROM image via Aries without doing verification or ARP checking
 */
AriesErrorType writeEEPROMImageNoArp(
        AriesDeviceType* device,
        uint8_t* image,
        bool legacyMode,
        int freqHz)
{
    AriesErrorType rc;

    // Adjust the Aries I2C Master interface frequency
    rc = ariesI2CMasterSetFrequency(device->i2cDriver, freqHz);
    CHECK_SUCCESS(rc);

    // Write FW image to Aries
    rc = ariesWriteEEPROMImage(device, image, legacyMode);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("EEPROM write attempt failed.");
        return -1;
    }

    return ARIES_SUCCESS;
}


/*
 * API to write data to EEPROM directly
 */
AriesErrorType writeEEPROM(
        uint8_t* EEPROMSlaveAddrs,
        int* EEPROMSlaveHandles,
        int numEEPROMSlaves,
        uint8_t* image)
{
    AriesErrorType rc;

    int slaveIdx;
    int pageIdx;
    int blockIdx;
    int dataBlockIdx;
    uint8_t dataBytes[(EEPROM_WRITE_DATA_BLOCK_SIZE+1)];

    int addr = 0;

    uint8_t addrHigh;
    uint8_t addrLow;

    // Get number of blocks to write per page
    // Cannot cross page boundary in block transaction, and hence have to
    // write a page at a time
    int blockCount = EEPROM_PAGE_SIZE/EEPROM_WRITE_DATA_BLOCK_SIZE;
    int blockRemBytes = EEPROM_PAGE_SIZE % EEPROM_WRITE_DATA_BLOCK_SIZE;
    if (blockRemBytes > 0)
    {
        blockCount += 1;
    }

    uint8_t dataBytesRem[(blockRemBytes+1)];

    ASTERA_INFO("Starting EEPROM write");

    // Fill up EEPROM, one slave, one page at a time
    for (slaveIdx = 0; slaveIdx < numEEPROMSlaves; slaveIdx++)
    {
        addr = 0;
        ASTERA_INFO("    Writing EEPROM: 0x%02x", EEPROMSlaveAddrs[slaveIdx]);
        for (pageIdx = 0; pageIdx < EEPROM_SLAVE_NUM_PAGES; pageIdx++)
        {
            for (blockIdx = 0; blockIdx < blockCount; blockIdx++)
            {
                addr = (pageIdx*EEPROM_PAGE_SIZE) +
                    (blockIdx*EEPROM_WRITE_DATA_BLOCK_SIZE);
                addrHigh = (addr>>8) & 0xff;
                addrLow = addr & 0xff;

                // Check if last iteration of page
                // If so, we write fewer bytes
                if ((blockRemBytes > 0) && (blockIdx == (blockCount-1)))
                {
                    dataBytesRem[0] = addrLow;
                    dataBlockIdx = 0;
                    while (dataBlockIdx < blockRemBytes)
                    {
                        dataBytesRem[(dataBlockIdx+1)] =
                            image[((slaveIdx*EEPROM_SLAVE_SIZE_BYTES) + addr +
                            dataBlockIdx)];
                        dataBlockIdx++;
                    }
                    rc = asteraI2CWriteBlockData(EEPROMSlaveHandles[slaveIdx],
                        addrHigh, (blockRemBytes+1), dataBytesRem);
                    CHECK_SUCCESS(rc);
                }
                else
                {
                    dataBytes[0] = addrLow;
                    dataBlockIdx = 0;
                    while (dataBlockIdx < EEPROM_WRITE_DATA_BLOCK_SIZE)
                    {
                        dataBytes[(dataBlockIdx+1)] = image[(
                            (slaveIdx*EEPROM_SLAVE_SIZE_BYTES) + addr +
                            dataBlockIdx)];
                        dataBlockIdx++;
                    }
                    rc = asteraI2CWriteBlockData(EEPROMSlaveHandles[slaveIdx],
                        addrHigh, (EEPROM_WRITE_DATA_BLOCK_SIZE+1), dataBytes);
                    CHECK_SUCCESS(rc);
                }
                // EEPROM block write delay (10ms accroding to spec)
                usleep(EEPROM_BLOCK_WRITE_DELAY_USEC);
            }
        }
    }

    ASTERA_INFO("Done write");

    return ARIES_SUCCESS;
}


/*
 * API to read data from EEPROM directly, and verify against expected image
 */
AriesErrorType verifyEEPROM(
        uint8_t* EEPROMSlaveAddrs,
        int* EEPROMSlaveHandles,
        int numEEPROMSlaves,
        uint8_t* image)
{
    AriesErrorType rc = 0;

    int slaveIdx = 0;
    int dataIdx;
    int addr = 0;

    // 2 byte address
    uint8_t address[2];
    // 32 byte data from EEPROM
    uint8_t dataBytes[EEPROM_READ_DATA_BLOCK_SIZE];

    uint8_t addrHigh;
    uint8_t addrLow;
    int actAddr;
    uint8_t receivedByte;
    uint8_t expectedByte;

    // Use i2c message to construct a read
    struct i2c_msg readMsg[2];
    struct i2c_rdwr_ioctl_data i2cReadData;

    ASTERA_INFO("Starting EEPROM Verify");

    for (slaveIdx = 0; slaveIdx < numEEPROMSlaves; slaveIdx++)
    {
        ASTERA_INFO("    Reading EEPROM: 0x%02x", EEPROMSlaveAddrs[slaveIdx]);
        addr = 0;
        while (addr < EEPROM_SLAVE_SIZE_BYTES)
        {
            addrHigh = (addr >> 8) & 0xff;
            addrLow = addr & 0xff;

            // Write 2 byte address
            address[0] = addrHigh;
            address[1] = addrLow;
            readMsg[0].addr = EEPROMSlaveAddrs[slaveIdx];
            readMsg[0].flags = 0;
            readMsg[0].len = 2;
            readMsg[0].buf = &address;

            // Set up data buffer for write back
            readMsg[1].addr = EEPROMSlaveAddrs[slaveIdx];
            readMsg[1].flags = I2C_M_RD;
            readMsg[1].len = EEPROM_READ_DATA_BLOCK_SIZE;
            readMsg[1].buf = &dataBytes;

            // Set up read message
            i2cReadData.msgs = readMsg;
            i2cReadData.nmsgs = 2;

            rc = ioctl(EEPROMSlaveHandles[slaveIdx], I2C_RDWR, &i2cReadData);
            if (rc < 0)
            {
                return rc;
            }

            for (dataIdx = 0; dataIdx < EEPROM_READ_DATA_BLOCK_SIZE; dataIdx++)
            {
                actAddr = (slaveIdx*EEPROM_SLAVE_SIZE_BYTES) + addr + dataIdx;
                expectedByte = image[actAddr];
                receivedByte = dataBytes[dataIdx];

                if (expectedByte != receivedByte)
                {
                    ASTERA_ERROR("(Addr: %d) Expected: %d, Received: %d",
                        actAddr, expectedByte, receivedByte);
                    rc = ARIES_EEPROM_VERIFY_FAILURE;
                }
            }
            addr+=EEPROM_READ_DATA_BLOCK_SIZE;
        }
    }
    ASTERA_INFO("Done with verifying EEPROM contents");
    return ARIES_SUCCESS;
}
