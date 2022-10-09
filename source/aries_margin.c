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
 * @file aries_margin.c
 * @brief Implementation of receiver margining functions for the SDK.
 */

#include "../include/aries_margin.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * noCommand does nothing
 */
AriesErrorType ariesMarginNoCommand(
        AriesRxMarginType* marginDevice)
{
    ASTERA_WARN("Sending this out of band doesn't do anything");
    return ARIES_SUCCESS;
}

/*
 * Not implemented
 */
AriesErrorType ariesMarginAccessRetimerRegister(
        AriesRxMarginType* marginDevice)
{
    ASTERA_WARN("Register access is not implemented yet");
    return ARIES_SUCCESS;
}

/*
 * Report margin control capabilities
 */
AriesErrorType ariesMarginReportMarginControlCapabilities(
        AriesRxMarginType* marginDevice,
        int* capabilities)
{
    ASTERA_INFO("Reporting Margin Control Capabilities:");
    ASTERA_INFO("\tM_VOLTAGE_SUPPORTED = %d", VOLTAGESUPPORTED);
    ASTERA_INFO("\tM_IND_UP_DOWN_VOLTAGE = %d", INDUPDOWNVOLTAGE);
    ASTERA_INFO("\tM_IND_LEFT_RIGHT_TIMING = %d", INDLEFTRIGHTTIMING);
    ASTERA_INFO("\tM_SAMPLE_REPORTING_METHOD = %d", SAMPLEREPORTINGMETHOD);
    ASTERA_INFO("\tM_IND_ERROR_SAMPLER = %d", INDERRORSAMPLER);
    capabilities[0] = VOLTAGESUPPORTED;
    capabilities[1] = INDUPDOWNVOLTAGE;
    capabilities[2] = INDLEFTRIGHTTIMING;
    capabilities[3] = SAMPLEREPORTINGMETHOD;
    capabilities[4] = INDERRORSAMPLER;

    return ARIES_SUCCESS;
}

/*
 * Report number of voltage steps
 */
AriesErrorType ariesMarginReportNumVoltageSteps(
        AriesRxMarginType* marginDevice,
        int* numVoltageSteps)
{
    *numVoltageSteps = NUMVOLTAGESTEPS;
    return ARIES_SUCCESS;
}

/*
 * Report number of timing steps
 */
AriesErrorType ariesMarginReportNumTimingSteps(
        AriesRxMarginType* marginDevice,
        int* numTimingSteps)
{
    *numTimingSteps = NUMTIMINGSTEPS;
    return ARIES_SUCCESS;
}

/*
 * Report max timing offset
 */
AriesErrorType ariesMarginReportMaxTimingOffset(
        AriesRxMarginType* marginDevice,
        int* maxTimingOffset)
{
    *maxTimingOffset = MAXTIMINGOFFSET;
    return ARIES_SUCCESS;
}

/*
 * Report max voltage offset
 */
AriesErrorType ariesMarginReportMaxVoltageOffset(
        AriesRxMarginType* marginDevice,
        int* maxVoltageOffset)
{
    *maxVoltageOffset = MAXVOLTAGEOFFSET;
    return ARIES_SUCCESS;
}

/*
 * Report sampling rate voltage
 */
AriesErrorType ariesMarginReportSamplingRateVoltage(
        AriesRxMarginType* marginDevice,
        int* samplingRateVoltage)
{
    *samplingRateVoltage = SAMPLINGRATEVOLTAGE;
    return ARIES_SUCCESS;
}

/*
 * Report sampling rate timing
 */
AriesErrorType ariesMarginReportSamplingRateTiming(
        AriesRxMarginType* marginDevice,
        int* samplingRateTiming)
{
    *samplingRateTiming = NUMTIMINGSTEPS;
    return ARIES_SUCCESS;
}

/*
 * We do not use sample count
 */
AriesErrorType ariesMarginReportSampleCount(
        AriesRxMarginType* marginDevice)
{
    ASTERA_WARN("We support sampling rates instead of sample count");
    return ARIES_SUCCESS;
}

