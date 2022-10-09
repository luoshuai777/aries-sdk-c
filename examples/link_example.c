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

#define NUM_RETIMERS 2
#define NUM_LINKS_PER_RETIMER 2
#define NUM_TOTAL_LINKS NUM_RETIMERS * NUM_LINKS_PER_RETIMER

int main(void)
{
    // -------------------------------------------------------------------------
    // SETUP
    // -------------------------------------------------------------------------
    // This portion of the example shows how to set up data structures for
    // accessing and configuring Aries Retimers
    AriesDeviceType* ariesDevice[NUM_RETIMERS];
    AriesI2CDriverType* i2cDriver[NUM_RETIMERS];
    AriesErrorType rc;
    int ariesHandle[NUM_RETIMERS];
    int i2cBus[NUM_RETIMERS] = {1, 2};
    int ariesSlaveAddress[NUM_RETIMERS] = {0x20, 0x20};

    // Enable SDK-level debug prints
    asteraLogSetLevel(1); // ASTERA_INFO type statements (or higher)

    int i = 0;
    for (i = 0; i < NUM_RETIMERS; ++i)
    {
        // Open connection to Aries Retimer
        ariesHandle[i] = asteraI2COpenConnection(i2cBus[i], ariesSlaveAddress[i]);

        // Initialize I2C Driver for SDK transactions
        i2cDriver[i] = (AriesI2CDriverType*) malloc(sizeof(AriesI2CDriverType));
        i2cDriver[i]->handle = ariesHandle[i];
        i2cDriver[i]->slaveAddr = ariesSlaveAddress[i];
        i2cDriver[i]->pecEnable = ARIES_I2C_PEC_DISABLE;
        i2cDriver[i]->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
        // Flag to indicate lock has not been initialized. Call ariesInitDevice()
        // later to initialize.
        i2cDriver[i]->lockInit = 0;

        // Initialize Aries device structure
        ariesDevice[i] = (AriesDeviceType*) malloc(sizeof(AriesDeviceType));
        ariesDevice[i]->i2cDriver = i2cDriver[i];
        ariesDevice[i]->i2cBus = i2cBus[i];
        ariesDevice[i]->partNumber = ARIES_PTX16;
    }

    // -------------------------------------------------------------------------
    // INITIALIZATION
    // -------------------------------------------------------------------------
    // Check Connection and Init device
    // If the connection is not good, the ariesInitDevice() API will enable ARP
    // and update the i2cDriver with the new address. It also checks for the
    // Main Micro heartbeat before reading the FW version. In case the heartbeat
    // is not up, it sets the firmware version to 0.0.0.
    for (i = 0; i < NUM_RETIMERS; ++i)
    {
        rc = ariesInitDevice(ariesDevice[i], ariesSlaveAddress[i]);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Init device failed");
            return rc;
        }
    }

    // NOTE: The following parameters are configured and checked by the SDK and
    // do not change the behavior of Firmware running on the Retimer
    for (i = 0; i < NUM_RETIMERS; ++i)
    {
        // Set temperature thresholds
        ariesDevice[i]->tempAlertThreshC = 110.0; // Trigger alert when Temp > 110C
        ariesDevice[i]->tempWarnThreshC = 100.0; // Trigger warn when Temp > 100C
        ariesDevice[i]->minLinkFoMAlert = 0x55; // Trigger warn when link FoM < 0x55
        // Set DPLL frequency thresholds
        ariesDevice[i]->minDPLLFreqAlert = 2*1024;
        ariesDevice[i]->maxDPLLFreqAlert = 14*1024;
    }

    // NOTE: In this case, we have two Retimers and each has two Links; since
    // a system may have N Links supported by M Retimers (N>M), therefore
    // multiple link objects/structs may be used to keep track of the Links'
    // status.
    // Configure Link structs
    AriesLinkType link[NUM_TOTAL_LINKS];
    // Retimer 0 Link 0
    link[0].device = ariesDevice[0];
    link[0].config.linkId = 0;
    link[0].config.partNumber = ariesDevice[0]->partNumber;
    link[0].config.maxWidth = 8;
    link[0].config.startLane = 0;
    // Retimer 0 Link 1
    link[1].device = ariesDevice[0];
    link[1].config.linkId = 1;
    link[1].config.partNumber = ariesDevice[0]->partNumber;
    link[1].config.maxWidth = 8;
    link[1].config.startLane = 8;
    // Retimer 1 Link 0
    link[2].device = ariesDevice[1];
    link[2].config.linkId = 0;
    link[2].config.partNumber = ariesDevice[1]->partNumber;
    link[2].config.maxWidth = 8;
    link[2].config.startLane = 0;
    // Retimer 1 Link 1
    link[3].device = ariesDevice[1];
    link[3].config.linkId = 1;
    link[3].config.partNumber = ariesDevice[1]->partNumber;
    link[3].config.maxWidth = 8;
    link[3].config.startLane = 8;

    // -------------------------------------------------------------------------
    // CONTINUOUS MONITORING
    // -------------------------------------------------------------------------
    // This portion of the example shows how Aries Links can be monitored
    // periodically during regular system health checking.
    int recoveryCount = 0;
    while (1)
    {
        for (i = 0; i < NUM_TOTAL_LINKS; ++i)
        {
            // Get current link state for link[i]
            rc = ariesCheckLinkHealth(&link[i]);
            CHECK_SUCCESS(rc);

            // Read and report the recovery count
            rc = ariesGetLinkRecoveryCount(&link[i], &recoveryCount);
            CHECK_SUCCESS(rc);

            if (recoveryCount > 0)
            {
                ASTERA_WARN("The link%d went into recovery %d times!", i, recoveryCount);
                rc = ariesClearLinkRecoveryCount(&link[i]);
                CHECK_SUCCESS(rc);
            }

            // -----------------------------------------------------------------
            // ERROR-SCENARIO HANDLING
            // -----------------------------------------------------------------
            // Check if the Link is in on expected state, and if not, capture
            // additional debug information.
            // The expected Retimer state during PCIe L0 is the "Forwarding" or
            // "FWD" state.
            if (link[i].state.linkOkay == false)
            {
                ASTERA_ERROR("Unexpected link%d state: %d", i, link[i].state.state);
                ASTERA_ERROR("Now capturing link stats and logs");

                // Capture detailed debug information from Retimer
                rc = ariesLinkDumpDebugInfo(&link[i]);
                CHECK_SUCCESS(rc);

                // In this example, the continuous monitoring section exits when
                // the Link state is unexpected.
                break;
            }
        }

        // Wait 5 seconds before reading state again (this up to the user)
        sleep(5);
    }

    for (i = 0; i < NUM_RETIMERS; ++i)
    {
        // Close all open connections
        closeI2CConnection(ariesHandle[i]);
    }

    return ARIES_SUCCESS;
}
