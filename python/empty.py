#!/usr/bin/env python3.6

# import sys
import os
from easypbr  import *

config_file="empty.cfg"


config_path=os.path.join( os.path.dirname( os.path.realpath(__file__) ) , '../config', config_file)
view=Viewer.create(config_path) #first because it needs to init context
while True:
    view.update()


