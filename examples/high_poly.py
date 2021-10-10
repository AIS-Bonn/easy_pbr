#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/high_poly.cfg"

view=Viewer.create(config_file)

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
view.m_camera.set_position([186.721,  92.773, 104.44])
view.m_camera.set_lookat([50.7827,  64.5595, -30.6014 ])

mesh=Mesh("./data/scan_the_world/masterpiece-goliath-ii.stl")
# mesh=Mesh("/media/rosu/Data/phd/c_ws/src/easy_pbr/goliath_subsampled.ply")
mesh.model_matrix.rotate_axis_angle( [1.0, 0.0, 0.0], -90 )
mesh.m_vis.m_solid_color=[1.0, 188.0/255.0, 130.0/255.0] #for goliath
mesh.m_vis.m_metalness=0.0
mesh.m_vis.m_roughness=0.5

Scene.show(mesh,"mesh")

#hide the gird floor
Scene.set_floor_visible(False)


while True:
    view.update()
    # break
