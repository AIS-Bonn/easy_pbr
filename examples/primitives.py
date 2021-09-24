#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/primitives.cfg"

view=Viewer.create(config_file)
# Scene.set_floor_visible(False)


box=Mesh()
box.create_box(1,1,1)
Scene.show(box,"box")

sphere=Mesh()
sphere.create_sphere([0,0,0], 1)
sphere.model_matrix.translate([2,0,0])
Scene.show(sphere,"sphere")

cylinder=Mesh()
origin_at_bottom=True
with_cap=False
cylinder.create_cylinder([0,1,0], 1, 1, origin_at_bottom, with_cap ) #main axis, height, radius
cylinder.model_matrix.translate([4,0,0])
Scene.show(cylinder,"cylinder")

while True:
    view.update()