/*
 * Report max lanes
 */
AriesErrorType ariesMarginReportMaxLanes(
        AriesRxMarginType* marginDevice,
        int* maxLanes)
{
    *maxLanes = MAXLANES;
    return ARIES_SUCCESS;
}

/*
 * Set the Error Count Limit
 */
AriesErrorType ariesMarginSetErrorCountLimit(
        AriesRxMarginType* marginDevice,
        int limit)
{
    if (limit > 63)
    {
        ASTERA_WARN("Error Count Limit cannot be greater than 63. Setting it to 63");
        marginDevice->errorCountLimit = 63;
    }
    else
    {
        ASTERA_INFO("Error Count Limit is %d", limit);
        marginDevice->errorCountLimit = limit;
    }
    return ARIES_SUCCESS;
}

/*
 * Reset the phase and voltage offset and disable margining
 */
AriesErrorType ariesMarginGoToNormalSettings(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane)
{
    AriesErrorType rc;

    rc = ariesMarginPmaRxMarginStop(marginDevice, port, lane);
    CHECK_SUCCESS(rc)

    rc = ariesMarginClearErrorLog(marginDevice, port, lane);
    CHECK_SUCCESS(rc)

    return ARIES_SUCCESS;
}

/*
 * Clears error count of a given port and lane
 */
AriesErrorType ariesMarginClearErrorLog(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane)
{
    marginDevice->errorCount[port][lane] = 0;
    return ARIES_SUCCESS;
}

/*
 * Margin the device at a specified timing offset
 */
