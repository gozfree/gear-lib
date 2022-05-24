#!/bin/env python3
import os 
import sys




def platform_on_win():
    pass


def platform_on_linux():
    dir_list = os.listdir("components")
    for dir_s in dir_list:
        cmd_s = "ln -s {} {}"
        dir_1 = "./components/" + dir_s
        dir_2 = "../../gear-lib/" + dir_s
        os.system(cmd_s.format(dir_1, dir_2))

if __name__ == "__main__":
    if sys.platform == "linux":
        platform_on_linux()
    if sys.platform == "win32":
        platform_on_win()


