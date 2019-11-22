#!/usr/bin/env python3.6

import sys
import os
# easy_pbr_path= os.path.join( os.path.dirname( os.path.realpath(__file__) ) , '../build')
easy_pbr_path= "/home/rosu/Downloads/conda/lib/python3.7/site-packages/lib/"
sys.path.append( easy_pbr_path )
from EasyPBR  import *

config_file="empty.cfg"


view=Viewer.create(config_file) #first because it needs to init context
while True:
    view.update()


