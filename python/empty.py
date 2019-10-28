#!/usr/bin/env python3.6

import sys
sys.path.insert(0, '../build')
from EasyPBR  import *
# from MBZIRC_CH1  import *

config_file="empty.cfg"


view=Viewer.create(config_file) #first because it needs to init context
synth=SyntheticGenerator.create(config_file)

while True:
    view.update()


