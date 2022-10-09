"""!@package print_link_logs
Documentation for this module.

Script to print the Main and Path logs for all lanes of a given link
"""

import sys
import os
import logging
import argparse

from os.path import dirname
from os.path import basename
from os.path import isfile
from os.path import join
import glob

# Get folder name from arg to script
# Parser object to parse arguments to this script
# The filename is ltssm_log_<link_num>.py
parser = argparse.ArgumentParser(description="generate link stats (detailed) table")
parser.add_argument('-f', '--folder', dest='folder', default='link_micro_logs',
    help='Folder where log file(s) resides')
args = parser.parse_args()

sys.path.append(args.folder)

from ALMainFmtIdDict import main_fmt_id_dict
from ALMainFmtIdDict import main_fmt_id_dict_fw_version_major
from ALMainFmtIdDict import main_fmt_id_dict_fw_version_minor
from ALMainFmtIdDict import main_fmt_id_dict_fw_build_number
from ALPathFmtIdDict import path_fmt_id_dict
from ALPathFmtIdDict import path_fmt_id_dict_fw_version_major
from ALPathFmtIdDict import path_fmt_id_dict_fw_version_minor
from ALPathFmtIdDict import path_fmt_id_dict_fw_build_number

# List of log files generated as input to the script
# Capture all log files from specified directory
files = []
for filename in glob.glob(join(args.folder, "*.py")):
    if isfile(filename):
        if filename.endswith("__init__.py"):
            pass
        elif filename.endswith("ALMainFmtIdDict.py"):
            pass
        elif filename.endswith("ALPathFmtIdDict.py"):
            pass
        else:
            files.append(basename(filename)[:-3])

# List of Python modules from the input log files
modules = map(__import__, files)

