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
 * @file rpi.c
 * @brief Implementation of helper functions used by RPi 3
 */

#include "../include/rpi.h"

int asteraI2CWriteBlockData(
        int handle,
        uint8_t cmdCode,
        uint8_t bufLength,
        uint8_t* buffer)
{
    return i2cWriteI2CBlockData(handle, cmdCode, buffer, bufLength);
}

int asteraI2CReadBlockData(
        int handle,
        uint8_t cmdCode,
        uint8_t bufLength,
        uint8_t* buffer)
{
    return i2cReadI2CBlockData(handle, cmdCode, buffer, bufLength);
}

int asteraI2COpenConnection(
        int bus,
        int slaveAddress)
{
    if (gpioInitialise() < 0)
    {
        printf("xxx ERROR :: Failed to open GPIO connection xxx");
        return -1;
    }
    int flags = 0;
    int handle = i2cOpen(bus, slaveAddress, flags);
    if (handle < 0)
    {
        printf("xxx ERROR :: Failed to open I2C handle xxx");
        return -1;
    }
    return handle;
}

void closeI2CConnection(
        int handle)
{
    i2cClose(handle);
}
