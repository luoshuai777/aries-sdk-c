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
 * @file eeprom_direct.c
 * @brief Example application to write and verify contents of the EEPROM directly
 */

#include "../include/aries_api.h"
#include "include/aspeed.h"
#include "include/eeprom.h"
#include "include/misc.h"

#include <unistd.h>

int main(int argc, char* argv[])
{
    AriesErrorType rc;
    int i2cBus = 1;

    // List out EEPROM slave addresses
    int numEEPROMSlaves = 4;
    uint8_t eepromAddr[4] = {0x50, 0x51, 0x52, 0x53};
    int eepromHandles[4];
    int eepromIdx;
    int switchHandle;
    int ariesHandle;
    int switchAddr = 0x70;
    int ariesAddr = 0x20;

    AriesI2CDriverType* i2cDriver;

    // Operation to connect to DUT
    // This sequence sets the I2C switch to connect to the DUT (slave 0x70)
    // Not required if BMC directly connected to DUT
    switchHandle = asteraI2COpenConnection(i2cBus, switchAddr);
    rc = svbSwitchRead(switchHandle, 1);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to connect to DUT");
    }

    // Set up connection to Aries
    ariesHandle = asteraI2COpenConnection(i2cBus, ariesAddr);
    i2cDriver = (AriesI2CDriverType*) malloc(sizeof(AriesI2CDriverType));
    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesAddr;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
    // Flag to indicate lock has not been initialized. Call ariesInitDevice()
    // later to initialize.
    i2cDriver->lockInit = 0;

    // Set bit 8 in HW reset (I2C Master reset)
    uint8_t dataBytes[2];
    dataBytes[0] = 0x0;
    dataBytes[1] = 0x2;
    rc = ariesWriteBlockData(i2cDriver, ARIES_HW_RST_ADDR, 2, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("HW reset operation failed");
    }
    ASTERA_INFO("Disabled I2C Master on Aries device");

    // Close i2c connection to Aries
    closeI2CConnection(ariesHandle);

    // Operation to connect to EEPROM
    rc = svbSwitchRead(switchHandle, 2);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to connect to EEPROM");
    }


    for (eepromIdx = 0; eepromIdx < numEEPROMSlaves; eepromIdx++)
    {
        eepromHandles[eepromIdx] = asteraI2COpenConnection(i2cBus,
            eepromAddr[eepromIdx]);
    }

    // Load FW .ihx file
    int numBytes = 262144;  // Num. bytes to write
    uint8_t image[numBytes];
    if (argc == 2)
    {
        rc = ariesLoadIhxFile(argv[1], image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to load the .ihx file. RC = %d", rc);
        }
    }
    else
    {
        ASTERA_ERROR("Missing arg. Must pass FW .ihx file as argument");
        return -1;
    }

    // Write EEPROM contents
    rc = writeEEPROM(eepromAddr, eepromHandles, numEEPROMSlaves, image);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to write EEPROM image");
    }

    // Verify EEPROM contents
    rc =verifyEEPROM(eepromAddr, eepromHandles, numEEPROMSlaves, image);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failure in verifying EEPROM image");
    }

    // Close EEPROM I2C connections
    for (eepromIdx = 0; eepromIdx < numEEPROMSlaves; eepromIdx++)
    {
        closeI2CConnection(eepromHandles[eepromIdx]);
    }

    return 0;

}
