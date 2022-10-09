# aries-sdk-c

## Introduction
This repository contains a C-language Software Developers' Kit (C-SDK) for the [Aries PCIe Retimer](https://www.asteralabs.com/products/smart-retimers/). The documentation for this C-SDK can be found in html/index.html. This is the home page for the repository. To view the list of files, click on the "Files" tab and "File List" under it. This lists the .c and .h files in the C-SDK. Clicking on any of the files shows the list of functions, defines, structs, and enums with a brief description.

## Integration
To integrate the Aries C-SDK into a larger Baseboard Management Controller (BMC) software, refer to guidance in the following sections.

### Build Considerations
The Aries C-SDK source code files (.c) are in the **source** directory, and the include files (.h) are in the **include** directory. The example applications in the **examples** directory make use of the APIs defined in **source** and **include**. The Makefile at the root of this repository shows how these example applications can be built to include the application code and C-SDK code.

### Integrating into a System BMC
The Aries C-SDK is designed to be integrated into a larger BMC application and operates in three phases, as detailed below.

#### Setup Phase
The setup phase includes creating connections from the BMC to the Retimer and instantiating device and Link data structures. The connection between the BMC and Retimer can be established by calling native SMBus access APIs within the BMC library. The device data structure is used to store device-level parameters such as firmware version, temperature warning thresholds, Link margin warning thresholds, etc. The Link data structure is used to store Link-level configuration and statistics such as adaptation parameters, eye quality figures of merit (FoMs), lane-to-lane deskew results, precoding requests and status, etc.

#### Continuous Monitoring Phase
During the normal operation of a system (e.g., a server), the user may wish to continuously monitor the health of each PCIe Link via the BMC. This is useful for the purposes of diagnosing an issue when Link problems occur, but also to help anticipate or even prevent Link issues before they occur by closely monitoring the health and margin of the Link. One simple way to continuously monitor the Link is by repeatedly calling **ariesCheckLinkHealth()**. This API checks for temperature, eye parameters, expected state, and receiver FoM values for a particular Link and flags cases when one of the parameters is outside the user-defined limits. If such an alert is raised, the application can then gather additional detailed Link statistics as part of the error-handling phase of Link monitoring.

#### Error-Scenario Handling Phase
In the event an error scenario is encountered during the continuous monitoring phase, this portion of the software is designed to handle those scenarios. Critical information is saved to a file for post-processing. The C-SDK includes APIs to capture a trace of the Link state history and detailed Link statistics (stats); and easy-to-use Python scripts are provided to visualize this data in a human-readable format. A single **ariesLinkDumpDebugInfo()** function is provided to record all debug information from the Retimer. This includes the Link status table consisting of the per-Lane electrical parameters, hardware state, and firmware state; as well as per Link LTSSM history logs.

### Fleet Management Example
An example application demonstrating the code and function calls implementing the three steps above is included in the C-SDK in **examples/link_example.c**. The application shows how to configure and monitor multiple Retimer each with multiple Links. Refer to this file for a Fleet Management Example that runs on the ASPEED AST2500 BMC.