for module in modules:

    # FW version (major)
    fw_version_major = module.fw_version_major
    # FW version (minor)
    fw_version_minor = module.fw_version_minor
    # FW version (build)
    fw_version_build = module.fw_version_build

    # Log module from input log file
    aries_micro_logs = module.aries_micro_logs

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

    # Check for FW version mismatch
    # Compare FW versions to match main fmt ID files
    if (fw_version_major != main_fmt_id_dict_fw_version_major) or \
        (fw_version_minor != main_fmt_id_dict_fw_version_minor) or \
        (fw_version_build != main_fmt_id_dict_fw_build_number):
        logger.info("Log catured running FW version: %d.%d.%d" % (fw_version_major, fw_version_minor, fw_version_build))
        logger.info("Main Fmt dictionary version: %d.%d.%d" % (main_fmt_id_dict_fw_version_major,
            main_fmt_id_dict_fw_version_minor, main_fmt_id_dict_fw_build_number))
        logger.error("FW version mismatch between log and main_fmt_dict")
        logger.error("Please copy the ALMainFmtIdDict and ALPathFmtIdDict python dictionary files included in the FW release to the link_micro_logs folder")
        sys.exit(1)

    # Compare FW versions to match path fmt ID files
    if (fw_version_major != path_fmt_id_dict_fw_version_major) or \
        (fw_version_minor != path_fmt_id_dict_fw_version_minor) or \
        (fw_version_build != path_fmt_id_dict_fw_build_number):
        logger.info("Log catured running FW version: %d.%d.%d" % (fw_version_major, fw_version_minor, fw_version_build))
        logger.info("Path Fmt dictionary version: %d.%d.%d" % (path_fmt_id_dict_fw_version_major,
            path_fmt_id_dict_fw_version_minor, path_fmt_id_dict_fw_build_number))
        logger.error("FW version mismatch between log and path_fmt_dict")
        logger.error("Please copy the ALMainFmtIdDict and ALPathFmtIdDict python dictionary files included in the FW release to the link_micro_logs folder")
        sys.exit(1)

    for log in aries_micro_logs:
        # Type of log (Main or Path micro)
        log_type = log['log_type']
        # Logs for this micro
        this_micro_log = log['log']

        if log_type == 0xff:
            logger.info("==== Main Micro log ====\n\n")
        else:
            logger.info("==== Path Micro %d log ====\n\n" % (log_type))

        len_micro_log = len(this_micro_log)

        log_idx = 0
        reset_flag = False
        while log_idx < len_micro_log:
            # Check Fmt ID before processing
            log_info = this_micro_log[log_idx]
            fmt_id = log_info['data']
            if reset_flag and fmt_id == 0:
                break
            elif fmt_id == 0:
                pass
            else:
                if log_type == 0xff:
                    try:
                        msg = main_fmt_id_dict[fmt_id]
                    except KeyError:
                        log_idx = log_idx + 1
                        continue
                else:
                    try:
                        msg = path_fmt_id_dict[fmt_id]
                    except KeyError:
                        log_idx = log_idx + 1
                        continue

                num_vars = msg.count("%")
                is_time_var = False
                if "%x ms" in msg:
                    is_time_var = True

                if this_micro_log[log_idx]['offset'] == 0:
                    reset_flag = True

                msg = msg.replace("%x ms", "%6.1f ms")
                msg = msg.replace("%c", "%04x")
                msg = msg.replace("%x", "%04x")

                if log_type == 0xff:
                    log_idx = log_idx + 1
                    link_path_id = this_micro_log[log_idx]['data']
                    link_id = (link_path_id >> 4) & 0x0f
                    path_id = link_path_id & 0x0f

                    if link_path_id == 0xff:
                        link_id = -1  # This entry has no link number
                        path_id = -1  # This entry has no path number
                        if is_time_var:
                            msg = "              :     " + msg
                        else:
                            msg = "              :            " + msg
                    elif ((link_path_id & 0x0f) == 0x0f):
                        path_id = -1 # This entry has no path number
                        if is_time_var:
                            msg = ("Link %d        : " % (link_id)) + msg
                        else:
                            msg = ("Link %d        :          : " % (link_id)) + msg
                    else:
                        if is_time_var:
                            msg =  ("Link %d  Path %d: " % (link_id, path_id)) + msg
                        else:
                            msg =  ("Link %d  Path %d:          : " % (link_id, path_id)) + msg

                    vars = []
                    if num_vars > 0:
                        for var_i in range(0, num_vars):
                            try:
                                log_idx = log_idx + 1
                                var1 = this_micro_log[log_idx]['data']
                                log_idx = log_idx + 1
                                var0 = this_micro_log[log_idx]['data']
                                var = (var1 << 8) + var0
                            except:
                                var = 0

                            if (var_i == 0) and is_time_var:
                                var = var * 0.065536
                            vars.append(var)

                    # import pdb; pdb.set_trace()
                    if num_vars == 0:
                        logger.info(msg)
                    elif num_vars == 1:
                        fmsg = msg % (vars[0])
                        logger.info(fmsg)
                    elif num_vars == 2:
                        fmsg = msg % (vars[0], vars[1])
                        logger.info(fmsg)
                    elif num_vars == 3:
                        fmsg = msg % (vars[0], vars[1], vars[2])
                        logger.info(fmsg)
                    elif num_vars == 4:
                        fmsg = msg % (vars[0], vars[1], vars[2], vars[3])
                        logger.info(fmsg)
                    elif num_vars == 5:
                        fmsg = msg % (vars[0], vars[1], vars[2], vars[3], vars[4])
                        logger.info(fmsg)

                else:  # Path Micros
                    vars = []
                    if not is_time_var:
                        msg = "         : " + msg
                    if num_vars > 0:
                        for var_i in range(0, num_vars):
                            try:
                                log_idx = log_idx + 1
                                var1 = this_micro_log[log_idx]['data']
                                log_idx = log_idx + 1
                                var0 = this_micro_log[log_idx]['data']
                                var = (var1 << 8) + var0
                            except:
                                var = 0

                            if (var_i == 0) and is_time_var:
                                var = var * 0.065536
                            vars.append(var)

                    if num_vars == 0:
                        logger.info(msg)
                    elif num_vars == 1:
                        fmsg = msg % (vars[0])
                        logger.info(fmsg)
                    elif num_vars == 2:
                        fmsg = msg % (vars[0], vars[1])
                        logger.info(fmsg)
                    elif num_vars == 3:
                        fmsg = msg % (vars[0], vars[1], vars[2])
                        logger.info(fmsg)
                    elif num_vars == 4:
                        fmsg = msg % (vars[0], vars[1], vars[2], vars[3])
                        logger.info(fmsg)
                    elif num_vars == 5:
                        fmsg = msg % (vars[0], vars[1], vars[2], vars[3], vars[4])
                        logger.info(fmsg)

            log_idx = log_idx + 1
        logger.info("\n\n")
