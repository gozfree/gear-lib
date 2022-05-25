#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
# @license Apache 2.0
#

import argparse
import os, sys, time, re, shutil
import subprocess
from multiprocessing import cpu_count
import json


parser = argparse.ArgumentParser(add_help=False, prog="flash.py")

############################### Add option here #############################
parser.add_argument("-p", "--port", help="flash device port", default="")
parser.add_argument("-b", "--baudrate", type=int, help="flash baudrate", default=115200)
parser.add_argument("-t", "--terminal", help="open terminal after flash ok", default=False, action="store_true")

dict_arg = {"port":"", 
            "baudrate": 115200
            }

dict_arg_not_save = ["terminal"]
#############################################################################

# use project_args created by SDK_PATH/tools/cmake/project.py

# args = parser.parse_args()
if __name__ == '__main__':
    firmware = ""
    try:
        flash_conf_path = project_path+"/.flash.conf.json"
        if project_args.cmd == "clean_conf":
            if os.path.exists(flash_conf_path):
                os.remove(flash_conf_path)
            exit(0)
        if project_args.cmd != "flash":
            print("call flash.py error")
            exit(1)
    except Exception:
        print("-- call flash.py directly!")
        parser.add_argument("firmware", help="firmware file name")
        project_parser = parser
        project_args = project_parser.parse_args()
        project_path = ""
        if not os.path.exists(project_args.firmware):
            print("firmware not found:{}".format(project_args.firmware))
            exit(1)
        firmware = project_args.firmware
        sdk_path = ""

    config_old = {}
    # load flash config from file
    try:
        with open(flash_conf_path, "r") as f:
            config_old = json.load(f)
    except Exception as e:
        pass
    # update flash config from args
    for key in dict_arg.keys():
        dict_arg[key] = getattr(project_args, key)
    # check if config update, if do, use new and update config file
    config = {}
    for key in config_old:
        config[key] = config_old[key]
    for key in dict_arg.keys():
        if dict_arg[key] != project_parser.get_default(key): # arg valid, update config
            config[key] = dict_arg[key]
        else:
            if not key in config:
                config[key] = dict_arg[key]
    if config != config_old:
        print("-- flash config changed, update at {}".format(flash_conf_path))
        with open(flash_conf_path, "w+") as f:
            json.dump(config, f, indent=4)

    # mask options that not read from file
    for key in config:
        if key in dict_arg_not_save:
            config[key] = dict_arg[key]

    print("== flash start ==")
    ############## Add flash command here ################
    print("!!! please write flash ops here ...")
    print("-- flash port    :{}".format(config["port"]))
    print("-- flash baudrate:{}".format(config["baudrate"]))
    print("project path:{}".format(project_path))

    if config["port"] == "":
        print("[ERROR] Invalid port:{}, set by -p or --port, e.g. -p /dev/ttyUSB0".format(config["port"]))
        exit(1)

    ######################################################
    print("== flash end ==")

