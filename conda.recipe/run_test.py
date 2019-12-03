#!/usr/bin/env python3.6

from easypbr  import *

config_file="empty.cfg"


view=Viewer.create(config_file) #first because it needs to init context
while True:
    view.update()


