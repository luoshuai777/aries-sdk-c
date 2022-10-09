"""!@package print_link_state
Documentation for this module.

Script to print the link state for all the lanes in the link
"""

import os
import sys
import logging
import argparse

from os.path import dirname
from os.path import basename
from os.path import isfile
from os.path import join
import glob

# Truncated to 13 characters for clean Lnik status print
psm_state = {
   0x00: "IDLE",#          // 0x0 : Idle state
   0x01: "RESET",#         // 0x1 : Reset state
   0x02: "RESET_WT_CMPL",# // 0x2 : Reset completion state
   0x03: "DETECT",#        // 0x3 : Receiver Detect state
   0x04: "DETECT",#        // 0x4 : Receiver Detect state",# waiting for txdetectrx result (Print as "DETECT" to avoid customer confusion)
   0x05: "DET_WT_CMPL",#   // 0x5 : Receiver Detect state",# waiting for Link Master
   0x06: "WT_RX_VALID_E",# // 0x6 : Entry state for WAIT_RX_VALID
   0x07: "WT_RX_VALID",#   // 0x7 : Wait for Rx Valid to transition high
   0x08: "CHECK_RX_POL",#  // 0x8 : Check for polarity invertion in Rx data
   0x09: "RDY_TO_DSK_E",#  // 0x9 : Send to link master to notify ready to deskew
   0x0a: "RDY_TO_DSK",#    // 0xa : Wait for link master to start deskew
   0x0b: "DESKEW_E",#      // 0xb : Entry state for Deskew
   0x0c: "DESKEW",#        // 0xc : Deskew state
   0x0d: "RT_CHNG_WT_ST",# // 0xd : Wait for both pseudo ports to be ready for Rate change
   0x0e: "RT_CHNG_WT_CT",# // 0xe : Wait for a Rate change to complete
   0x0f: "RT_CHNG_INTER",# // 0xf : Wait for a Rate change to complete
   0x10: "RDY_TO_FWD_E",#  // 0x10: Send to link master to notify ready to forward
   0x11: "RDY_TO_FWD",#    // 0x11: Wait for link master to start forward
   0x12: "FWD_E",#         // 0x12: Entry state into Forwarding Mode",# training sets
   0x13: "FWD",#           // 0x13: Forwarding Mode",# Training S
   0x14: "LPBK_SLV_ENTY", #// 0x14:
   0x15: "LPBK_SLV_EXIT", #// 0x15:
   0x16: "EQ_P2ACT_E",#    // 0x16: Entry to Execution Mode",# Phase 2",# active
   0x17: "EQ_P2ACT_WTST",# // 0x17: Wait to enter execution Mode",# Phase 2",# active
   0x18: "EQ_P2ACT_WTSW",# // 0x18: Wait to enter execution Mode",# Phase 2",# active
   0x19: "EQ_P2ACT",#      // 0x19: Execution Mode",# Phase 2",# active
   0x1a: "EQ_P2PSV_E",#    // 0x1a: Execution Mode",# Phase 2",# passive
   0x1b: "EQ_P2PSV",#      // 0x1b: Execution Mode",# Phase 2",# passive
   0x1c: "EQ_P3ACT_E",#    // 0x1c: Entry to Execution Mode",# Phase 3",# active
   0x1d: "EQ_P3ACT_WT",#   // 0x1d: Wait to ener execution Mode",# Phase 3",# active
   0x1e: "EQ_P3ACT",#      // 0x1e: Execution Mode",# Phase 3",# active
   0x1f: "EQ_P3PSV",#      // 0x1f: Execution Mode",# Phase 3",# passive
   0x20: "EQ_WT_ENT_FWD",# // 0x20: Wait for link master's notification to enter forward mode
   0x21: "EQ_CMPL_FWD_E",# // 0x21: Execution to Forwarding Mode transition
   0x22: "EQ_CMPL_FWD",#   // 0x22: Forwarding mode after Execution (place-holder",# may not be needed)
   0x23: "HOT_RESET",#     // 0x23: Hot Reset",# Section 4.3.6.11
   0x24: "DISABLE_LINK",#  // 0x24: Disable Link",# Section 4.3.6.12
   0x25: "LOOPBACK_FWD",#  // 0x25: Forwarding Loopback
   0x26: "LOOPBACK_SLV",#  // 0x26: Slave Loopback
   0x27: "ERROR",#         // Error state
}


