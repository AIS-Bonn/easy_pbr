#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/textures.cfg"

view=Viewer.create(config_file) 
view.m_camera.from_string("63.0951 90.6802 182.917  -0.12626  0.163956 0.0211642 0.978125       0 39.8803       0 90 4.48741 4487.41")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/aspose_traders_cart.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Roughness.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/Regency/aspose_Regency_low_optimized.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Diffuse.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Specular.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Glossiness.png")

mesh=Mesh("./data/textured/lantern/lantern_obj.obj")
mesh.set_diffuse_tex("./data/textured/lantern/textures/lantern_Base_Color.jpg")
mesh.set_metalness_tex("./data/textured/lantern/textures/lantern_Metallic.jpg")
mesh.set_roughness_tex("./data/textured/lantern/textures/lantern_Roughness.jpg")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/gas_cylinder/uploads_files_2108533_Gas.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/gas_cylinder/uploads_files_2108533_map/lambert3SG_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/gas_cylinder/uploads_files_2108533_map/lambert3SG_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/gas_cylinder/uploads_files_2108533_map/lambert3SG_Roughness.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/distil/uploads_files_1857460_Samovar.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/distil/Textures_UE4/Samovar_BaseColor.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/old_globe/uploads_files_1988578_Old_Antique_Standing_Globe_OBJ/Old_Antique_Standing_Globe_.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/old_globe/Textures/Diffuse.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/old_globe/Textures/Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/old_globe/Textures/Roughness.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/wooden_house/uploads_files_989564_OBJ/hatka_local_.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/wooden_house/uploads_files_989564_texture/home_hatka_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/wooden_house/uploads_files_989564_texture/home_hatka_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/wooden_house/uploads_files_989564_texture/home_hatka_Roughness.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/Vintage_Suitcase_LP.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Colour.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Roughness.png")


mesh.m_vis.set_color_texture()
# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()