AriesErrorType ariesMarginStepMarginToTimingOffset(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int direction,
        int steps,
        double dwell,
        int* eCount)
{
    AriesErrorType rc;
    if (direction != 0 && direction != 1)
    {
        ASTERA_ERROR("Unsupported direction argument, must be 1 or 0");
        return ARIES_INVALID_ARGUMENT;
    }
    else if (steps > NUMTIMINGSTEPS)
    {
        ASTERA_ERROR("Unsupported Lane Margining command: Exceeded NumTimingSteps");
        return ARIES_INVALID_ARGUMENT;
    }
    else
    {
        rc = ariesMarginPmaRxMarginTiming(marginDevice, port, lane, direction, steps);
        CHECK_SUCCESS(rc)
        usleep((int) (dwell * 1000000));
        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
        CHECK_SUCCESS(rc)
        if (marginDevice->do1XAnd0XCapture)
        {
            rc = ariesMarginPmaRxMarginTiming(marginDevice, port, lane, direction, steps);
            CHECK_SUCCESS(rc)
            usleep((int) (dwell * 100000));
            rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
        }
        if (marginDevice->errorCount[port][lane] > 63)
            marginDevice->errorCount[port][lane] = 63;
        if (marginDevice->errorCount[port][lane] > marginDevice->errorCountLimit)
        {
            ASTERA_WARN("Error count on port %d lane %d exceeded error count limit: %d > %d",
                        port,lane, marginDevice->errorCount[port][lane],marginDevice->errorCountLimit);
            *eCount = marginDevice->errorCount[port][lane];
            ASTERA_INFO("Port %d lane %d is going back to default settings", port, lane);
            rc = ariesMarginGoToNormalSettings(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
            return ARIES_SUCCESS;
        }
        else
        {
            ASTERA_INFO("Margining time is in progress on port %d lane %d. Current error count is: %d",
                        port, lane, marginDevice->errorCount[port][lane]);
            *eCount = marginDevice->errorCount[port][lane];
            return ARIES_SUCCESS;
        }
    }
}

/*
 * Margin the device at a specified voltage offset
 */
AriesErrorType ariesMarginStepMarginToVoltageOffset(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int direction,
        int steps,
        double dwell,
        int* eCount)
{
    AriesErrorType rc;
    if (direction != 0 && direction != 1)
    {
        ASTERA_ERROR("Unsupported direction argument, must be 0 or 1");
        return ARIES_INVALID_ARGUMENT;
    }
    else if (steps > NUMVOLTAGESTEPS)
    {
        ASTERA_ERROR("Unsupported Lane Margining command: Exceeded NumTimingSteps");
        return ARIES_INVALID_ARGUMENT;
    }
    else
    {
        rc = ariesMarginPmaRxMarginVoltage(marginDevice, port, lane, direction, steps);
        CHECK_SUCCESS(rc)
        usleep((int) (dwell * 100000));
        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
        CHECK_SUCCESS(rc)
        if (marginDevice->do1XAnd0XCapture)
        {
            direction = 1 - direction;
            rc = ariesMarginPmaRxMarginVoltage(marginDevice, port, lane, direction, steps);
            CHECK_SUCCESS(rc)
            usleep((int) (dwell * 100000));
            rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
        }
        if (marginDevice->errorCount[port][lane] > 63)
            marginDevice->errorCount[port][lane] = 63;
        if (marginDevice->errorCount[port][lane] > marginDevice->errorCountLimit)
        {
            ASTERA_WARN("Error count on port %d lane %d exceeded error count limit: %d > %d",
                        port,lane, marginDevice->errorCount[port][lane],marginDevice->errorCountLimit);
            *eCount = marginDevice->errorCount[port][lane];
            ASTERA_INFO("Port %d lane %d is going back to default settings", port, lane);
            rc = ariesMarginGoToNormalSettings(marginDevice, port, lane);
            CHECK_SUCCESS(rc)

            return ARIES_SUCCESS;
        }
        else
        {
            ASTERA_INFO("Margining voltage is in progress on port %d lane %d. Current error count is: %d",
                        port, lane, marginDevice->errorCount[port][lane]);
            *eCount = marginDevice->errorCount[port][lane];

            return ARIES_SUCCESS;
        }
    }
}

/*
 * Not implemented
 */
AriesErrorType ariesMarginVendorDefined(
        AriesRxMarginType* marginDevice)
{
    ASTERA_WARN("Vendor defined function is not implemented yet");
    return ARIES_SUCCESS;
}

/*
 * Tell the retimer we are finished margining
 */
AriesErrorType ariesMarginPmaRxMarginStop(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;

    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice,&quadSliceLane);
    CHECK_SUCCESS(rc)

    uint8_t dataWord[2];
    rc = ariesReadWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice, quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, dataWord);
    CHECK_SUCCESS(rc)
    uint8_t en = (dataWord[0] >> 1) & 0x1; // enable has offset of 1

    if (en == 1)
    {
        // Setting OVRD enable to 0 for rx IQ
        // Offset of 12, width of 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                        12, 0, 1);
        CHECK_SUCCESS(rc)
        // Setting OVRD enable to 0 for margin vdac
        // Offset of 11, width of 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        11, 0, 1);
        CHECK_SUCCESS(rc)
        // Setting OVRD enable to 0 for margin in progress
        // Offset of 13, width of 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        13, 0, 1);
        CHECK_SUCCESS(rc)

        rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane);
        CHECK_SUCCESS(rc)

        // Setting OVRD enable to 0 for margin error clear
        // Offset of 1, width of 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        1, 0, 1);
        CHECK_SUCCESS(rc)
    }

    return ARIES_SUCCESS;
}

/*
 * Set margin timing sampler to a specified time value
 */
AriesErrorType ariesMarginPmaRxMarginTiming(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int direction,
        int steps)
{
    AriesErrorType rc;

    // Determine pma side and qs from device port and lane
    int side, quadSlice, quadSliceLane;
    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice,
                               &quadSliceLane);
    CHECK_SUCCESS(rc)

    // positive deltaValues move the slicer to the left
    // assume the direction is left and if it is right we will overwrite deltaValue later
    uint8_t deltaValue = steps;

    // negative deltaValues move the slicer to the right
    // if the direction is right, and we want to move at least 1 step then take the negation
    // of our steps, so we move the opposite direction
    if (direction == 1 && steps > 0) // right and not 0 steps
    {
        deltaValue = (( ~steps + 1) & 0x7f);
    }

    // Setting IQ OVRD to 1 and setting the IQ value
    // offset 12, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                12, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 5, width 7
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                5, deltaValue, 7);
    CHECK_SUCCESS(rc)

    // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd to 1
    // ALS-105: Stop a dummy req being generated by keeping Margin error clear high
    // in case iq value is unchanged form last command
    // offset 1, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                1, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 0, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                0, 1, 1);
    CHECK_SUCCESS(rc)

    // set rxX margin in prog
    // offset 13, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                13, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 12, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                12, 1, 1);
    CHECK_SUCCESS(rc)

    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane);
    CHECK_SUCCESS(rc)

    return ARIES_SUCCESS;
}

