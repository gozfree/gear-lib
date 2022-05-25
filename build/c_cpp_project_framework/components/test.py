#!/bin/env python3

comd_d = "sed -i \"s/if(CONFIG_LIBAVCAP_ENABLED)/if(CONFIG_{}_ENABLED)/\" {}"


import os
dir_l = os.listdir()

for di in dir_l:
    if di == "test.py":
        continue
    strr = di + "/CMakeLists.txt"
    os.system(comd_d.format(di.upper(), strr))


# print(dir_l)
