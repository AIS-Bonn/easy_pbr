#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/textures.cfg"

view=Viewer.create(config_file) 
# view.m_camera.from_string("63.0951 90.6802 182.917  -0.12626  0.163956 0.0211642 0.978125       0 39.8803       0 90 4.48741 4487.41") #good for lantern

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/aspose_traders_cart.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/Trades_cart/Traders_cart_mat_Roughness.png")

# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/Regency/aspose_Regency_low_optimized.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Diffuse.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Specular.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/Regency/Regency_low_my_Divani_Chester_nuovi_regency_mat_Glossiness.png")

#work
# mesh=Mesh("./data/textured/lantern/lantern_obj.obj")
# mesh.set_diffuse_tex("./data/textured/lantern/textures/lantern_Base_Color.jpg")
# mesh.set_metalness_tex("./data/textured/lantern/textures/lantern_Metallic.jpg")
# mesh.set_roughness_tex("./data/textured/lantern/textures/lantern_Roughness.jpg")

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

#works
# mesh=Mesh("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/Vintage_Suitcase_LP.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Colour.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/pbr/vintage_suitcase/uploads_files_957239_Vintage_Suitcase/tex/Vintage_Suitcase_Roughness.png")

#works
# mesh=Mesh("/media/rosu/Data/data/3d_objs/render_hub/vintage-auna-radio/Mesh\Auna_Radio.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/render_hub/vintage-auna-radio/Tex\AunaRadio_Tex_Diffuse.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/render_hub/vintage-auna-radio/Tex\AunaRadio_Tex_Metal.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/render_hub/vintage-auna-radio/Tex\AunaRadio_Tex_Roughness.png")

# #work bust doesnt look great
# mesh=Mesh("/media/rosu/Data/data/3d_objs/cgtrader/metal_stand/uploads_files_930602_MetalMiniature.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/cgtrader/metal_stand/Standard/fromcaot_initialShadingGroup_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/cgtrader/metal_stand/Standard/fromcaot_initialShadingGroup_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/cgtrader/metal_stand/Standard/fromcaot_initialShadingGroup_Roughness.png")

# #works and maybe even with normals in world coordinates
# mesh=Mesh("/media/rosu/Data/data/3d_objs/cgtrader/barrel/uploads_files_1962220_Barrel.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/cgtrader/barrel/Barrel Textures/Barrel_Base_Color.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/cgtrader/barrel/Barrel Textures/Barrel_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/cgtrader/barrel/Barrel Textures/Barrel_Roughness.png")

#SKETCHFAB ones 
#corvega car works but shows two cars and a weid plane...
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/corvega/chryslus-corvega_original/aspose_corvega.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/corvega/chryslus-corvega_original/textures/corvega2.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/corvega/chryslus-corvega_original/textures/corvega_m.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/corvega/chryslus-corvega_original/textures/corvega_r.png")

# # works officebot if you rotate a bit to deal with the singularity of the normal
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/office_bot/officebot_original/source/9fc616e208674820a5ccda1c21e844f6.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/office_bot/officebot_original/textures/Albedo3.tga.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/office_bot/officebot_original/textures/metalness.tga.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/office_bot/officebot_original/textures/Roughness2.tga.png")

# #steampunk robot works! it may be the best one because the head is at the top
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/robot_steampunk/robot-steampunk-3d-coat-45-pbr_original/source/585a765e1ea54ef0a0ecc7196cd1b814.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/robot_steampunk/robot-steampunk-3d-coat-45-pbr_original/textures/robot_steampunk_color.tga.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/robot_steampunk/robot-steampunk-3d-coat-45-pbr_original/textures/robot_steampunk_metalness.tga.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/robot_steampunk/robot-steampunk-3d-coat-45-pbr_original/textures/robot_steampunk_rough.tga.png")

#works bake_my_skan lion but looks kinda shitty becaose no normal mapping
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/bake_my_skan/bakemyscan.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/bake_my_skan/bakemyscan-baking-example/source/bakemyscan/bakemyscan_albedo.jpg")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/bake_my_skan/bakemyscan-baking-example/source/bakemyscan/bakemyscan_metallic.jpg")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/bake_my_skan/bakemyscan-baking-example/source/bakemyscan/bakemyscan_roughness.jpg")

#works bulkhead door after rotating a bit
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/bulkhead_door/SM_BulkheadDoor.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/bulkhead_door/bulkhead-door/textures/T_bulkheadDoor_low_BC.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/bulkhead_door/bulkhead-door/textures/bulkheadDoor_low_M_bulkHdoor_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/bulkhead_door/bulkhead-door/textures/bulkheadDoor_low_M_bulkHdoor_Roughness.png")

#TODOOOO a captains pride

#work corinthyan helmet but looks shitty withut normal mapping
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/corinthian_helmet/helmet_low.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/corinthian_helmet/pbr-corinthian-helmet/textures/helmet_low_DefaultMaterial_BaseColor.png")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/corinthian_helmet/pbr-corinthian-helmet/textures/helmet_low_DefaultMaterial_Metallic.png")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/corinthian_helmet/pbr-corinthian-helmet/textures/helmet_low_DefaultMaterial_Roughness.png")

#works equatrian napoleon but could look a bit better with normal mapping
# mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/equestrian_napoleon/napoleon.obj")
# mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/equestrian_napoleon/equestrian-statue-of-napoleon/textures/basecolor.jpg")
# mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/equestrian_napoleon/equestrian-statue-of-napoleon/textures/metallic.jpg")
# mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/equestrian_napoleon/equestrian-statue-of-napoleon/textures/roughness.jpg")

#WORKS la chasse adn is quite nice
mesh=Mesh("/media/rosu/Data/data/3d_objs/sketchfab/la_chasse/statue.obj")
mesh.set_diffuse_tex("/media/rosu/Data/data/3d_objs/sketchfab/la_chasse/la-chasse/textures/statue_albedo.jpg")
mesh.set_metalness_tex("/media/rosu/Data/data/3d_objs/sketchfab/la_chasse/la-chasse/textures/statue_metal.jpg")
mesh.set_roughness_tex("/media/rosu/Data/data/3d_objs/sketchfab/la_chasse/la-chasse/textures/statue_roughness.jpg")

#TODOOO the military ambulance
#TODOOOO the old truck
#TODOOOO the pokeball
#TODOOOO the railroad lantern
#TODOOOO the simple helmet 







mesh.m_vis.set_color_texture()
# mesh.m_vis.m_solid_color=[1.0, 1.0, 1.0]
Scene.show(mesh,"mesh")

#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()