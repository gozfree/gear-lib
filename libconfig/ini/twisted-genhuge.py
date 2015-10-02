# -*- coding: utf-8 -*-
import os
import sys

if __name__=="__main__":
    f=open('twisted-massive.ini', 'w')
    for i in range(100):
        f.write('[%03d]\n' % i)
        for j in range(100):
            f.write('key-%03d=1;\n' % j)
    f.close()

