#!/usr/bin/env python3.6

# import torch
# from torch.autograd import Function
# from torch import Tensor

import sys
# sys.path.append('/media/rosu/Data/phd/c_ws/devel/lib/') #contains the modules of pycom
sys.path.append('/media/rosu/Data/phd/c_ws/src/easy_pbr/build') #contains the modules of pycom
# from DataLoaderTest import * 
from EasyPBR  import *
# from DataLoaderTest.P import *
import numpy as np
import time
# import rospy

config_file="empty.cfg"


#Just to have something close to the macros we have in c++
# TIME_START = lambda name: Profiler.start(name)
# TIME_END = lambda name: Profiler.end(name)


def empty():
    view=Viewer(config_file) #first because it needs to init context
    while True:
        view.update()

       

 


def main():
    empty()



if __name__ == "__main__":
     main()  # This is what you would have, but the following is useful:

    # # These are temporary, for debugging, so meh for programming style.
    # import sys, trace

    # # If there are segfaults, it's a good idea to always use stderr as it
    # # always prints to the screen, so you should get as much output as
    # # possible.
    # sys.stdout = sys.stderr

    # # Now trace execution:
    # tracer = trace.Trace(trace=1, count=0, ignoredirs=["/usr", sys.prefix])
    # tracer.run('main()')
