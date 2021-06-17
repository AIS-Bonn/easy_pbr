#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/shadows.cfg"

view=Viewer.create(config_file)

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
view.m_camera.set_position([2.86746,     1.01, 0.697562  ])
view.m_camera.set_lookat([ 0.428064, -0.494944, -0.046783    ])

mesh=Mesh("./data/3d_scene.obj")
mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()
