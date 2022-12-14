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
 * @file aries_link.c
 * @brief Implementation of public Link level functions for the SDK.
 */

#include "../include/aries_link.h"

#ifdef __cplusplus
extern "C" {
#endif

AriesErrorType ariesLinkDumpDebugInfo(
        AriesLinkType* link)
{
    AriesErrorType rc;

    // Capture detailed debug information
    // This function prints detailed state information to a file
    rc = ariesLinkPrintDetailedState(link, ".", "link_state_detailed");
    CHECK_SUCCESS(rc);
    // This function prints Aries LTSSM logs to a file
    rc = ariesLinkPrintMicroLogs(link, ".", "ltssm_micro_log");
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

AriesErrorType ariesLinkPrintMicroLogs(
        AriesLinkType* link,
        const char* basepath,
        const char* filename)
{
    AriesErrorType rc;
    char filepath[ARIES_PATH_MAX];
    FILE* fp;

    if (!link || !basepath || !filename)
    {
        ASTERA_ERROR("Invalid link or basepath or filename passed");
        return ARIES_INVALID_ARGUMENT;
    }

    if (strlen(basepath) == 0 || strlen(filename) == 0)
    {
        ASTERA_ERROR("Can't load a file without the basepath or filename");
        return ARIES_INVALID_ARGUMENT;
    }

    int startLane = ariesGetStartLane(link);

    // Write micro logs output to a file
    // File is printed as a python dict so that we can post-process it to a
    // readable format as log messages
    snprintf(filepath, ARIES_PATH_MAX, "%s/%s_%d.py", basepath, filename, link->config.linkId);

    fp = fopen(filepath, "w");
    if(fp == NULL)
    {
        ASTERA_ERROR("Could not open the specified filepath");
        return ARIES_INVALID_ARGUMENT;
    }

    fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
    fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n", ariesGetSDKVersion());

    fprintf(fp, "fw_version_major = %d\n", link->device->fwVersion.major);
    fprintf(fp, "fw_version_minor = %d\n", link->device->fwVersion.minor);
    fprintf(fp, "fw_version_build = %d\n", link->device->fwVersion.build);

    // Initialise logger
    rc = ariesLTSSMLoggerInit(link, 0, ARIES_LTSSM_VERBOSITY_HIGH);
    if (rc != ARIES_SUCCESS)
    {
        fprintf(fp, "# Encountered error during ariesLinkPrintMicroLogs->ariesLTSSMLoggerInit. Closing file.\n");
        fclose(fp);
        return rc;
    }

    // Enable Macros to print
    rc = ariesLTSSMLoggerPrintEn(link, 1);
    if (rc != ARIES_SUCCESS)
    {
        fprintf(fp, "# Encountered error during ariesLinkPrintMicroLogs->ariesLTSSMLoggerPrintEn. Closing file.\n");
        fclose(fp);
        return rc;
    }

    fprintf(fp, "aries_micro_logs = [\n");
    fprintf(fp, "    {\n");
    fprintf(fp, "        'log_type': %d,\n", ARIES_LTSSM_LINK_LOGGER);
    fprintf(fp, "        'log': [      # Open MM log\n");
    // Print Main micro log
    rc = ariesPrintLog(link, ARIES_LTSSM_LINK_LOGGER, fp);
    fprintf(fp, "        ],    # Close MM log\n");
    fprintf(fp, "    },\n");
    if(rc != ARIES_SUCCESS)
    {
        fprintf(fp, "# Encountered error during ariesLinkPrintMicroLogs->ariesPrintLog. Closing file.\n");
        fclose(fp);
        return rc;
    }

    // Print path micro logs
    int laneNum;
    int laneIdx;
    for (laneIdx = 0; laneIdx < link->state.width; laneIdx++)
    {
        laneNum = laneIdx + startLane;
        fprintf(fp, "    {\n");
        fprintf(fp, "        'log_type': %d,\n", laneNum);
        fprintf(fp, "        'log': [      # Open PM %d log\n", laneNum);
        rc |= ariesPrintLog(link, laneNum, fp);
        fprintf(fp, "        ],    # Close PM %d log\n", laneNum);
        fprintf(fp, "    },\n");
    }
    fprintf(fp, "]\n");

    if(rc != ARIES_SUCCESS)
    {
        fprintf(fp, "# Encountered error during ariesLinkPrintMicroLogs->ariesPrintLog. Closing file.\n");
        fclose(fp);
        return rc;
    }
    fclose(fp);

    return ARIES_SUCCESS;
}

// Capture the detailed link state and print it to file
AriesErrorType ariesLinkPrintDetailedState(
        AriesLinkType* link,
        const char* basepath,
        const char* filename)
{
    AriesErrorType rc;
    char filepath[ARIES_PATH_MAX];
    FILE *fp;

    if (!link || !basepath || !filename)
    {
        ASTERA_ERROR("Invalid link or basepath or filename passed");
        return ARIES_INVALID_ARGUMENT;
    }

    if (strlen(basepath) == 0 || strlen(filename) == 0)
    {
        ASTERA_ERROR("Can't load a file without the basepath or filename");
        return ARIES_INVALID_ARGUMENT;
    }

    // Check overall device health
    rc = ariesCheckDeviceHealth(link->device);
    CHECK_SUCCESS(rc);

    // Get Link state
    rc = ariesGetLinkStateDetailed(&link[0]);
    CHECK_SUCCESS(rc);

    int startLane = ariesGetStartLane(link);

    // Write link state detailed output to a file
    // File is printed as a python dict so that we can post-process it to a
    // readable format as a table
    snprintf(filepath, ARIES_PATH_MAX, "%s/%s_%d.py", basepath, filename, link->config.linkId);

    fp = fopen(filepath, "w");
    if(fp == NULL)
    {
        ASTERA_ERROR("Could not open the specified filepath");
        return ARIES_INVALID_ARGUMENT;
    }

    fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
    fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n", ariesGetSDKVersion());

    // Aries device struct parameters
    fprintf(fp, "aries_device = {}\n");
    int b = 0;
    for (b = 0; b < 12; b += 1)
    {
        fprintf(fp, "aries_device['chipID_%d'] = %d\n", b, link->device->chipID[b]);
    }
    for (b = 0; b < 6; b += 1)
    {
        fprintf(fp, "aries_device['lotNumber_%d'] = %d\n", b, link->device->lotNumber[b]);
    }
    fprintf(fp, "aries_device['deviceOkay'] = %s\n", link->device->deviceOkay ? "True":"False");
    fprintf(fp, "aries_device['allTimeMaxTempC'] = %f\n", link->device->maxTempC);
    fprintf(fp, "aries_device['currentTempC'] = %f\n", link->device->currentTempC);
    fprintf(fp, "aries_device['tempAlertThreshC'] = %f\n\n", link->device->tempAlertThreshC);

    // Aries link struct parameters
    fprintf(fp, "aries_link = {}\n");
    fprintf(fp, "aries_link['link_id'] = %d\n", link->config.linkId);
    fprintf(fp, "aries_link['linkOkay'] = %s\n", link->state.linkOkay ? "True":"False");
    fprintf(fp, "aries_link['state'] = %d\n", link->state.state);
    fprintf(fp, "aries_link['width'] = %d\n", link->state.width);
    fprintf(fp, "aries_link['curWidth'] = %d\n", link->state.curWidth);
    fprintf(fp, "aries_link['linkMinFoM'] = %d\n", link->state.linkMinFoM);
    fprintf(fp, "aries_link['linkMinFoMRx'] = '%s'\n", link->state.linkMinFoMRx);
    fprintf(fp, "aries_link['recoveryCount'] = %d\n", link->state.recoveryCount);
    fprintf(fp, "aries_link['uspp_speed'] = %2.1f\n", link->state.usppSpeed);
    fprintf(fp, "aries_link['dspp_speed'] = %2.1f\n", link->state.dsppSpeed);
    fprintf(fp, "aries_link['uspp'] = {}\n");
    fprintf(fp, "aries_link['uspp']['tx'] = {}\n");
    fprintf(fp, "aries_link['uspp']['rx'] = {}\n");
    fprintf(fp, "aries_link['dspp'] = {}\n");
    fprintf(fp, "aries_link['dspp']['tx'] = {}\n");
    fprintf(fp, "aries_link['dspp']['rx'] = {}\n");
    fprintf(fp, "aries_link['rt_core'] = {}\n");
    fprintf(fp, "aries_link['rt_core']['us'] = {}\n");
    fprintf(fp, "aries_link['rt_core']['ds'] = {}\n");

    int phyLaneNum;
    int laneIndex;
    for (laneIndex = 0; laneIndex < link->state.width; laneIndex++)
    {
        phyLaneNum = laneIndex + startLane;

        fprintf(fp, "\n\n### Stats for logical lane %d (physical lane %d) ###\n", laneIndex, phyLaneNum);

        fprintf(fp, "aries_link['uspp']['tx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['logical_lane'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['physical_pin'] = '%s'\n",
            laneIndex, link->state.usppState.txState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['de'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].de);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['pre'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].pre);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['cur'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].cur);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['pst'] = %f\n",
            laneIndex, link->state.usppState.txState[laneIndex].pst);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_eq_rate'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_pre_req'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastPreReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_cur_req'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastCurReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_pst_req'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastPstReq);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['uspp']['rx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['logical_lane'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['physical_pin'] = '%s'\n",
            laneIndex, link->state.usppState.rxState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['termination'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].termination);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['polarity'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].polarity);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['att_db'] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].attdB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['ctle_boost_db'] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].ctleBoostdB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['ctle_pole'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].ctlePole);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['vga_db'] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].vgadB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][1] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][2] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][3] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe3);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][4] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe4);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][5] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe5);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][6] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe6);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][7] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe7);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][8] = %f\n",
            laneIndex, link->state.usppState.rxState[laneIndex].dfe8);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_eq_rate'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFom);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_m1'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_fom_m1'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_m2'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_fom_m2'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_m3'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM3);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req_fom_m3'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM3);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['rt_core']['us'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['tj_c'] = %2.2f\n",
            laneIndex, link->state.coreState.usppTempC[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['skew_ns'] = %d\n",
            laneIndex, link->state.coreState.usDeskewNs[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['tj_c_alert'] = %d\n",
            laneIndex, link->state.coreState.usppTempAlert[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['pth_fw_state'] = %d\n",
            laneIndex, link->state.coreState.usppPathFWState[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['pth_hw_state'] = %d\n",
            laneIndex, link->state.coreState.usppPathHWState[laneIndex]);

        fprintf(fp, "aries_link['rt_core']['ds'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['tj_c'] = %2.2f\n",
            laneIndex, link->state.coreState.dsppTempC[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['skew_ns'] = %d\n",
            laneIndex, link->state.coreState.dsDeskewNs[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['tj_c_alert'] = %d\n",
            laneIndex, link->state.coreState.dsppTempAlert[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['pth_fw_state'] = %d\n",
            laneIndex, link->state.coreState.dsppPathFWState[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['pth_hw_state'] = %d\n",
            laneIndex, link->state.coreState.dsppPathHWState[laneIndex]);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['dspp']['tx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['logical_lane'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['physical_pin'] = '%s'\n",
            laneIndex, link->state.dsppState.txState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['de'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].de);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['pre'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].pre);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['cur'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].cur);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['pst'] = %f\n",
            laneIndex, link->state.dsppState.txState[laneIndex].pst);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_eq_rate'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_pre_req'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastPreReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_cur_req'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastCurReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_pst_req'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastPstReq);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['dspp']['rx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['logical_lane'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['physical_pin'] = '%s'\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['termination'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].termination);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['polarity'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].polarity);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['att_db'] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].attdB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['ctle_boost_db'] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].ctleBoostdB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['ctle_pole'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].ctlePole);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['vga_db'] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].vgadB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][1] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][2] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][3] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe3);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][4] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe4);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][5] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe5);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][6] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe6);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][7] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe7);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][8] = %f\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].dfe8);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_eq_rate'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFom);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_m1'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqM1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_fom_m1'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_m2'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqM2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_fom_m2'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_m3'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].lastPresetReqM3);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req_fom_m3'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM3);
        fprintf(fp, "\n");
    }
    fclose(fp);

    return ARIES_SUCCESS;
}


