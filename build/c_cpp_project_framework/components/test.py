#!/bin/env python3

# comd_d = "sed -i \"s/if(CONFIG_LIBAVCAP_ENABLED)/if(CONFIG_{}_ENABLED)/\" {}"

cmd = "mklink /D .\{}\{}_win ..\..\..\..\gear-lib\{}"


import os
dir_l = os.listdir()

for di in dir_l:
    if di == "test.py":
        continue
    # strr = di + "/CMakeLists.txt"
    print(cmd.format(di, di, di))
    os.system(cmd.format(di, di, di))


# print(dir_l)
