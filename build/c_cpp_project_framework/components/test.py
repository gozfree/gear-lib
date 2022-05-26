#!/bin/env python3

# comd_d = "sed -i \"s/if(CONFIG_LIBAVCAP_ENABLED)/if(CONFIG_{}_ENABLED)/\" {}"

# cmd = "cd {};ln -s ../../../../gear-lib/{} ./{}_linux"
# cmd = "mklink /D .\{}\{}_win ..\..\..\..\gear-lib\{}"

# cmdd = "sed -i \"1a if(WIN32)\\n\ \ \ \ set(MODULE_DIR_C\ \\\"{}_win\\\")\\nelseif(APPLE)\\n\ \ \ \ set(MODULE_DIR_C\ \\\"{}_apple\\\")\\nelseif(UNIX)\\n\ \ \ \ set(MODULE_DIR_C\ \\\"{}_linux\\\")\\nendif()\" {}/CMakeLists.txt"

cmdd = "cd {};sed -i \"1a set(MODULE_DIR_C \\\"../../../../gear-lib/{}\\\")\" CMakeLists.txt"

import os
dir_l = os.listdir()

for di in dir_l:
    if di == "test.py":
        continue
    # strr = di + "/CMakeLists.txt"
    file_nl = os.listdir(di)
    # print(file_nl)
    for file_n in file_nl:
        # if file_n == "{}_linux".format(di):
        #     continue
        # if file_n == "{}_win".format(di):
        #     continue
        if file_n == "CMakeLists.txt":
            continue
        if file_n == "Kconfig":
            continue
        os.system("rm {}/{} -r".format(di, file_n))
    # print(cmdd.format(di, di,di, di))
    # os.system(cmdd.format(di, di,di,di))
    # os.system(cmdd.format(di,di))


# print(dir_l)
