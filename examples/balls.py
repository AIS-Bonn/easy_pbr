#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/balls.cfg"

view=Viewer.create(config_file) 

# mesh=Mesh("./data/scan_the_world/barbarian.stl")
# mesh=Mesh("./data/scan_the_world/maqueta-catedral.stl")
mesh=Mesh("/media/rosu/Data/data/3d_objs/objects/sphere_15cm.obj")
# mesh.m_vis.m_solid_color=[1.0, 188.0/255.0, 130.0/255.0] #for goliath
# mesh.m_vis.m_metalness=0.64
# mesh.m_vis.m_roughness=0.5

Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    #set light 
    # view.spotlight_with_idx(0).m_color=[1.0, 1.0, 1.0]
    # view.spotlight_with_idx(1).m_color=[1.0, 1.0, 1.0]
    # view.spotlight_with_idx(2).m_color=[1.0, 1.0, 1.0]

    view.update()