### Implementation Dependent I2C Functions
The C-SDK provides function prototypes for the low level I2C transactions in the **aries_i2c.h** header file. All of the other SDK functions call these low level I2C functions to communicate with the Retimer. Since each platform's I2C access methods are unique these low level I2C functions (**asteraI2COpenConnection()**, **asteraI2CWriteBlockData()**, **asteraI2CReadBlockData()**, **asteraI2CBlock()**, and **asteraI2CUnblock()**) must be implemented in the User's application. We provide examples of how to implement these functions for an ASPEED, or RaspberryPi platforms. Please find these demonstration **aspeed.c/.h** and **rpi.c/.h** files in the **examples/source/** and **examples/include/** folders.

## Post Processing Scripts for Link Statistics and Logging

We provide python scripts to help display the stats and Link logs in a better and more readable format.

Requirements: Python 3.0+

### Script to Print Link Stats

**print_link_state.py** is a post processing script which could be used to present the Link stats in a tabular form. The SDK generates detailed log files, named **link_state_detailed_log_0.py** (where 0 is the Link number) by default, which contains a Python dictionary containing the all the statistics for a given Link. This script helps present the data in a more readable format.

To run this post processing script, place the SDK generated log files in the **link_state_logs** folder and run the python script. The output is printed to stdout and to a log file. The **--folder** parameter is optional. Here is an example to run the command:

```bash
python ./print_link_state --folder link_state_logs
```

where **link_state_logs** is the directory containing the C SDK generated log files **link_state_detailed_log_0.py**.

We have included the **link_state_logs** directory in the SDK for reference.

### Script to Print Link Micro Logs

**print_link_logs.py** is a post processing script to print the Main Micro and Path Micro logs generated by the C-SDK. The logger module in the SDK generates a list of format IDs and data values which are then mapped to a corresponding log message through a post-processing Python script. The C-SDK log module is FW independent, but the post-processing Python script requires references to the format ID dictionaries generated along with a firmware release. The C-SDK output is in form of a Python dictionary and one log file is generated per Link. By default this file is named as **ltssm_log_X.py** (where X is the Link id). Per Link, the log will contain Main and Path Micro logs.

**ALMainFmtIDDict.py** and **ALPathFmtIDDict.py** contain the format id to log message translations. These files are released along with a new firmware, and are only valid with that firmware. If the firmware version of **ALMainFmtIDDict.py** and **ALPathFmtIDDict.py** are different from the SDK generated format id log, the resulting log messages are **invalid**. Although we do return an error message in such a case, please make sure that the FW version between files are consistent.

To run this post processing script, place the SDK generated log files in the **link_micro_logs** folder and run the python script. The output is printed to stdout and to a log file. The **--folder** parameter is optional. Here is an example to run the command:

```bash
python ./print_link_logs --folder link_micro_logs
```

where **link_micro_logs** is the directory containing the C SDK generated log files **ltssm_log_X.py**, and the firmware dictionaries **ALMainFmtIDDict.py** and **ALPathFmtIDDict.py**.

We have included the **link_micro_logs** directory in the SDK for reference.

## Updating the FW on Aries

The Aries SDK suite includes APIs to update the Firmware image stored in the EEPROM connected to the Retimer. The EEPROM programming happens via the Retimer. We provide a single routine to programming and verify the EEPROM:

```C
AriesErrorType ariesUpdateFirmware(
        AriesDeviceType* device,
        const char* filename,
        AriesFWImageFormatType fileType);
```

The input to the above function is the pointer to the device structure corresponding to the Retimer you want to update, the filepath to either the .ihx or .bin firmware file, and the FW image format type, either **ARIES_FW_IMAGE_FORMAT_IHX** or **ARIES_FW_IMAGE_FORMAT_BIN**, that is used to properly load the firmware image into a byte array.

### Rewriting and Re-verifying EEPROM Content on Failure

While verifying the EEPROM programming, if we encounter an error, i.e. the data read from the EEPROM does not match what you expected to write to the EEPROM, we rewrite the expected byte to the EEPROM and re-verify the byte at the same address.

### Updating Firmware on Aries over a Current Invalid Firmware

If the current firmware on Aries is invalid or empty we will detect this is the case and enable ARP (Address Resolution Protocol) in order to successfully perform read and write transactions. This is handled automatically by the **ariesInitDevice()** function.

## Examples

### Monitoring Link Stats

The **link_example** example application is written to monitor link health and dump statistics on an error. I2C, Device, and Link structures are initialized, Link warning and error parameters are set, and then **ariesCheckLinkHealth()** is used to poll the health of a link. If an error occurs **ariesLinkDumpDebugInfo()** is used to collect all necessary debug information. You can find this example in **examples/link_example.c**

### Programming the EEPROM

There are 2 ways to program the EEPROM. One way is to program the EEPROM via the Retimer, and the other is to program the EEPROM directly from the BMC.

#### Program the EEPROM via Retimer

The **eeprom_update** example application shows the necessary API calls to update the firmware image stored in EEPROM thru the Retimer. **ariesInitDevice()** is called, then the firmware is programmed using **ariesUpdateFirmware()**, the new firmware will be loaded after a device reset which is accomplished with **ariesSetPcieHwReset()**. You can find this example in **examples/eeprom_update.c**.

#### Program the EEPROM Directly

The **eeprom_direct** example application demonstrates the APIs to program the EEPROM directly and verify its contents against the expected firmware image. In order to run these APIs, the BMC must have a connection directly to the EEPROM slaves. You can find this example in **examples/eeprom_direct.c**.

## Change Log

### 2.7

* Add aries_margin.c/.h which impliment the out of band Receiver Lane Margining feature
* Add ariesLinkGetCurrentWidth to report number of lanes in FWD state
* Add curWidth as a variable in ariesLinkState structure
* Add ariesSetI2CMasterReset to halt tranactions on the I2C bus connected to the EEPROM
* Update print_link_state script to also report current width

### 2.6.1

* Fix register address for RLD_ON_SBR flag

### 2.6

* Add logger callback function
* Add aries_link files to include Link level APIs and moved APIs to collect debug info
* Enhance ways logs are collected to accept path and filename
* Enhance the Main Micro assisted EEPROM write APIs to reduce write time on FW 1.24.0 and later
* Update link_example to be a better example of fleet management feature
* Update README.md to better document SDK integration into BMC
* Update and fix Doxygen documentation

### 2.5.3

* Enhance firmware APIs to use a const char pointer for filename
* Enhance temperature measurements APIs to return error code if data not ready
* Add ariesFirmwareUpdateProgress API to report FW update percent complete
* Add APIs and example application to read and clear I2C bus clear event status

### 2.5.2

* Add ariesFWReloadOnNextSBR API to trigger Aries to reload FW on next SBR
* Add return code checks for all ariesUnlock API calls
* Replace all occurrences of RX_ADAPT_MM_FOM with RX_ADAPT_FOM

### 2.5.1

* Fix incorrect asteraI2COpenConnection function names
* Add recoveryAddr as argument to ariesInitDevice
* Add option to use binary file to ariesUpdateFirmware
* Improve and consolidate include files and sourcing
* Fix and improve various compile warnings

### 2.5

* Add singular ariesUpdateFirmware function to aries_api which takes file path as input
* Add eeprom_update example showing recommended way to update firmware on Aries
* Improve ariesInit to automatically check SMBus connection run ARP if required
* Update default ARP address to 0x55 to align with Python SDK
* Add tempCalCodeAvg to AriesDevice struct and improve accuracy of temperature reporting
* Add progress logs to the EEPROM write and verify
* Update AriesBifurcationType values
* Update and clean code formatting consistency

### 2.4.1

* Update example PRBS test application (examples/prbs.c) to better illustrate suggested test flow
* Fix bugs in ariesTestMode\*() APIs. Function prototypes did not change.

### 2.4

* Add APIs for performing PRBS pattern generation & checking (ariesTestMode\*)
* Add logging of error return codes
* Fix some lint/style issues in examples/links.c and examples/source/parse_ihx_file.c
* Add example application: examples/prbs.c

### 2.3

* Fix print_link_state processing script determining link speed
* Fix which path micro to read from to support x8 devices
* Update Link State Type values
* Enhance consistency of heartbeat check code
* Enhance eeprom_test write and verify behaviour
* Add ariesGetLinkRecoveryCount and ariesClearLinkRecoveryCount to aries_api

### 2.2.1

* Fix log processing scripts
* Update SDK version type from float to char
* Add SDK version to generated python log files
* Fix direction in ariesGetLinkRxTerm

### 2.2

* Fix verify via checksum
* Update LTSSM logger post-processing script
* Various bug fixes