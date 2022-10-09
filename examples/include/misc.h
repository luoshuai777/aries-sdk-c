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
 * @file misc.h
 * @brief Implementation of helper functions used by the examples
 */

#ifndef ASTERA_ARIES_EX_MISC_H_
#define ASTERA_ARIES_EX_MISC_H_

#include <stdint.h>
#include <stdio.h>

#include "../../include/astera_log.h"
#include "../../include/aries_i2c.h"
#include "../../include/aries_api_types.h"
#include "../../include/aries_error.h"
#include "aspeed.h"

// Way to connect driver to either Aries or EEPROM
AriesErrorType svbSwitchRead(
        int handle,
        int reg);

#endif // ASTERA_ARIES_EX_MISC_H_
