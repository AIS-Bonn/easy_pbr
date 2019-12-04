#!/usr/bin/env python3.6

from easypbr  import *

config_file="/media/rosu/Data/phd/c_ws/src/easy_pbr/config/empty.cfg"


view=Viewer.create(config_file) #first because it needs to init context
while True:
    view.update()


