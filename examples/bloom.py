#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/bloom.cfg"

view=Viewer.create(config_file)

mesh=Mesh("./data/head.obj")
Scene.show(mesh,"mesh")

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
view.m_camera.set_position([ 0.113911, -0.307625,   8.70148  ])
view.m_camera.set_lookat([ 0, -0.846572,      0    ])

# spotlights can be moved in the scene as if they were a camera
light=view.spotlight_with_idx(2)
light.set_position([ -8.25245,  8.19705, -3.44996   ])
light.set_lookat([ 0, -0.846572,      0  ])

#hide the gird floor
Scene.set_floor_visible(False)

while True:
    view.update()
