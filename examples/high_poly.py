#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/high_poly.cfg"

view=Viewer.create(config_file) 
# view.m_camera.from_string("9.60482 48.7507 186.803  0.0503284 -0.0824315 0.00416816 0.995317  41.6137  68.4781 -5.11737 90 6.53242 6532.42")  #for barbarian
# view.m_camera.from_string("260.526 99.6492 50.0316 -0.055629  0.560416 0.0377659 0.825485   51.968  69.2687 -32.7791 90 6.53242 6532.42")  #for goliath
view.m_camera.from_string("186.721  92.773 104.449 -0.0674357    0.38319  0.0280626 0.920786  50.7827  64.5595 -30.6014 90 6.53242 6532.42")  #for goliath

# mesh=Mesh("./data/scan_the_world/barbarian.stl")
# mesh=Mesh("./data/scan_the_world/maqueta-catedral.stl")
mesh=Mesh("./data/scan_the_world/masterpiece-goliath-ii.stl")
mesh.rotate_model_matrix( [1.0, 0.0, 0.0], -90 )
# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0] #for barbarian
mesh.m_vis.m_solid_color=[1.0, 188.0/255.0, 130.0/255.0] #for goliath
# mesh.m_vis.m_roughness=0.39 #for barbarian
# mesh.m_vis.m_roughness=0.42 #for goliath
#for metal goliath
mesh.m_vis.m_metalness=0.64
mesh.m_vis.m_roughness=0.5

# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
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
