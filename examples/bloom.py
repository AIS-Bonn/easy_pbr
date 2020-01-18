#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/bloom.cfg"

view=Viewer.create(config_file) 
# view=Viewer.create() 
view.m_camera.from_string(" 0.113911 -0.307625   8.70148   -0.030921  0.00654196 0.000202454 0.9995         0 -0.846572         0 70 0.213054 213.054")

mesh=Mesh("./data/head.obj")
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()
    view.spotlight_with_idx(2).from_string("-8.25245  8.19705 -3.44996 -0.213496 -0.768136 -0.320654 0.511436         0 -0.846572         0 40 1.27198 127.198")


