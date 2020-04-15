#!/usr/bin/env python3.6
try:
  import torch
except ImportError:
    pass
from easypbr  import *
from easypbr_wrapper  import *

view = Viewer.create()
config_file="config/example.cfg"
wrapper = EasyPBRwrapper.create(config_file,view)

while True:
    view.update()