/*
 * Set margin voltage sampler to a specified voltage
 */
AriesErrorType ariesMarginPmaRxMarginVoltage(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int direction,
        int steps)
{
    AriesErrorType rc;

    // Determine pma side and qs from device port and lane
    int side, quadSlice, quadSliceLane;
    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice,
                               &quadSliceLane);
    CHECK_SUCCESS(rc)

    // positive vdacValues move the slicer up
    // assume the direction is up and if it is down we will overwrite deltaValue later

    uint16_t vdacValue = steps;

    // negative vdacValues move the slicer down
    // if the direction is down, and we want to move at least 1 step then take the negation
    // of our steps, so we move the opposite direction (aka down)
    if (direction == 1 && steps > 0) // down or not 0 steps
    {
        vdacValue = (( ~steps + 1) & 0x1ff);
    }

    // set VDAC override to 1 and set the VDAC value
    // offset 11, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                11, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 2, width 9
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                2, vdacValue, 9);
    CHECK_SUCCESS(rc)

    // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd to 1
    // ALS-105: Stop a dummy req being generated by keeping Margin error clear high
    // in case iq value is unchanged form last command
    // offset 1, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                1, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 0, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                0, 1, 1);
    CHECK_SUCCESS(rc)

    // set rxX margin in prog
    // offset 13, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                13, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 12, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                12, 1, 1);
    CHECK_SUCCESS(rc)

    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane);
    CHECK_SUCCESS(rc)

    return ARIES_SUCCESS;
}

/*
 * Set margin sampler to specified timing and voltage
 */
AriesErrorType ariesMarginPmaTimingVoltageOffset(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int timeDirection,
        int timeSteps,
        int voltageDirection,
        int voltageSteps,
        double dwell,
        int* eCount)
{
    AriesErrorType rc;

    // Determine pma side and qs from device port and lane
    int side, quadSlice, quadSliceLane;
    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice,
                               &quadSliceLane);
    CHECK_SUCCESS(rc)

    // get the 1X eye capture
    uint16_t deltaValue, vdacValue;
    deltaValue = timeSteps;
    vdacValue = voltageSteps;

    if (timeDirection == 1 && timeSteps != 0)
    {
        deltaValue = ((~timeSteps + 1) & 0x7f);
    }

    if (voltageDirection == 1 && timeSteps != 0)
    {
        vdacValue = ((~voltageSteps + 1) & 0x1ff);
    }

    // set IQ override to 1 and set the IQ value
    // offset 12, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                    12, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 5, width 7
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                    5, deltaValue, 7);
    CHECK_SUCCESS(rc)
    // set VDAC override to 1 and set the VDAC value
    // offset 11, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    11, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 2, width 9
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    2, vdacValue, 9);
    CHECK_SUCCESS(rc)

    // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd to 1
    // ALS-105: Stop a dummy req being generated by keeping Margin error clear high
    // in case iq value is unchanged form last command
    // offset 1, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    1, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 0, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    0, 1, 1);
    CHECK_SUCCESS(rc)

    // set rxX margin in prog
    // offset 13, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    13, 1, 1);
    CHECK_SUCCESS(rc)
    // offset 12, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                    12, 1, 1);
    CHECK_SUCCESS(rc)

    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane);
    CHECK_SUCCESS(rc)
    CHECK_SUCCESS(rc)

    usleep((int) (dwell * 100000));

    rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
    CHECK_SUCCESS(rc)

    if (marginDevice->do1XAnd0XCapture)
    {
        voltageDirection = 1 - voltageDirection;

        if (timeDirection == 1 && timeSteps != 0)
        {
            deltaValue = ((~timeSteps + 1) & 0x7f);
        }

        if (voltageDirection == 1 && timeSteps != 0)
        {
            vdacValue = ((~voltageSteps + 1) & 0x1ff);
        }

        // set IQ override to 1 and set the IQ value
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                        12, 1, 1);
        CHECK_SUCCESS(rc)
        // offset 5, with 7
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7,
                                                        5, deltaValue, 7);
        CHECK_SUCCESS(rc)
        // set VDAC override to 1 and set the VDAC value
        // offset 11, with 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        11, 1, 1);
        CHECK_SUCCESS(rc)
        // offset 2, width 9
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        2, vdacValue, 9);
        CHECK_SUCCESS(rc)

        // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd to 1
        // ALS-105: Stop a dummy req being generated by keeping Margin error clear high
        // in case iq value is unchanged form last command
        // offset 1, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        1, 1, 1);
        CHECK_SUCCESS(rc)
        // offset 0, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        0, 1, 1);
        CHECK_SUCCESS(rc)

        // set rxX margin in prog
        // offset 13, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        13, 1, 1);
        CHECK_SUCCESS(rc)
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                        quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9,
                                                        12, 1, 1);
        CHECK_SUCCESS(rc)

        rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane);
        CHECK_SUCCESS(rc)

        usleep((int) (dwell * 100000));

        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane);
        CHECK_SUCCESS(rc)

    }

    if (marginDevice->errorCount[port][lane] > 63)
        marginDevice->errorCount[port][lane] = 63;

    *eCount = marginDevice->errorCount[port][lane];

    return ARIES_SUCCESS;
}