# Truncated to 13 character limit for clean Link status print
pth_hw_state = {
   0x00: "RESET",#         // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_RESET
   0x01: "DSK_MRK_DET",#   // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_DESKEW_MARKER_DETECT
   0x02: "DESKEW",#        // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_DESKEW
   0x03: "EXEC_P2ACT",#    // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_EQ_P2ACTIVE
   0x04: "EXEC_P2PSV",#    // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_EQ_P2PASSIVE
   0x05: "EXEC_P3ACT",#    // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_EQ_P3ACTIVE
   0x06: "EXEC_P3PSV",#    // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_EQ_P3PASSIVE
   0x07: "EXEC_CMPL_FWD",# // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_COMPLETE_THEN_FORWARD
   0x08: "FWD_BYP",#       // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_FORWARD_BYPASS
   0x09: "COMPLIANCE",#    // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_COMPLIANCE
   0x0a: "LOOPBACK",#      // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_EXEC_LOOPBACK
   0x0b: "FWD",#           // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_FORWARD
   0x0c: "DEBUG",#         // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_DEBUG
   0x0d: "REDESKEW",#      // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_REDESKEW
   0x0f: "DEAD",#          // AL_RET_PATH_GBL_CSR_PT_NEXT_STATE_ENUM_RET_SM_DEAD
}


def print_row(values, widths, print_width):
    """!@brief Print a specific row of the link state table

    @param values column data as a list
    @param widths width per column type
    @param print_width width marking num lanes in link row per path
    """

    val_col = 0
    width_col = 0
    format_str = f'{values[val_col]:>{widths[width_col]}}|'
    val_col += 1
    width_col += 1
    format_str += f'{values[val_col]:>{widths[width_col]}}:'
    val_col += 1
    width_col += 1
    for idx in range(print_width):
        format_str += f'{values[val_col+idx]:^{widths[width_col]}}|'
    val_col += print_width
    width_col += 1
    format_str += f'{values[val_col]:>{widths[width_col]}}:'

    val_col += 1
    width_col += 1
    for idx in range(print_width):
        format_str += f'{values[val_col+idx]:^{widths[width_col]}}|'
    logger.info(format_str)


