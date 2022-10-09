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
 * @file eeprom.h
 * @brief Example application to write EEPROM image through Aries
 */

#include <unistd.h>
#include "../../include/aries_api.h"

// Size of data block being written to EEPROM
#define EEPROM_WRITE_DATA_BLOCK_SIZE 31
// Size of data block being read from EEPROM
#define EEPROM_READ_DATA_BLOCK_SIZE 32
// Size of a page in EEPROM
#define EEPROM_PAGE_SIZE 256
// Num pages per slave in EEPROM
#define EEPROM_SLAVE_NUM_PAGES 256
// Size of slave(in bytes) in EEPROM
#define EEPROM_SLAVE_SIZE_BYTES 0x10000

// EEPROM block write delay
#define EEPROM_BLOCK_WRITE_DELAY_USEC 10000

// API to write EEPROM image via Aries
// Enable ARP and retry if write fails
AriesErrorType writeEEPROMImage(
        AriesDeviceType* device,
        uint8_t* image);

AriesErrorType writeEEPROMImageNoArp(
        AriesDeviceType* device,
        uint8_t* image,
        bool legacyMode,
        int freqHz);

// API to write to EEPROM directly
AriesErrorType writeEEPROM(
        uint8_t* EEPROMSlaveAddrs,
        int* EEPROMSlaveHandles,
        int numEEPROMSlaves,
        uint8_t* image);

// API to read from EEPROM directly
AriesErrorType verifyEEPROM(
        uint8_t* EEPROMSlaveAddrs,
        int* EEPROMSlaveHandles,
        int numEEPROMSlaves,
        uint8_t* image);
