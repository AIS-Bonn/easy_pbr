#!/usr/bin/env python3.6

import sys
sys.path.insert(0, '../build')
from EasyPBR  import *

config_file="empty.cfg"


view=Viewer(config_file) #first because it needs to init context
while True:
    view.update()


