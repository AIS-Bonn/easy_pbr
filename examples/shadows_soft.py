#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/shadows_soft.cfg"

view=Viewer.create(config_file)

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
# view.m_camera.set_position([2.86746,     1.01, 0.697562  ])
# view.m_camera.set_lookat([ 0.428064, -0.494944, -0.046783    ])

mesh=Mesh("/media/rosu/Data/data/3d_objs/my_mini_factory/bust-of-nefertiti-at-the-neues-museum-berlin-1.stl")
# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
mesh.model_matrix.rotate_axis_angle([1,0,0],-90)
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_show_lines=False
grid_floor.m_vis.m_show_mesh=True
grid_floor.scale_mesh(10)

view.m_nr_pcss_pcf_samples=256
view.m_nr_pcss_blocker_samples=256

while True:
    view.update()