// Print the micro logger entries
AriesErrorType ariesPrintLog(
        AriesLinkType* link,
        AriesLTSSMLoggerEnumType log,
        FILE* fp)
{
    AriesLTSSMEntryType ltssmEntry;
    int offset = 0;
    int oneBatchModeEn;
    int oneBatchWrEn;
    int batchComplete;
    int currFmtID;
    int currWriteOffset;
    AriesErrorType rc;
    int full = 0;

    ltssmEntry.logType = log;

    // Buffer size different for main and path micros
    int bufferSize;
    if (log == ARIES_LTSSM_LINK_LOGGER)
    {
        bufferSize = ARIES_MM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
    }
    else
    {
        bufferSize = ARIES_PM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
    }

    // Get One batch mode enable
    rc = ariesGetLoggerOneBatchModeEn(link, log, &oneBatchModeEn);
    CHECK_SUCCESS(rc);

    // Get One batch write enable
    rc = ariesGetLoggerOneBatchWrEn(link, log, &oneBatchWrEn);
    CHECK_SUCCESS(rc);

    if (oneBatchModeEn == 0)
    {
        // Stop Micros from printing
        rc = ariesLTSSMLoggerPrintEn(link, 0);
        CHECK_SUCCESS(rc);

        // In this mode we print the logger from current write offset
        // and reset the offset to zero once we reach the end of the buffer
        // Capture current write offset
        rc = ariesGetLoggerWriteOffset(link, log, &currWriteOffset);
        CHECK_SUCCESS(rc);
        // Start offset from the current (paused) offset
        offset = currWriteOffset;

        // Print logger from current offset
        while (offset < bufferSize)
        {
            rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
            CHECK_SUCCESS(rc);
            fprintf(fp, "            {\n");
            fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
            fprintf(fp, "                'offset': %d\n", ltssmEntry.offset);
            fprintf(fp, "            },\n");
        }

        // Wrap around and continue reading the log entries
        if (currWriteOffset != 0)
        {
            // Reset offset to start from beginning
            offset = 0;

            // Print logger from start of print buffer
            while(offset < currWriteOffset)
            {
                // Read logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n", ltssmEntry.offset);
                fprintf(fp, "            },\n");
            }
        }

        // Enable Macros to print
        rc = ariesLTSSMLoggerPrintEn(link, 1);
        CHECK_SUCCESS(rc);
    }
    else
    {
        // Check if batch is complete
        batchComplete = (oneBatchModeEn == 1) && (oneBatchWrEn == 0);

        // Read format ID at current offset
        rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
        CHECK_SUCCESS(rc);

        if (batchComplete == 1)
        {
            full = 1;
            while ((currFmtID != 0) && (offset < bufferSize))
            {
                // Get logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n", ltssmEntry.offset);
                fprintf(fp, "            },\n");
                // Read Fmt ID
                rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
                CHECK_SUCCESS(rc);
            }
        }
        else
        {
            // Print from start of the buffer until the end
            while ((currFmtID != 0) && (offset < bufferSize) &&
                (offset < currWriteOffset))
            {
                // Get logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n", ltssmEntry.offset);
                fprintf(fp, "            },\n");
                // Read Fmt ID
                rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
                CHECK_SUCCESS(rc);
                // Read current write offset
                rc = ariesGetLoggerWriteOffset(link, log, &currWriteOffset);
                CHECK_SUCCESS(rc);
            }
        }
    }

    if (full == 0)
    {
        ASTERA_INFO("There is more to print ...");
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesLinkGetCurrentWidth(
        AriesLinkType* link,
        int* currentWidth)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t addressOffset;
    // Check state of each path micro in the link. If it is in FWD we add to current width
    *currentWidth = 0;

    int i;
    int startLane = ariesGetStartLane(link);
    for (i = startLane; i < link->config.maxWidth + startLane; i++)
    {
        addressOffset = ARIES_QS_0_CSR_OFFSET + (ARIES_PATH_WRAPPER_1_CSR_OFFSET * i);

        rc = ariesReadByteData(link->device->i2cDriver, addressOffset + 0xb7, dataByte);
        CHECK_SUCCESS(rc);

        if (dataByte[0] == 0x13) // FWD state
        {
            (*currentWidth)++;
        }
    }

    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
