#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/head.cfg"

view=Viewer.create(config_file) 
#hide the gird floor
Scene.set_floor_visible(False)


# view.load_environment_map("./data/sibl/Barcelona_Rooftops/Barce_Rooftop_C_3k.hdr")
view.load_environment_map("./data/hdr/blaubeuren_night_4k.hdr")

head=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Head/Head.OBJ")
head.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg", 16)
head.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg", 16)
# head.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Gloss_8k.jpg")
head.m_vis.m_roughness=0.49
head.m_model_matrix.rotate_axis_angle([0,1,0], -80)

jacket=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Jacket/Jacket.OBJ")
jacket.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Colour.jpg", 16)
jacket.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Normal.jpg", 16)
# jacket.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Gloss.jpg")
jacket.m_vis.m_roughness=0.6
jacket.m_model_matrix.rotate_axis_angle([0,1,0], -80)



Scene.show(head,"head")
Scene.show(jacket,"jacket")


while True:
    view.update()