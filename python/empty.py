try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/default_params.cfg"

view=Viewer.create(config_file) #first because it needs to init context

while True:
    view.update()


