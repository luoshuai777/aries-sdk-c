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
 * @file aries_link.h
 * @brief Definition of Link level functions for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_LINK_H_
#define ASTERA_ARIES_SDK_LINK_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_api.h"
#include "aries_misc.h"

#ifdef ARIES_MPW
#include "aries_mpw_reg_defines.h"
#else
#include "aries_a0_reg_defines.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dump all the debug info from the retimer
AriesErrorType ariesLinkDumpDebugInfo(
        AriesLinkType* link);

// Function to iterate over logger and print entry
AriesErrorType ariesLinkPrintMicroLogs(
        AriesLinkType* link,
        const char* basepath,
        const char* filename);

// Capture the detailed link state and print it to file
AriesErrorType ariesLinkPrintDetailedState(
        AriesLinkType* link,
        const char* basepath,
        const char* filename);

// Print the micro logger entries
AriesErrorType ariesPrintLog(
        AriesLinkType* link,
        AriesLTSSMLoggerEnumType log,
        FILE* fp);

// Calculate the current width of the link
AriesErrorType ariesLinkGetCurrentWidth(
        AriesLinkType* link,
        int* currentWidth);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_LINK_H_ */
