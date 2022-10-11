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
 * @file eeprom_test.c
 * @brief Example application to write EEPROM image through Aries.
 * This application does the following:
 *      - Write EEPROM image through Aries Retimer
 *          - If write fails with negative error code, run ARP (Address
 *            Resolution Protocol) and try write again
 *          - If write fails again, return error code
 *          - If write succeeds, continue to next step
 *      - Verify contents of EEPROM with expected image
 */

#include "../include/aries_api.h"
#include "include/aspeed.h"

#include <unistd.h>

int main(int argc, char* argv[])
{
    // -------------------------------------------------------------------------
    // SETUP
    // -------------------------------------------------------------------------
    // This portion of the example shows how to set up data structures for
    // accessing and configuring Aries Retimers
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = 6;
    int ariesSlaveAddress = 0x20;

    // Enable SDK-level debug prints
    asteraLogSetLevel(1); // ASTERA_INFO type statements (or higher)

    // Open connection to Aries Retimer
    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);

    // Initialize I2C Driver for SDK transactions
    i2cDriver = (AriesI2CDriverType*) malloc(sizeof(AriesI2CDriverType));
    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
    // Flag to indicate lock has not been initialized. Call ariesInitDevice()
    // later to initialize.
    i2cDriver->lockInit = 0;

    // Initialize Aries device structure
    ariesDevice = (AriesDeviceType*) malloc(sizeof(AriesDeviceType));
    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = ARIES_PTX16;

    // -------------------------------------------------------------------------
    // INITIALIZATION
    // -------------------------------------------------------------------------
    // Check Connection and Init device
    // If the connection is not good, the ariesInitDevice() API will enable ARP
    // and update the i2cDriver with the new address. It also checks for the
    // Main Micro heartbeat before reading the FW version. In case the heartbeat
    // is not up, it sets the firmware version to 0.0.0.
    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    // Set I2C Master frequency to 400 kHz
    /*ASTERA_INFO("Updating I2C Master frequency to 400 kHz");*/
    /*rc = ariesI2CMasterSetFrequency(i2cDriver, 370000);*/
    /*CHECK_SUCCESS(rc);*/

    // Load FW file (passed in as an argument to this function)
    // Convert .ihx file to byte array
    // EEPROM size is 2Mbits = 262144 bytes
    uint8_t image[ARIES_EEPROM_NUM_BYTES];
    if (argc >= 2)
    {
        // Load ihx file
        rc = ariesLoadIhxFile(argv[1], image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to load the .ihx file. RC = %d", rc);
        }
    }
    else
    {
        ASTERA_ERROR("Missing arg. Must pass FW .ihx file as first argument");
        return ARIES_INVALID_ARGUMENT;
    }

    // -------------------------------------------------------------------------
    // PROGRAMMING
    // -------------------------------------------------------------------------
    // legacyMode is the slow programming mode
    // This is set if you have an old FW version (< 1.0.48), or have ARP
    // enabled, which means you are trying to recover from a corrupted image
    // Optionally, it can be manually enabled by sending a 1 as the 2nd argument
    bool legacyMode = false;
    if (ariesDevice->arpEnable)
    {
        legacyMode = true;
    }
    if (argc >= 3)
    {
        if (argv[2][0] == '1')
        {
            legacyMode = true;
        }
    }

    // Program EEPROM image
    rc = ariesWriteEEPROMImage(ariesDevice, image, legacyMode);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to program the EEPROM. RC = %d", rc);
    }

    // Corrupt a byte
    // Optionally, if the user sends in a 1 or 2 as the 3rd argument we will
    // corrupt the image with either a single or multi byte corruption
    if (argc >= 4)
    {
        if (argv[3][0] == '1')
        {
            uint8_t corruptByte[1];
            corruptByte[0] = 0xC0;
            rc = ariesWriteEEPROMByte(ariesDevice, 37, corruptByte);
            CHECK_SUCCESS(rc);
        }
        else if (argv[3][0] == '2')
        {
            uint8_t corruptByte[1];
            corruptByte[0] = 0xC0;
            rc = ariesWriteEEPROMByte(ariesDevice, 37, corruptByte);
            CHECK_SUCCESS(rc);
            corruptByte[0] = 0xFF;
            rc = ariesWriteEEPROMByte(ariesDevice, 38, corruptByte);
            CHECK_SUCCESS(rc);
            corruptByte[0] = 0xEE;
            rc = ariesWriteEEPROMByte(ariesDevice, 39, corruptByte);
            CHECK_SUCCESS(rc);
            corruptByte[0] = 0xDE;
            rc = ariesWriteEEPROMByte(ariesDevice, 0x20001, corruptByte);
            CHECK_SUCCESS(rc);
            corruptByte[0] = 0xAD;
            rc = ariesWriteEEPROMByte(ariesDevice, 0x20002, corruptByte);
            CHECK_SUCCESS(rc);
        }
    }

    bool checksumVerifyFailed = false;
    if (legacyMode == false)
    {
        // Verify EEPROM programming by reading EEPROM and computing a checksum
        rc = ariesVerifyEEPROMImageViaChecksum(ariesDevice, image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to verify the EEPROM using checksum. RC = %d", rc);
            checksumVerifyFailed = true;
        }
    }

    // If the EEPROM verify via checksum failed, attempt the byte by byte verify
    // Optionally, it can be manually enabled by sending a 1 as the 4th argument
    if (legacyMode || checksumVerifyFailed || argc >= 5)
    {
        if (legacyMode || checksumVerifyFailed || argv[4][0] == '1')
        {
            // Verify EEPROM programming by reading EEPROM and comparing data with
            // expected image. In case there is a failure, the API will attempt a
            // rewrite once
            rc = ariesVerifyEEPROMImage(ariesDevice, image, legacyMode);
            if (rc != ARIES_SUCCESS)
            {
                ASTERA_ERROR("Failed to read and verify the EEPROM. RC = %d", rc);
            }
        }
    }

    // Reboot device to check if FW version was applied
    // Assert HW reset
    ASTERA_INFO("Performing Retimer HW reset ...");
    rc = ariesSetHwReset(ariesDevice, 1);
    // Wait 10 ms before de-asserting
    usleep(10000);
    // De-assert HW reset
    rc = ariesSetHwReset(ariesDevice, 0);

    // It takes 1.8 sec for Retimer to reload firmware. Hence set
    // wait to 2 secs before reading the FW version again
    usleep(3000000);

    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    // Print new FW version
    ASTERA_INFO("Updated FW Version is %d.%d.%d", ariesDevice->fwVersion.major,
        ariesDevice->fwVersion.minor, ariesDevice->fwVersion.build);

    // Close all open connections
    closeI2CConnection(ariesHandle);

    return ARIES_SUCCESS;
}
