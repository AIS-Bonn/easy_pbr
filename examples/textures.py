#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/default_params.cfg"

view=Viewer.create(config_file) 
# view.m_camera.from_string(" 2.86746     1.01 0.697562 -0.211693  0.574033  0.156736 0.775312  0.428064 -0.494944 -0.046783 90 0.00761097 7.61097")

mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/aspose_traders_cart.obj")
mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Base_Color.png")
mesh.m_vis.set_color_texture()
# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
Scene.show(mesh,"mesh")

#hide the gird floor
# grid_floor=Scene.get_mesh_with_name("grid_floor")
# grid_floor.m_vis.m_is_visible=False

while True:
    view.update()