#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/surfel.cfg"

view=Viewer.create(config_file)

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
view.m_camera.set_position([0.353818,  0.45987, 0.869124   ])
view.m_camera.set_lookat([  -0.023013,  0.0821874, -0.0281476    ])
view.m_camera.m_fov=30

#bunny
mesh=Mesh("/media/rosu/Data/phd/c_ws/src/easy_pbr/data/bunny.ply")
mesh.recalculate_normals()
surfel_size=0.003
mesh.compute_tangents(surfel_size)
mesh.m_vis.m_show_surfels=True
mesh.m_vis.m_show_mesh=False
Scene.show(mesh,"mesh")


#hide the gird floor
Scene.set_floor_visible(False)

while True:
    view.update()
