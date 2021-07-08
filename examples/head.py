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


def make_figure():
  head=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Head/Head.OBJ")
  head.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg", 1)
  head.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg", 1)
  # head.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Gloss_8k.jpg")
  head.m_vis.m_roughness=0.49
  head.model_matrix.rotate_axis_angle([0,1,0], -80)
  head.apply_model_matrix_to_cpu(True)

  jacket=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Jacket/Jacket.OBJ")
  jacket.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Colour.jpg", 1)
  jacket.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Normal.jpg", 1)
  # jacket.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Gloss.jpg")
  jacket.m_vis.m_roughness=0.55
  jacket.model_matrix.rotate_axis_angle([0,1,0], -80)
  jacket.apply_model_matrix_to_cpu(True)

  jacket.name="jacket"
  head.add_child(jacket)

  return head, jacket



# view.load_environment_map("./data/sibl/Barcelona_Rooftops/Barce_Rooftop_C_3k.hdr")
# view.load_environment_map("./data/hdr/blaubeuren_night_4k.hdr")
view.load_environment_map("/media/rosu/Data/data/hdri_haven/nature/epping_forest_01_4k.hdr")

# view.m_camera.from_string("-0.644804  0.302574  0.371758 -0.0450536  -0.476531 -0.0244621 0.877661 -0.00559545    0.224117  -0.0433487 30 0.0320167 32.0167")
view.m_camera.from_string("-0.614212  0.293787  0.377881 -0.0415488  -0.463654 -0.0217731 0.884773 -0.00559545    0.224117  -0.0433487 32 0.0320167 32.0167")

for i in range(1):
  head, jacket = make_figure()
  head.model_matrix.set_translation([i*0.5, 0, 0])
  jacket.model_matrix.set_translation([i*0.5, 0, 0])
  Scene.show(head,"head"+str(i))
  Scene.show(jacket,"jacket"+str(i))

view.m_camera.m_exposure=1.0
view.spotlight_with_idx(2).from_string("0.00953877    1.36971   -1.45745 -0.00112774    0.938742    0.344605 0.00307224        0 0.132991        0 40 0.191147 19.1147")
view.spotlight_with_idx(2).m_power=40
view.spotlight_with_idx(2).m_color=[90/255, 221/255, 255/255]
view.spotlight_with_idx(1).from_string("-1.11644  1.35694 0.953531 -0.309229 -0.393641 -0.142557 0.853874        0 0.132991        0 40 0.191147 19.1147")
view.spotlight_with_idx(1).m_power=11
view.spotlight_with_idx(1).m_color=[255/255, 225/255, 225/255]
view.spotlight_with_idx(0).from_string("1.28357 1.02985 1.09627 -0.219563  0.406239   0.10122 0.881201        0 0.132991        0 40 0.191147 19.1147")
view.spotlight_with_idx(0).m_power=11
view.spotlight_with_idx(0).m_color=[160/255, 225/255, 225/255]








# ########## trying it on /media/rosu/Data/data/hdri_haven/nature/epping_forest_02_4k.hdr
# view.load_environment_map("/media/rosu/Data/data/hdri_haven/nature/epping_forest_02_4k.hdr")

# # view.m_camera.from_string("-0.644804  0.302574  0.371758 -0.0450536  -0.476531 -0.0244621 0.877661 -0.00559545    0.224117  -0.0433487 30 0.0320167 32.0167")
# # view.m_camera.from_string("-0.614212  0.293787  0.377881 -0.0415488  -0.463654 -0.0217731 0.884773 -0.00559545    0.224117  -0.0433487 32 0.0320167 32.0167")
# view.m_camera.from_string("0.463583 0.303449 0.676096 -0.0265493   0.318993 0.00893969 0.947345  -0.0555635    0.255303 -0.00738952 32 0.0320167 32.0167")

# head=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Head/Head.OBJ")
# head.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg", 4)
# head.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg", 4)
# # head.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Gloss_8k.jpg")
# head.m_vis.m_roughness=0.49
# head.m_model_matrix.rotate_axis_angle([0,1,0], -80 + 90)

# jacket=Mesh("/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Jacket/Jacket.OBJ")
# jacket.set_diffuse_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Colour.jpg", 4)
# jacket.set_normals_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Normal.jpg", 4)
# # jacket.set_gloss_tex("/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Gloss.jpg")
# jacket.m_vis.m_roughness=0.55
# jacket.m_model_matrix.rotate_axis_angle([0,1,0], -80+ 90)

# jacket.name="jacket"
# head.add_child(jacket)

# view.m_camera.m_exposure=1.0
# view.spotlight_with_idx(2).from_string("0.00953877    1.36971   -1.45745 -0.00112774    0.938742    0.344605 0.00307224        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(2).m_power=11
# view.spotlight_with_idx(2).m_color=[90/255, 221/255, 255/255]
# view.spotlight_with_idx(1).from_string("-1.11644  1.35694 0.953531 -0.309229 -0.393641 -0.142557 0.853874        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(1).m_power=11
# view.spotlight_with_idx(1).m_color=[255/255, 225/255, 225/255]
# view.spotlight_with_idx(0).from_string("1.28357 1.02985 1.09627 -0.219563  0.406239   0.10122 0.881201        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(0).m_power=11
# view.spotlight_with_idx(0).m_color=[160/255, 225/255, 225/255]









while True:
    view.update()
