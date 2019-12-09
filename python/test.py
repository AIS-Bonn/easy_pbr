#!/usr/bin/env python3.6

# import sys
# import os
# easy_pbr_path= os.path.join( os.path.dirname( os.path.realpath(__file__) ) , '../build')
# easy_pbr_path= "/home/rosu/Downloads/conda/lib/python3.7/site-packages/lib/"
# easy_pbr_path= "/opt/conda/envs/pt/lib/"
# easy_pbr_path= "/tmp/tmpwpby4mtf/lib/"
# sys.path.append( easy_pbr_path )

# from distutils.sysconfig import get_python_inc
# print( get_python_inc() ) #this gives the include dir


from easypbr  import *

# config_file="/media/rosu/Data/phd/c_ws/src/easy_pbr/config/empty.cfg"
config_file="/media/rosu/Data/phd/c_ws/src/easy_pbr/config/empty.cfg"


view=Viewer.create(config_file) #first because it needs to init context
while True:
    view.update()