def print_status(link):
    """!@brief Print out link detailed stats as a table

    @param link link stats (as a dict)
    """

    title_column_width = 7
    subtitle_column_width = 15
    value_column_width = 13
    num_pp_value_rows = 26

    # Print Link # and Link Width rows
    title = 'LINK #'
    width = title_column_width
    value = link['link_id']
    logger.info(f'{title:>{width}}: {value}')

    title = 'WIDTH'
    logger.info(f'{title:>{width}}:')
    title = 'EXPECT'
    width = title_column_width
    value = link['width']
    logger.info(f'{title:>{width}}: {value}')
    title = 'CURRENT'
    width = title_column_width
    value = link['curWidth']
    logger.info(f'{title:>{width}}: {value}')

    if link['width'] == 16:
        num_passes = 2
        start_lane = link['width']-1
        end_lane = int(link['width']/2)
        print_width = int(link['width']/2)
    else:
        num_passes = 1
        start_lane = link['width']-1
        end_lane = 0
        print_width = link['width']

    total_width = (title_column_width+1) + (2*(subtitle_column_width+1)) + (print_width*2*(value_column_width+1))

    row_separator = total_width * '-'

    for pass_id in range(num_passes):
        values = []
        widths = []
        logger.info(row_separator)
        widths = [title_column_width, subtitle_column_width, print_width*(value_column_width+1)-1, subtitle_column_width, print_width*(value_column_width+1)-1]
        values = ["", "PATH", "UPSTREAM PATH", "PATH", "DOWNSTREAM PATH"]
        logger.info(f'{values[0]:>{widths[0]}}|{values[1]:>{widths[1]}}:{values[2]:^{widths[2]}}|{values[3]:>{widths[3]}}:{values[4]:^{widths[4]}}|')

        uspp_tx_parameters = link['uspp']['tx']
        uspp_rx_parameters = link['uspp']['rx']
        dspp_tx_parameters = link['dspp']['tx']
        dspp_rx_parameters = link['dspp']['rx']
        rt_core_us = link['rt_core']['us']
        rt_core_ds = link['rt_core']['ds']

        # Print uspp and dspp speed
        uspp_speed = link['uspp_speed']
        dspp_speed = link['dspp_speed']
        widths = [title_column_width, subtitle_column_width, print_width*(value_column_width+1)-1, subtitle_column_width, print_width*(value_column_width+1)-1]
        values = ["", "CURRENT SPEED", "%4.1f GT/s" % uspp_speed, "CURRENT SPEED", "%4.1f GT/s" % dspp_speed]
        logger.info(f'{values[0]:>{widths[0]}}|{values[1]:>{widths[1]}}:{values[2]:^{widths[2]}}|{values[3]:>{widths[3]}}:{values[4]:^{widths[4]}}|')

        # Print 'logical lane #' row
        widths = [title_column_width, subtitle_column_width, value_column_width, subtitle_column_width, value_column_width]
        values = [""]
        values.append("LOGICAL LANE #")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            lane_num = uspp_tx_parameters[lane]['logical_lane']
            values.append('PAD' if lane_num==0xf7 else lane_num)
        values.append("LOGICAL LANE #")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            lane_num = dspp_tx_parameters[lane]['logical_lane']
            values.append('PAD' if lane_num==0xf7 else lane_num)
        print_row(values=values, widths=widths, print_width=print_width)

        #XXX TODO PHYSICAL PATH
        logger.info(row_separator)

        # Print 'Physical pin info' row
        widths = [title_column_width, subtitle_column_width, value_column_width, subtitle_column_width, value_column_width]
        values = ["USPP"]
        values.append("PHYSICAL PIN #")
        for lane in range(start_lane, end_lane-1, -1):
            lane_num = uspp_tx_parameters[lane]['physical_pin']
            values.append('PAD' if lane_num==0xf7 else lane_num)
        values.append("PHYSICAL PIN #")
        for lane in range(start_lane, end_lane-1, -1):
            lane_num = uspp_rx_parameters[lane]['physical_pin']
            values.append('PAD' if lane_num==0xf7 else lane_num)
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX PRE and RX TERM
        values = [""]
        values.append("TX CURRENT PRE")
        for lane in range(start_lane, end_lane-1, -1):
            if ((uspp_speed==2.5) or (uspp_speed==5.0)):
                values.append('-')
            else:
                val = str(uspp_tx_parameters[lane]['pre'])
                values.append(val)
        values.append("RX TERM")
        for lane in range(start_lane, end_lane-1, -1):
            val = uspp_rx_parameters[lane]['termination']
            if val == 1:
                values.append("ON")
            else:
                values.append("OFF")
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX CUR and RX Polarity
        values = [""]
        values.append("TX CURRENT CUR")
        for lane in range(start_lane, end_lane-1, -1):
            if ((uspp_speed==2.5) or (uspp_speed==5.0)):
                values.append('-')
            else:
                val = str(uspp_tx_parameters[lane]['cur'])
                values.append(val)
        values.append("RX POLARITY")
        for lane in range(start_lane, end_lane-1, -1):
            code = uspp_rx_parameters[lane]['polarity']
            if code == 1:
                values.append("INVERTED")
            else:
                values.append("NORMAL")
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX PST and RX ATT
        values = [""]
        values.append("TX CURRENT PST")
        for lane in range(start_lane, end_lane-1, -1):
            val = "%d" %(uspp_tx_parameters[lane]['pst'])
            values.append(val)
        values.append("RX ATT")
        for lane in range(start_lane, end_lane-1, -1):
            value_db = uspp_rx_parameters[lane]['att_db']
            values.append("%4.1f dB" % (value_db))
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX CTLE BOOST
        values = [""]
        values.append("")
        for lane in range(start_lane, end_lane-1, -1):
            values.append("")
        values.append("RX CTLE BOOST")
        for lane in range(start_lane, end_lane-1, -1):
            boost_value_db = uspp_rx_parameters[lane]['ctle_boost_db']
            values.append("%4.1f dB" % (boost_value_db))
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX CTLE POLE
        values = [""]
        values.append("")
        for lane in range(start_lane, end_lane-1, -1):
            values.append('')
        values.append("RX CTLE POLE")
        for lane in range(start_lane, end_lane-1, -1):
            ctle_pole = uspp_rx_parameters[lane]['ctle_pole']
            values.append(ctle_pole)
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX VGA
        values = [""]
        values.append("")
        for lane in range(start_lane, end_lane-1, -1):
            values.append("")
        values.append("RX VGA")
        for lane in range(start_lane, end_lane-1, -1):
            vga_value_db = uspp_rx_parameters[lane]['vga_db']
            values.append("%3.1f dB" % (vga_value_db))
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX DFE values (1 - 8)
        for dfe in range(1, 9):
            values = [""]
            values.append("")
            for lane in range(start_lane, end_lane-1, -1):
                values.append("")
            values.append("RX DFE%d" % dfe)
            for lane in range(start_lane, end_lane-1, -1):
                dfe_code = "%2.1f mV" % (uspp_rx_parameters[lane]['dfe'][dfe])
                values.append(dfe_code)
            print_row(values=values, widths=widths, print_width=print_width)

        if uspp_tx_parameters[lane]['last_eq_rate'] >= 3:
            # Print last eq speed
            values = [""]
            values.append("LAST EQ SPEED")
            for lane in range(start_lane, end_lane-1, -1):
                speed = "GEN-%d" % (uspp_tx_parameters[lane]['last_eq_rate'])
                values.append(speed)
            values.append("LAST EQ SPEED")
            for lane in range(start_lane, end_lane-1, -1):
                speed = "GEN-%d" % (uspp_rx_parameters[lane]['last_eq_rate'])
                values.append(speed)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 0
            # If last preset req is 0xf, dont display it
            # If last preset req is not 0xf, dont display pre, cur, pst req values
            values = [""]
            last_preset_req_vals = {}
            values.append("LAST PRESET REQ")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = uspp_tx_parameters[lane]['last_preset_req']
                last_preset_req_vals[lane] = preset_req
                if preset_req == 0xf:
                    values.append("-")
                else:
                    values.append(preset_req)
            values.append("PRESET REQ 0")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (uspp_rx_parameters[lane]['last_preset_req_m3'], uspp_rx_parameters[lane]['last_preset_req_fom_m3'])
                values.append(preset_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 0
            values = [""]
            values.append("LAST PRE REQ")
            for lane in range(start_lane, end_lane-1, -1):
                if last_preset_req_vals[lane] != 0xf:
                    pre_req = '-'
                else:
                    pre_req = str(uspp_tx_parameters[lane]['last_pre_req'])
                values.append(pre_req)
            values.append("PRESET REQ 1")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (uspp_rx_parameters[lane]['last_preset_req_m2'], uspp_rx_parameters[lane]['last_preset_req_fom_m2'])
                values.append(preset_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 0
            values = [""]
            values.append("LAST CUR REQ")
            for lane in range(start_lane, end_lane-1, -1):
                if last_preset_req_vals[lane] != 0xf:
                    cur_req = '-'
                else:
                    cur_req = str(uspp_tx_parameters[lane]['last_cur_req'])
                values.append(cur_req)
            values.append("PRESET REQ 2")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (uspp_rx_parameters[lane]['last_preset_req_m1'], uspp_rx_parameters[lane]['last_preset_req_fom_m1'])
                values.append(preset_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 0
            values = [""]
            values.append("LAST PST REQ")
            for lane in range(start_lane, end_lane-1, -1):
                if last_preset_req_vals[lane] != 0xf:
                    pst_req = '-'
                else:
                    pst_req = str(uspp_tx_parameters[lane]['last_pst_req'])
                values.append(pst_req)
            values.append("PRESET REQ 3")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (uspp_rx_parameters[lane]['last_preset_req'], uspp_rx_parameters[lane]['last_preset_req_fom'])
                values.append(preset_req)
            print_row(values=values, widths=widths, print_width=print_width)

        logger.info(row_separator)

        # RT Core Parameters
        values = ["RT CORE"]
        values.append("TEMP")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = "%2d C" % (int(rt_core_us[lane]['tj_c']))
            values.append(tmp)
        values.append("TEMP")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = "%2d C" % (int(rt_core_us[lane]['tj_c']))
            values.append(tmp)
        print_row(values=values, widths=widths, print_width=print_width)

        values = [""]
        values.append("DESKEW")
        for lane in range(start_lane, end_lane-1, -1):
            skew = "%d ns" % (rt_core_us[lane]['skew_ns'])
            values.append(skew)
        values.append("DESKEW")
        for lane in range(start_lane, end_lane-1, -1):
            skew = "%d ns" % (rt_core_ds[lane]['skew_ns'])
            values.append(skew)
        print_row(values=values, widths=widths, print_width=print_width)

        values = [""]
        values.append("PATH HW STATE")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = int(rt_core_ds[lane]['pth_hw_state'])
            values.append(pth_hw_state[tmp])
        values.append("PATH HW STATE")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = int(rt_core_us[lane]['pth_hw_state'])
            values.append(pth_hw_state[tmp])
        print_row(values=values, widths=widths, print_width=print_width)

        values = [""]
        values.append("PATH FW STATE")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = int(rt_core_ds[lane]['pth_fw_state'])
            values.append(psm_state[tmp])
        values.append("PATH FW STATE")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = int(rt_core_us[lane]['pth_fw_state'])
            values.append(psm_state[tmp])
        print_row(values=values, widths=widths, print_width=print_width)

        values = [""]
        values.append("TEMP")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = "%2d C" % (int(rt_core_ds[lane]['tj_c']))
            values.append(tmp)
        values.append("TEMP")
        for lane in range(start_lane, end_lane-1, -1):
            tmp = "%2d C" % (int(rt_core_ds[lane]['tj_c']))
            values.append(tmp)
        print_row(values=values, widths=widths, print_width=print_width)

        logger.info(row_separator)

        # DSPP parameters
        values = ["DSPP"]
        values.append("PHYSICAL PIN #")
        for lane in range(start_lane, end_lane-1, -1):
            values.append(dspp_rx_parameters[lane]['physical_pin'])
        values.append("PHYSICAL PIN #")
        for lane in range(start_lane, end_lane-1, -1):
            values.append(dspp_tx_parameters[lane]['physical_pin'])
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX PRE and RX TERM
        values = [""]
        values.append("RX TERM")
        for lane in range(start_lane, end_lane-1, -1):
            term = dspp_rx_parameters[lane]['termination']
            if term == 1:
                values.append("ON")
            else:
                values.append("OFF")
        values.append("TX CURRENT PRE")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            if ((dspp_speed==2.5) or (dspp_speed==5.0)):
                values.append('-')
            else:
                pre = str(dspp_tx_parameters[lane]['pre'])
                values.append(pre)
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX CUR and RX Polarity
        values = [""]
        values.append("RX POLARITY")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            code = dspp_rx_parameters[lane]['polarity']
            if code == 1:
                values.append("INVERTED")
            else:
                values.append("NORMAL")
        values.append("TX CURRENT CUR")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            if ((dspp_speed==2.5) or (dspp_speed==5.0)):
                values.append('-')
            else:
                cur = str(dspp_tx_parameters[lane]['cur'])
                values.append(cur)
        print_row(values=values, widths=widths, print_width=print_width)

        # Print TX PST and RX ATT
        values = [""]
        values.append("RX ATT")
        for lane in range(start_lane, end_lane-1, -1):
            value_db = dspp_rx_parameters[lane]['att_db']
            values.append("%4.1f dB" % (value_db))
        values.append("TX CURRENT PST")
        for lane in range(start_lane, end_lane-1, -1):
            val = "%d" %(dspp_tx_parameters[lane]['pst'])
            values.append(val)
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX CTLE BOOST
        values = [""]
        values.append("RX CTLE BOOST")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            boost_value_db = dspp_rx_parameters[lane]['ctle_boost_db']
            values.append("%4.1f dB" % (boost_value_db))
        values.append("")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            values.append('')
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX CTLE POLE
        values = [""]
        values.append("RX CTLE POLE")
        for lane in range(start_lane, end_lane-1, -1):
            ctle_pole = dspp_rx_parameters[lane]['ctle_pole']
            values.append(ctle_pole)
        values.append("")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            values.append('')
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX VGA
        values = [""]
        values.append("RX VGA")
        for lane in range(start_lane, end_lane-1, -1):
            vga_value_db = dspp_rx_parameters[lane]['vga_db']
            values.append("%3.1f dB" % (vga_value_db))
        values.append("")
        for lane in range(start_lane, end_lane-1, -1): # Print highest numbered lane first
            values.append('')
        print_row(values=values, widths=widths, print_width=print_width)

        # Print RX DFE values (1 - 8)
        for dfe in range(1, 9):
            values = [""]
            values.append("RX DFE%d" % dfe)
            for lane in range(start_lane, end_lane-1, -1):
                dfe_code = "%2.1f mV" % (dspp_rx_parameters[lane]['dfe'][dfe])
                values.append(dfe_code)
            values.append("")
            for lane in range(start_lane, end_lane-1, -1):
                values.append("")
            print_row(values=values, widths=widths, print_width=print_width)

        if dspp_tx_parameters[lane]['last_eq_rate'] >= 3:
            # Print last eq speed
            values = [""]
            values.append("LAST EQ SPEED")
            for lane in range(start_lane, end_lane-1, -1):
                speed = "GEN-%d" % (dspp_rx_parameters[lane]['last_eq_rate'])
                values.append(speed)
            values.append("LAST EQ SPEED")
            for lane in range(start_lane, end_lane-1, -1):
                speed = "GEN-%d" % (dspp_tx_parameters[lane]['last_eq_rate'])
                values.append(speed)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print preset req, and preset req 0
            # If last preset req is 0xf, dont display it
            # If last preset req is not 0xf, dont display pre, cur, pst req values
            values = [""]
            last_preset_req_vals = {}
            values.append("PRESET REQ 0")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (dspp_rx_parameters[lane]['last_preset_req_m3'], dspp_rx_parameters[lane]['last_preset_req_fom_m3'])
                values.append(preset_req)
            values.append("LAST PRESET REQ")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = dspp_tx_parameters[lane]['last_preset_req']
                last_preset_req_vals[lane] = preset_req
                if preset_req == 0xf:
                    values.append('-')
                else:
                    values.append(preset_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 1
            values = [""]
            values.append("PRESET REQ 1")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (dspp_rx_parameters[lane]['last_preset_req_m2'], dspp_rx_parameters[lane]['last_preset_req_fom_m2'])
                values.append(preset_req)
            values.append("LAST PRE REQ")
            for lane in range(start_lane, end_lane-1, -1):
                pre_req = str(dspp_tx_parameters[lane]['last_pre_req'])
                if last_preset_req_vals[lane] != 0xf:
                    values.append('-')
                else:
                    values.append(pre_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 2
            values = [""]
            values.append("PRESET REQ 2")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (dspp_rx_parameters[lane]['last_preset_req_m1'], dspp_rx_parameters[lane]['last_preset_req_fom_m1'])
                values.append(preset_req)
            values.append("LAST CUR REQ")
            for lane in range(start_lane, end_lane-1, -1):
                cur_req = str(dspp_tx_parameters[lane]['last_cur_req'])
                if last_preset_req_vals[lane] != 0xf:
                    values.append('-')
                else:
                    values.append(cur_req)
            print_row(values=values, widths=widths, print_width=print_width)

            # Print pre req, and preset req 3
            values = [""]
            values.append("PRESET REQ 3")
            for lane in range(start_lane, end_lane-1, -1):
                preset_req = "%d (FOM=0x%02x)" % (dspp_rx_parameters[lane]['last_preset_req'], dspp_rx_parameters[lane]['last_preset_req_fom'])
                values.append(preset_req)
            values.append("LAST PST REQ")
            for lane in range(start_lane, end_lane-1, -1):
                pst_req = str(dspp_tx_parameters[lane]['last_pst_req'])
                if last_preset_req_vals[lane] != 0xf:
                    values.append('-')
                else:
                    values.append(pst_req)
            print_row(values=values, widths=widths, print_width=print_width)
        logger.info(row_separator)

        # Update start and end lane pointer
        start_lane = end_lane - 1
        end_lane = 0

# Get folder name from arg to script
# The filename is link_state_detailed_log.py
# Parser object to capture all arguments
parser = argparse.ArgumentParser(description="generate link stats (detailed) table")
parser.add_argument('-f', '--folder', dest='folder', default='link_state_logs',
    help='Folder where log file(s) resides')
args = parser.parse_args()

sys.path.append(args.folder)

# List of link state log files in specified directories
files = []
for filename in glob.glob(join(args.folder, "*.py")):
    if isfile(filename):
        if filename.endswith("__init__.py"):
            pass
        else:
            files.append(basename(filename)[:-3])

# Link state modules from the log files
modules = map(__import__, files)

# Create seperate stat files for each table
for module in modules:

    # Stat info from the log file
    aries_link = module.aries_link

    # Input module filename
    fname = module.__file__
    # Output log file name
    name = fname.split("\\")[-1][:-3]
    # Initialise logger module
    logger = logging.getLogger(name)
    logger.setLevel(logging.INFO)

    # Format to print log messages in
    formatter = logging.Formatter('%(message)s')

    # Remove old log
    if os.path.exists(name + '.log'):
        os.remove(name + '.log')

    # Logger file handler
    file_handler = logging.FileHandler(name + '.log')
    file_handler.setFormatter(formatter)
    file_handler.setLevel(logging.INFO)

    # Logger stream handler
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    stream_handler.setLevel(logging.INFO)

    logger.addHandler(file_handler)
    logger.addHandler(stream_handler)

    # Print the detailed link state
    print_status(aries_link)
