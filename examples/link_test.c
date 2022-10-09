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
 * @file link_example.c
 * @brief Example application to setup link object and monitor its health.
 * This application shows a recommended procedure for:
 *        - Setting up Aries Retimer data structures to store status and
 *          diagnostics information on a per-Link basis.
 *        - Continuous monitoring of key status parameters (e.g. Link state,
 *          junction temperature, eye height/width monitors, etc.).
 *        - Error-scenario handling for cases where continuous monitoring has
 *          determined the state is unexpected and additional debug information
 *          needs to be gathered.
 */

#include "../include/aries_api.h"
#include "../include/aries_link.h"
#include "include/aspeed.h"

#include <unistd.h>
#include <stdio.h>

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

    AriesDeviceType* ariesDevice2;
    AriesI2CDriverType* i2cDriver2;
    int ariesHandle2;
    int i2cBus2 = 2;

    ariesHandle2 = asteraI2COpenConnection(i2cBus2, ariesSlaveAddress);

    i2cDriver2 = (AriesI2CDriverType*) malloc(sizeof(AriesI2CDriverType));
    i2cDriver2->handle = ariesHandle2;
    i2cDriver2->slaveAddr = ariesSlaveAddress;
    i2cDriver2->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver2->i2cFormat = ARIES_I2C_FORMAT_ASTERA;

    i2cDriver2->lockInit = 0;

    ariesDevice2 = (AriesDeviceType*) malloc(sizeof(AriesDeviceType));
    ariesDevice2->i2cDriver = i2cDriver2;
    ariesDevice2->i2cBus = i2cBus2;
    ariesDevice2->partNumber = ARIES_PTX08;

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

    // Read an Aries register (Global Param Reg 0)
    // This is an indication if I2C access is working as expected
    // Return error if read fails
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

    // Print FW version
    uint8_t fwVersionMajor = ariesDevice->fwVersion.major;
    uint8_t fwVersionMinor = ariesDevice->fwVersion.minor;
    uint16_t fwVersionBld = ariesDevice->fwVersion.build;
    ASTERA_INFO("FW Version: %d.%d.%d", fwVersionMajor, fwVersionMinor,
        fwVersionBld);

    // Set temperature thresholds
    ariesDevice->tempAlertThreshC = 110.0; // Trigger alert when Temp > 110C
    ariesDevice->tempWarnThreshC = 100.0; // Trigger warn when Temp > 100C
    ariesDevice->minLinkFoMAlert = 0x55; // Trigger warn when link FoM < 0x55

    // Set DPLL frequency thresholds
    ariesDevice->minDPLLFreqAlert = 2*1024;
    ariesDevice->maxDPLLFreqAlert = 14*1024;

    rc = ariesInitDevice(ariesDevice2, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    // Read an Aries register (Global Param Reg 0)
    // This is an indication if I2C access is working as expected
    // Return error if read fails
    rc = ariesReadBlockData(i2cDriver2, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to read gbl_param_reg0");
        return rc;
    }
    int glb_param_reg0_2 = (dataBytes[3]<<24) + (dataBytes[2] <<16)
        + (dataBytes[1]<<8) + dataBytes[0];
    ASTERA_INFO("glb_param_reg0 = 0x%08x", glb_param_reg0_2);

    // Print FW version
    uint8_t fwVersionMajor2 = ariesDevice2->fwVersion.major;
    uint8_t fwVersionMinor2 = ariesDevice2->fwVersion.minor;
    uint16_t fwVersionBld2 = ariesDevice2->fwVersion.build;
    ASTERA_INFO("FW Version: %d.%d.%d", fwVersionMajor2, fwVersionMinor2,
        fwVersionBld2);

    // Set temperature thresholds
    ariesDevice2->tempAlertThreshC = 110.0; // Trigger alert when Temp > 110C
    ariesDevice2->tempWarnThreshC = 100.0; // Trigger warn when Temp > 100C
    ariesDevice2->minLinkFoMAlert = 0x55; // Trigger warn when link FoM < 0x55

    // Set DPLL frequency thresholds
    ariesDevice2->minDPLLFreqAlert = 2*1024;
    ariesDevice2->maxDPLLFreqAlert = 14*1024;


    // Configure Link structs
    // NOTE: In this case, we have one Link structure for one Link; however,
    // a system may have N Links supported by M Retimers (N>M), and therefore
    // multiple link objects/structs may be used to keep track of the Links'
    // status.
    AriesLinkType link[2];
    link[0].device = ariesDevice;
    link[0].config.linkId = 0;
    link[0].config.partNumber = ariesDevice->partNumber;
    link[0].config.maxWidth = 16;
    link[0].config.startLane = 0;

    link[1].device = ariesDevice2;
    link[1].config.linkId = 0;
    link[1].config.partNumber = ariesDevice2->partNumber;
    link[1].config.maxWidth = 8;
    link[1].config.startLane = 0;
    // -------------------------------------------------------------------------
    // CONTINUOUS MONITORING
    // -------------------------------------------------------------------------
    // This portion of the example shows how Aries Links can be monitored
    // periodically during regular system health checking.
    int run = 1;
    int recoveryCount = 0;
    int recoveryClearCount = 0;
    // Dump Link Logs if user passes in argument with value 1
    bool dumpLogs = false;
    if (argc >= 2)
    {
        if (argv[1][0] == '1')
        {
            dumpLogs = true;
        }
    }
    while (run)
    {
        // Get current link state for link[0]
        rc = ariesCheckLinkHealth(&link[0]);
        CHECK_SUCCESS(rc);

        // Do the same for link[1]
        rc = ariesCheckLinkHealth(&link[1]);
        CHECK_SUCCESS(rc);

        // ---------------------------------------------------------------------
        // ERROR-SCENARIO HANDLING
        // ---------------------------------------------------------------------
        // Check if Link is in an unexpected state, and if not, capture
        // additional debug information.
        // The expected Retimer state during PCIe L0 is the "Forwarding" state.
        if (link[0].state.linkOkay == false || link[1].state.linkOkay == false || dumpLogs == true)
        {
            ASTERA_ERROR("Unexpected link0 state: %d", link[0].state.state);
            ASTERA_ERROR("Unexpected link1 state: %d", link[1].state.state);
            ASTERA_ERROR("Now capturing link stats and logs");

            // Capture detailed debug information
            // This function prints detailed state information to a file
            rc = ariesLinkPrintDetailedState(&link[0], ".", "x16_link_state_detailed");
            CHECK_SUCCESS(rc);
            // This function prints Aries LTSSM logs to a file
            rc = ariesLinkPrintMicroLogs(&link[0], ".", "x16_ltssm_micro_log");
            CHECK_SUCCESS(rc);

            // Capture detailed debug information
            // This function prints detailed state information to a file
            rc = ariesLinkPrintDetailedState(&link[1], ".", "x8_link_state_detailed");
            CHECK_SUCCESS(rc);
            // This function prints Aries LTSSM logs to a file
            rc = ariesLinkPrintMicroLogs(&link[1], ".", "x8_ltssm_micro_log");
            CHECK_SUCCESS(rc);

            // In this example, the continuous monitoring section exits when the
            // Link state is unexpected.
            break;
        }

        // Read and report the recovery count
        rc = ariesGetLinkRecoveryCount(&link[0], &recoveryCount);
        CHECK_SUCCESS(rc);
        ASTERA_INFO("Link0 Recovery counter value: %d", recoveryCount);
        if (recoveryClearCount >= 6)
        {
            recoveryClearCount = 0;
            ASTERA_INFO("Clearing Link0 recovery counter");
            rc = ariesClearLinkRecoveryCount(&link[0]);
            CHECK_SUCCESS(rc);
        }
        recoveryClearCount++;

        // Read and report the recovery count for the other link
        rc = ariesGetLinkRecoveryCount(&link[1], &recoveryCount);
        CHECK_SUCCESS(rc);
        ASTERA_INFO("Link1 Recovery counter value: %d", recoveryCount);
        if (recoveryClearCount >= 6)
        {
            recoveryClearCount = 0;
            ASTERA_INFO("Clearing Link1 recovery counter");
            rc = ariesClearLinkRecoveryCount(&link[1]);
            CHECK_SUCCESS(rc);
        }
        recoveryClearCount++;

        // Wait 5 seconds before reading state again (this up to the user)
        sleep(5);
    }

    // Close all open connections
    closeI2CConnection(ariesHandle);
    closeI2CConnection(ariesHandle2);

    return ARIES_SUCCESS;
}