/*
 * Updates the errorCount array with the number of errors that occurred in a given port and lane
 */
AriesErrorType ariesMarginPmaRxMarginGetECount(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane)
{
    // Determine pma side and quad slice
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice, &quadSliceLane);
    CHECK_SUCCESS(rc)

    // Get error count from error count register
    uint8_t dataWord[2];
    rc = ariesReadWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice, quadSliceLane,
                                               ARIES_PMA_RAWLANE_DIG_RX_CTL_RX_MARGIN_ERROR, dataWord);
    CHECK_SUCCESS(rc)

    uint8_t eCount = dataWord[0] & 0x3f; // eCount is only 6 bits wide. We want the first 6 bits.

    //update errorCount array
    marginDevice->errorCount[port][lane] += eCount;

    return ARIES_SUCCESS;
}

/*
 * Send handshake for a port and lane
 */
AriesErrorType ariesMarginPmaRxReqAckHandshake(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane, &side, &quadSlice, &quadSliceLane);
    CHECK_SUCCESS(rc)

    // Assert rxX_req
    // Force 0
    // Offset 5, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN,
                                                    5, 1, 1);
    CHECK_SUCCESS(rc)
    // Offset 4, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN,
                                                    4, 0, 1);
    CHECK_SUCCESS(rc)

    // Force 1
    // Offset 5, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN,
                                                    5, 1, 1);
    CHECK_SUCCESS(rc)
    // Offset 4, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN,
                                                    4, 1, 1);
    CHECK_SUCCESS(rc)

    // Check for ack
    uint8_t dataWord[2];
    uint8_t ack = 0x0;
    int count = 0;
    while (ack == 0)
    {
        rc = ariesReadWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                   quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_PCS_OUT,
                                                   dataWord);
        ack = dataWord[0] & 0x1; // only care about first bit
        CHECK_SUCCESS(rc)
        count += 1;

        if( count > 0x3fff)
        {
            ASTERA_ERROR("During Rx req handshake, ACK timed out");
            break;
        }
    }
    // Set rxX_req override to 0
    // Offset 5, width 1
    rc = ariesReadWriteWordPmaLaneMainMicroIndirect(marginDevice->i2cDriver, side, quadSlice,
                                                    quadSliceLane, ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN,
                                                    5, 0, 1);
    CHECK_SUCCESS(rc)

    return ARIES_SUCCESS;
}

