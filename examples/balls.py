#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/balls.cfg"

view=Viewer.create(config_file)

mesh=Mesh("./data/sphere.obj")

Scene.show(mesh,"mesh")

#hide the gird floor
Scene.set_floor_visible(False)

while True:
    view.update()
