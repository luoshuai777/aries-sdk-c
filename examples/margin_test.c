/**
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
 * @file margin_test.c
 * @brief Example application showing how to check eye stats using aries_margin.c
 */

#include "../include/aries_api.h"
#include "../include/aries_margin.h"
#include "include/aspeed.h"

int main(void) {

    // Structs we will need to set up and margin the device
    AriesDeviceType *ariesDevice;
    AriesI2CDriverType *i2cDriver;
    AriesDevicePartType partNumber = ARIES_PTX16;
    AriesOrientationType orientation;
    AriesErrorType rc;

    asteraLogSetLevel(1);

    // Connect to retimer
    i2cDriver = (AriesI2CDriverType *) malloc(sizeof(AriesI2CDriverType));
    i2cDriver->handle = asteraI2COpenConnection(1, 0x20);
    i2cDriver->slaveAddr = 0x20;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
    // Flag to indicate lock has not been initialized. Call ariesInitDevice()
    // later to initialize.
    i2cDriver->lockInit = 0;


    ariesDevice = (AriesDeviceType *) malloc(sizeof(AriesDeviceType));
    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = 1;
    ariesDevice->partNumber = ARIES_PTX16;


    rc = ariesInitDevice(ariesDevice, 0x55);
    if (rc != ARIES_SUCCESS) {
        ASTERA_ERROR("Init device failed");
        return rc;
    }
    ASTERA_INFO("FW VERSION: %d.%d.%d", ariesDevice->fwVersion.major,
                ariesDevice->fwVersion.minor, ariesDevice->fwVersion.build);
    // Get device orientation

    int o;
    rc = ariesGetPortOrientation(ariesDevice, &o);

    if (o == 0) {
        orientation = ARIES_ORIENTATION_NORMAL;
    } else if (o == 1) {
        orientation = ARIES_ORIENTATION_REVERSED;
    } else {
        ASTERA_ERROR("Could not determine port orientation");
        return ARIES_INVALID_ARGUMENT;
    }

    uint8_t *ec1 = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    uint8_t *ec2 = (uint8_t *) malloc(sizeof(uint8_t) * 16);
    uint8_t **ec = (uint8_t **) malloc(sizeof(uint8_t * ) * 2);

    ec[0] = ec1;
    ec[1] = ec2;

    // initialize our margin device. This will store important information about the
    // device we will margin
    AriesRxMarginType *marginDevice = (AriesRxMarginType *) malloc(sizeof(AriesRxMarginType));
    marginDevice->i2cDriver = i2cDriver;
    marginDevice->partNumber = partNumber;
    marginDevice->orientation = orientation;
    marginDevice->do1XAnd0XCapture = true;
    marginDevice->errorCountLimit = 4;
    marginDevice->errorCount = ec;

    int width;
    if (marginDevice->partNumber == ARIES_PTX16)
    {
        width = 16;
    }
    else if (marginDevice->partNumber == ARIES_PTX08)
    {
        width = 8;
    }

    // initialize our eyeResults array that we will store the results from ariesLogEye() in
    // (the results will also be saved to a file)
    double ***eyeResults = (double ***) malloc(sizeof(double **) * 2);
    int i = 0;
    for (i = 0; i < 2; i++)
    {
        eyeResults[i] = (double **) malloc(sizeof(double *) * width);
        int j;
        for (j = 0; j < width; j++)
        {
            eyeResults[i][j] = (double *) malloc(sizeof(double) * 4);
        }
    }

    // initialize our eyeDiagram array that we will store the results from ariesEyeDiagram() in
    // (the results will also be saved in their respective files)
    // [port]  [lane]  [timingsteps]    [voltagesteps]
    //   2     width  NUMTIMINGSTEPS    NUMVOLTAGESTEPS
    int ****eyeDiagramArr = (int ****) malloc(sizeof(int ***) * 2);
    for (i = 0; i < 2; i++)
    {
        eyeDiagramArr[i] = (int ***) malloc(sizeof(int **) * width);
        int j;
        for (j = 0; j < width; j++)
        {
            eyeDiagramArr[i][j] = (int **) malloc(sizeof(int *) * (NUMTIMINGSTEPS + 1));
            int k;
            for (k = 0; k < NUMTIMINGSTEPS + 1; k++)
            {
                eyeDiagramArr[i][j][k] = (int *) malloc(sizeof(int) * 15);
            }
        }
    }

    // Run the ariesLogEye method to check the eye stats for all the lanes on the Up stream pseudo port on this device
    // and save them to a document
    ariesLogEye(marginDevice, ARIES_UP_STREAM_PSEUDO_PORT, width, "margin_test", 0, 0.5, eyeResults);

    // Run eyeDiagram method to find the eye for all lanes on the UPSTREAMPSEUDOPORT on this device
    // the results will be saved in our eyeDiagramArr variable and will also be saved to respective files.
    for (i = 0; i < width; i++)
    {
        ariesEyeDiagram(marginDevice, ARIES_UP_STREAM_PSEUDO_PORT, i, 4, 0.5, eyeDiagramArr);
    }


    // Releasing memory
    free(i2cDriver);
    free(ariesDevice);
    free(ec1);
    free(ec2);
    free(ec);

    for (i = 0; i < 2; i++)
    {
        int j;
        for (j = 0; j < width; j++)
        {
            free(eyeResults[i][j]);
        }
        free(eyeResults[i]);
    }
    free(eyeResults);

    for (i = 0; i < 2; i++)
    {
        int j;
        for (j = 0; j < width; j++)
        {
            int k;
            for (k = 0; k < NUMTIMINGSTEPS; k++)
            {
                free(eyeDiagramArr[i][j][k]);
            }
            free(eyeDiagramArr[i][j]);
        }
        free(eyeDiagramArr[i]);
    }
    free(eyeDiagramArr);

    free(marginDevice);

}
