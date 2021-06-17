#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/default_params.cfg"

view=Viewer.create(config_file)

while True:
    view.update()
