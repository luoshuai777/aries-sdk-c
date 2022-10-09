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
 * @file astera_test.c
 * @brief Example application targeting basic testing of the Aries SDK.
 * This is recommended for:
 *        - Setting up Aries device and driver data structures
 *        - Reading general purpose registers
 *        - Getting the FW version
 */

#include "../include/aries_api.h"
#include "include/aspeed.h"

#include <unistd.h>

int main(void)
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
    int i2cBus = 1;
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

    // Print SDK version
    ASTERA_INFO("SDK Version: %s", ariesGetSDKVersion());

    // Print FW version
    ASTERA_INFO("FW Version: %d.%d.%d", ariesDevice->fwVersion.major,
        ariesDevice->fwVersion.minor, ariesDevice->fwVersion.build);

    uint8_t dataBytes[4];
    rc = ariesReadBlockData(i2cDriver, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to read gbl_param_reg0");
        return rc;
    }
    int glb_param_reg0 = (dataBytes[3]<<24) + (dataBytes[2] <<16)
        + (dataBytes[1]<<8) + dataBytes[0];
    ASTERA_INFO("glb_param_reg0 = 0x%08x", glb_param_reg0);

    /*Read Temperature*/
    rc = ariesGetCurrentTemp(ariesDevice);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("Current Temp: %.2f C", ariesDevice->currentTempC);

    rc = ariesGetMaxTemp(ariesDevice);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("Max Temp Seen: %.2f C", ariesDevice->maxTempC);

    ASTERA_INFO("Starting Bus Lock Test");
    ASTERA_INFO("Read a register");
    rc = ariesReadBlockData(i2cDriver, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("  Failed to read gbl_param_reg0");
        return rc;
    }
    glb_param_reg0 = (dataBytes[3]<<24) + (dataBytes[2] <<16)
        + (dataBytes[1]<<8) + dataBytes[0];
    ASTERA_INFO("  glb_param_reg0 = 0x%08x", glb_param_reg0);

    ASTERA_INFO("Read the bus lock status");
    int busLockSts = 0xff;
    rc = ariesGetI2CBusClearEventStatus(ariesDevice, &busLockSts);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("  This should be 0: %d", busLockSts);

    ASTERA_INFO("Force an error");
    rc = ariesReadBlockDataForceError(i2cDriver, 0, 4, dataBytes);
    CHECK_SUCCESS(rc);

    ASTERA_INFO("Read the bus lock status");
    rc = ariesGetI2CBusClearEventStatus(ariesDevice, &busLockSts);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("  This should be 1: %d", busLockSts);

    ASTERA_INFO("Ensure that we can still read a register properly");
    rc = ariesReadBlockData(i2cDriver, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("  Failed to read gbl_param_reg0");
        return rc;
    }
    glb_param_reg0 = (dataBytes[3]<<24) + (dataBytes[2] <<16)
        + (dataBytes[1]<<8) + dataBytes[0];
    ASTERA_INFO("  glb_param_reg0 = 0x%08x", glb_param_reg0);

    ASTERA_INFO("Clear the bus lock status");
    rc = ariesClearI2CBusClearEventStatus(ariesDevice);
    CHECK_SUCCESS(rc);

    ASTERA_INFO("Read the bus lock status");
    rc = ariesGetI2CBusClearEventStatus(ariesDevice, &busLockSts);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("  This should be 0: %d", busLockSts);

    ASTERA_INFO("Perform an invalid I2C operation");
    dataBytes[0] = 0;
    rc = asteraI2CWriteBlockData(i2cDriver->handle, 0x0, 1, dataBytes);
    CHECK_SUCCESS(rc);

    ASTERA_INFO("Read the bus lock status");
    rc = ariesGetI2CBusClearEventStatus(ariesDevice, &busLockSts);
    CHECK_SUCCESS(rc);
    ASTERA_INFO("  This should be 1: %d", busLockSts);

    ASTERA_INFO("Ensure that we can still read a register properly");
    rc = ariesReadBlockData(i2cDriver, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("  Failed to read gbl_param_reg0");
        return rc;
    }
    glb_param_reg0 = (dataBytes[3]<<24) + (dataBytes[2] <<16)
        + (dataBytes[1]<<8) + dataBytes[0];
    ASTERA_INFO("  glb_param_reg0 = 0x%08x", glb_param_reg0);


    closeI2CConnection(ariesHandle);
    return 0;
}
