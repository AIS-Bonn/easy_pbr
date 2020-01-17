#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/shadows.cfg"

view=Viewer.create(config_file) 
view.m_camera.from_string("  3.44715  0.838918 -0.254881 -0.142631  0.712764  0.151788 0.669765   0.448066  -0.501703 -0.0681618 90 0.00761097 7.61097")

mesh=Mesh("./data/3d_scene.obj")
mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
mesh.rotate_model_matrix_local([0.0, 1.0, 0.0], 10.0) #rotate around the y axis 10 degrees
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()


