/*
 * Copyright 2022 Astera Labs, Inc. or its affiliates. All Rights Reserved.
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
 * @file prbs.c
 * @brief Example application showing how to enable PRBS generators and
 * checkers on two Retimer devices for the purpose of doing bit error rate
 * testing.
 */

#include "../include/aries_api.h"
#include "include/aspeed.h"

#include <unistd.h>

#define NUM_RETIMERS 2

int main(void)
{
    // Test parameters
    int rate = 4; // Gen4
    int preset = 8;
    int dwell_time_sec = 5;
    AriesPRBSPatternType pattern_tx = LFSR23;
    AriesPRBSPatternType pattern_rx = LFSR23;

    AriesDeviceType* ariesDevice[NUM_RETIMERS];
    AriesI2CDriverType* i2cDriver[NUM_RETIMERS];
    AriesErrorType rc;

    int i2cBus[NUM_RETIMERS] = {1, 2};
    int ariesSlaveAddress = 0x20; // assume both Aries slave devices are at addr 0x20
    int ariesHandle[NUM_RETIMERS];

    // Connect to retimer
    int i;
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        ariesHandle[i] = asteraI2COpenConnection(i2cBus[i], ariesSlaveAddress);
        i2cDriver[i] = (AriesI2CDriverType*) malloc(sizeof(AriesI2CDriverType));
        i2cDriver[i]->handle = ariesHandle[i];
        i2cDriver[i]->slaveAddr = ariesSlaveAddress;
        i2cDriver[i]->pecEnable = ARIES_I2C_PEC_DISABLE;
        i2cDriver[i]->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
        // Flag to indicate lock has not been initialized. Call ariesInitDevice()
        // later to initialize.
        i2cDriver[i]->lockInit = 0;
    }

    for (i = 0; i < NUM_RETIMERS; i++)
    {
        ariesDevice[i] = (AriesDeviceType*) malloc(sizeof(AriesDeviceType));
        ariesDevice[i]->i2cDriver = i2cDriver[i];
        ariesDevice[i]->i2cBus = i2cBus[i];
        ariesDevice[i]->partNumber = ARIES_PTX16;
    }

    for (i = 0; i < NUM_RETIMERS; i++)
    {
        ASTERA_INFO("Retimer #%d", i+1);
        rc = ariesInitDevice(ariesDevice[i], ariesSlaveAddress);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Init device failed");
            return rc;
        }
        // Print SDK version
        ASTERA_INFO("SDK Version: %s", ariesGetSDKVersion());
        // Print FW version
        ASTERA_INFO("FW Version: %d.%d.%d", ariesDevice[i]->fwVersion.major,
            ariesDevice[i]->fwVersion.minor, ariesDevice[i]->fwVersion.build);
    }

    // Enable test mode on all Retimers
    ASTERA_INFO("Enable test mode on all Retimers...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Enable test mode
        ariesTestModeEnable(ariesDevice[i]);
        usleep(100000); // 100 ms
    }

    // Set data rate on all Retimers
    ASTERA_INFO("Set data rate on all Retimers...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Change data rate
        ASTERA_INFO("Setting data rate to Gen %d", rate);
        ariesTestModeRateChange(ariesDevice[i], rate);
        usleep(100000); // 100 ms
    }

    // Enable PRBS pattern generators on all Retimers
    ASTERA_INFO("Enable PRBS pattern generators on all Retimers...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Configure transmitters
        ariesTestModeTxConfig(ariesDevice[i], pattern_tx, preset, true);
        usleep(100000); // 100 ms
    }

    // Enable and adapt receivers, detect and correct polarity inversion, and read FoM on all Retimers
    ASTERA_INFO("Enable and adapt receivers, detect and correct polarity inversion, and read FoM on all Retimers...");
    int side, ln;
    int fom[NUM_RETIMERS][32]; // 32 lanes of FoM data for each Retimer instance
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Enable receivers
        ariesTestModeRxConfig(ariesDevice[i], pattern_rx, true);
        usleep(100000); // 100 ms
        // Capture FoM data (valid for Gen3/4/5
        if (rate >= 3)
        {
            ariesTestModeRxFomRead(ariesDevice[i], &(fom[i][0]));
            for (side = 0; side < 2; side++)
            {
                for (ln = 0; ln < 16; ln++)
                {
                    ASTERA_INFO("  Retimer %d, Side: %d, Lane: %02d, FoM = %03d", i, side, ln, fom[i][side*16 + ln]);
                }
            }
        }
    }

    // Clear error counters on all Retimers
    ASTERA_INFO("Clear error counters on all Retimers...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Clear error counters
        ariesTestModeRxEcountClear(ariesDevice[i]);
    }

    // BER gate
    ASTERA_INFO("Wait for %d seconds before checking error counters...", dwell_time_sec);
    int t;
    for (t = 0; t < (dwell_time_sec*10); t++)
    {
        usleep(100000); // 100 ms
    }

    // Read error counts on all Retimers
    ASTERA_INFO("Read error counters on all Retimers...");
    int ecount[NUM_RETIMERS][32]; // 32 lanes of ECOUNT data for each Retimer instance
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Capture ECOUNT data
        ariesTestModeRxEcountRead(ariesDevice[i], &(ecount[i][0]));
        for (side = 0; side < 2; side++)
        {
            for (ln = 0; ln < 16; ln++)
            {
                ASTERA_INFO("  Retimer %d, Side: %d, Lane: %02d, ECOUNT = %d", i, side, ln, ecount[i][side*16 + ln]);
            }
        }
    }

    // Inject one error on all transmitters
    ASTERA_INFO("Inject one error on all transmitters...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Inject single bit error
        ariesTestModeTxErrorInject(ariesDevice[i]);
    }

    // Read error counts again to confirm error was registered
    ASTERA_INFO("Read error counters again to confirm error was registered...");
    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Capture ECOUNT data
        ariesTestModeRxEcountRead(ariesDevice[i], &(ecount[i][0]));
        for (side = 0; side < 2; side++)
        {
            for (ln = 0; ln < 16; ln++)
            {
                ASTERA_INFO("  Retimer %d, Side: %d, Lane: %02d, ECOUNT (after error injection) = %d", i, side, ln, ecount[i][side*16 + ln]);
            }
        }
    }

    for (i = 0; i < NUM_RETIMERS; i++)
    {
        // Close all open connections
        closeI2CConnection(ariesHandle[i]);
    }

    return ARIES_SUCCESS;
}
