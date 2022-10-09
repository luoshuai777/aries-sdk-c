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
 * @file rpi.h
 * @brief Implementation of helper functions used by RPi 3+
 */

#ifndef ASTERA_ARIES_SDK_RPI_H_
#define ASTERA_ARIES_SDK_RPI_H_

#include <stdio.h>
#include <stdint.h>

#include "pigpio.h"   // pigpio RPi library

void closeI2CConnection(
        int handle);

#endif    // ASTERA_ARIES_SDK_RPI_H_