/*
 * Determine the PMA side and quadslice of a port and lane
 */
AriesErrorType ariesMarginDeterminePmaSideAndQs(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int* side,
        int* quadSlice,
        int* quadSliceLane)
{
    if (marginDevice->partNumber == ARIES_PTX16)
    {
        if (marginDevice->orientation == ARIES_ORIENTATION_NORMAL)  // Normal Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT)  // USPP
            {
                *side = 1;  // Side is a
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT)  // DSPP
            {
                *side = 0;  // Side is b
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else if (marginDevice->orientation == ARIES_ORIENTATION_REVERSED)  // Reversed Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT)  // USPP
            {
                *side = 0;  // Side is b
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT)  // DSPP
            {
                *side = 1;  // Side is a
            }
            else
            {
                ASTERA_ERROR("Invalid port: %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else
        {
            ASTERA_ERROR("Invalid Orientation: %d", marginDevice->orientation);
            return ARIES_INVALID_ARGUMENT;
        }
        *quadSlice = lane / 4;
    }
    else if (marginDevice->partNumber == ARIES_PTX08)
    {
        if (marginDevice->orientation == ARIES_ORIENTATION_NORMAL)  // Normal Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT)  // USPP
            {
                *side = 0;  // Side is b
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT)  // DSPP
            {
                *side = 1;  // Side is a
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else if (marginDevice->orientation == ARIES_ORIENTATION_REVERSED)  // Reversed Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT)  // USPP
            {
                *side = 1;  // Side is a
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT)  // DSPP
            {
                *side = 0;  // Side is b
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else
        {
            ASTERA_ERROR("Invalid Orientation: %d", marginDevice->orientation);
            return ARIES_INVALID_ARGUMENT;
        }
        *quadSlice = lane / 4 + 1;
    }
    else
    {
        ASTERA_ERROR("Failed to match board Part Number: %d", marginDevice->partNumber);
        return ARIES_INVALID_ARGUMENT;
    }

    *quadSliceLane = lane % 4;

    return ARIES_SUCCESS;
}

/*
 * determines eye height using binary search
 */
AriesErrorType ariesCheckEye(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        double dwell,
        double*** eyeResults)
{
    AriesErrorType rc;
    // timing
    int i;
    for (i = 0; i < 2; i++){ // 0:left, 1:right
        rc = ariesMarginGoToNormalSettings(marginDevice, port, lane);
        CHECK_SUCCESS(rc)
        int low = 0;
        int high = NUMTIMINGSTEPS;
        int steps;
        while (low != high)
        {
            steps = (low + high + 1) / 2;
            rc = ariesMarginClearErrorLog(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
            ASTERA_INFO("Checking timing offset direction %d steps %d", i, steps);
            int errorCount = 0;
            rc = ariesMarginStepMarginToTimingOffset(marginDevice, port, lane, i, steps, dwell, &errorCount);
            CHECK_SUCCESS(rc)
            if (errorCount > marginDevice->errorCountLimit)
            {
                // can't be here anymore. We saw too many errors here
                high = steps - 1;
            }
            else
            {
                low = steps;
                if (steps == NUMTIMINGSTEPS)
                {
                    ASTERA_INFO("We reached maximum timing offset, exiting margining");
                    break;
                }
            }
        }
        eyeResults[port][lane][i] = low;
    }

    // voltage
    for (i = 0; i < 2; i++){ // 0:up, 1:down
        rc = ariesMarginGoToNormalSettings(marginDevice, port, lane);
        CHECK_SUCCESS(rc)
        int low = 0;
        int high = NUMVOLTAGESTEPS;
        int steps;
        while (low != high)
        {
            steps = (low + high + 1) / 2;
            rc = ariesMarginClearErrorLog(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
            ASTERA_INFO("Checking voltage offset direction %d steps %d", i, steps);
            int errorCount = 0;
            rc = ariesMarginStepMarginToVoltageOffset(marginDevice, port, lane, i, steps, dwell, &errorCount);
            CHECK_SUCCESS(rc)
            if (errorCount > marginDevice->errorCountLimit)
            {
                // can't be here anymore. We saw too many errors here
                high = steps - 1;
            }
            else
            {
                low = steps;
                if (steps == NUMVOLTAGESTEPS)
                {
                    ASTERA_INFO("We reached maximum voltage offset, exiting margining");
                    break;
                }
            }
        }
        eyeResults[port][lane][i+2] = low;
    }

    double eyeWidthLeft = eyeResults[port][lane][0] / (double) NUMTIMINGSTEPS * (double) MAXTIMINGOFFSET;
    double eyeWidthRight = eyeResults[port][lane][1] / (double) NUMTIMINGSTEPS * (double) MAXTIMINGOFFSET;
    double eyeHeightUp = eyeResults[port][lane][2] / (double) NUMVOLTAGESTEPS * (double) MAXVOLTAGEOFFSET;
    double eyeHeightDown = eyeResults[port][lane][3] / (double) NUMVOLTAGESTEPS * (double) MAXVOLTAGEOFFSET;
    ASTERA_INFO("Eye stats for port %d lane %d", port, lane);
    ASTERA_INFO("\tWidth = -%.2fUI to %.2fUI", eyeWidthLeft/100.0, eyeWidthRight/100.0);
    ASTERA_INFO("\tHeight = -%.0fmv to %.0fmv", eyeHeightDown*10.0, eyeHeightUp*10.0);
    return ARIES_SUCCESS;
}

/*
 * logs eye results to a file
 */
AriesErrorType ariesLogEye(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int width,
        const char* filename,
        int startLane,
        double dwell,
        double*** eyeResults)
{
    AriesErrorType rc;

    if (width == 0)
    {
        if (marginDevice->partNumber == ARIES_PTX16)
        {
            width = 16;
        }
        else if (marginDevice->partNumber == ARIES_PTX08)
        {
            width = 8;
        }
    }

    char filepath[ARIES_PATH_MAX];
    snprintf(filepath, ARIES_PATH_MAX, "%s_%d.csv", filename, port);

    FILE* fp;
    fp = fopen(filepath, "w");
    // Adding header
    fprintf(fp, "Lane,Timing_neg_UI%%,Timing_pos_UI%%,Timing_tot_UI%%,Voltage_neg_mV,Voltage_pos_mV,Voltage_tot_mV\n");
    int i;
    for (i = startLane; i < startLane + width; i++)
    {
        rc = ariesCheckEye(marginDevice, port, i, dwell, eyeResults);
        CHECK_SUCCESS(rc)
        double eyeWidthLeft = eyeResults[port][i][0] / (double) NUMTIMINGSTEPS * (double) MAXTIMINGOFFSET;
        double eyeWidthRight = eyeResults[port][i][1] / (double) NUMTIMINGSTEPS * (double) MAXTIMINGOFFSET;
        double eyeHeightUp = eyeResults[port][i][2] / (double) NUMVOLTAGESTEPS * (double) MAXVOLTAGEOFFSET;
        double eyeHeightDown = eyeResults[port][i][3] / (double) NUMVOLTAGESTEPS * (double) MAXVOLTAGEOFFSET;
        fprintf(fp, "%d, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", i, eyeWidthLeft, eyeWidthRight, eyeWidthLeft + eyeWidthRight,
                eyeHeightDown * 10, eyeHeightUp * 10, (eyeHeightUp + eyeHeightDown) * 10);
    }

    fclose(fp);
    return ARIES_SUCCESS;
}

/*
 * Determine eye by going step by step
 */
AriesErrorType ariesSweepEye(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        double dwell,
        double**** eyeResults)
{
    AriesErrorType rc;
    // timing
    int i;
    for (i = 0; i < 2; i++)
    {
        int steps;
        for (steps = 0; steps <= NUMTIMINGSTEPS; steps++)
        {
            rc = ariesMarginClearErrorLog(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
            ASTERA_INFO("Checking timing offset direction %d steps %d", i, steps);
            int errorCount = 0;
            rc = ariesMarginStepMarginToTimingOffset(marginDevice, port, lane, i, steps, dwell, &errorCount);
            CHECK_SUCCESS(rc)
            eyeResults[port][lane][i][steps] = errorCount;
        }
    }
    // voltage
    for (i = 0; i < 2; i++)
    {
        int steps;
        for (steps = 0; steps <= NUMVOLTAGESTEPS; steps++)
        {
            rc = ariesMarginClearErrorLog(marginDevice, port, lane);
            CHECK_SUCCESS(rc)
            ASTERA_INFO("Checking voltage offset direction %d steps %d", i, steps);
            int errorCount = 0;
            rc = ariesMarginStepMarginToVoltageOffset(marginDevice, port, lane, i, steps, dwell, &errorCount);
            CHECK_SUCCESS(rc)
            eyeResults[port][lane][i+2][steps] = errorCount;
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Create an eyeDiagram of the device on a specific port and lane
 */
AriesErrorType ariesEyeDiagram(
        AriesRxMarginType* marginDevice,
        AriesPseudoPortType port,
        int lane,
        int rate,
        double dwell,
        int**** eyeResults)
{
    AriesErrorType rc;

    int* timingOffsets = (int*) malloc(sizeof(int) * NUMTIMINGSTEPS + 1);
    int voltageOffsets[] = {70, 60, 50, 40, 30, 20, 10, 0, -10, -20, -30, -40, -50, -60, -70};
    int i;
    if (rate == 3)
    {
        for(i = 0; i < NUMTIMINGSTEPS + 1; i++)
        {
            timingOffsets[i] = (-NUMTIMINGSTEPS * 2) + (4 * i);
        }
    }
    else if (rate >= 4 && rate < 6)
    {
        for(i = 0; i < NUMTIMINGSTEPS + 1; i++)
        {
            timingOffsets[i] = (-NUMTIMINGSTEPS) + (2 * i);
        }
    }
    else
    {
        ASTERA_ERROR("%d is not a valid rate", rate);
        return ARIES_INVALID_ARGUMENT;
    }

    char filepath[ARIES_PATH_MAX];
    snprintf(filepath, ARIES_PATH_MAX, "eye_diagram_%d_lane%d.csv", port, lane);

    FILE* fp;
    fp = fopen(filepath, "w");
    int voltageOffset;
    for (voltageOffset = 0; voltageOffset < 15; voltageOffset++)
    {
        fprintf(fp, "%3d,,", voltageOffsets[voltageOffset]);
        for (i = 0; i < NUMTIMINGSTEPS + 1; i++)
        {
            rc = ariesMarginClearErrorLog(marginDevice, port, lane);
            CHECK_SUCCESS(rc);

            int errorCount = 0;
            int timeDirection = 0;
            if (timingOffsets[i] < 0)
            {
                timeDirection = 0; // left
            }
            else
            {
                timeDirection = 1; // right
            }
            int timeSteps = abs(timingOffsets[i]);

            int voltageDirection = 0;
            if (voltageOffsets[voltageOffset] < 0)
            {
                voltageDirection = 0;
            }
            else
            {
                voltageDirection = 1;
            }
            int voltageSteps = abs(voltageOffsets[voltageOffset]);

            ASTERA_INFO("Checking offset x=%d,y=%d", timingOffsets[i], voltageOffsets[voltageOffset]);
            rc = ariesMarginPmaTimingVoltageOffset(marginDevice, port, lane, timeDirection,
                                   timeSteps, voltageDirection,
                                   voltageSteps, dwell, &errorCount);
            CHECK_SUCCESS(rc)
            eyeResults[port][lane][i][voltageOffset] = errorCount;
            fprintf(fp, "%3d,", errorCount);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fprintf(fp, "    ,,");
    for (i = 0; i < NUMTIMINGSTEPS + 1; i++)
    {
        fprintf(fp , "%d,", timingOffsets[i]);
    }
    fclose(fp);
    free(timingOffsets);
    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
