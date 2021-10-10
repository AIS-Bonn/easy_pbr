#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *


#Just to have something close to the macros we have in c++
def profiler_start(name):
    if(Profiler.is_profiling_gpu()):
        torch.cuda.synchronize()
    Profiler.start(name)
def profiler_end(name):
    if(Profiler.is_profiling_gpu()):
        torch.cuda.synchronize()
    Profiler.end(name)
TIME_START = lambda name: profiler_start(name)
TIME_END = lambda name: profiler_end(name)





config_file="./config/cuda_gl_interop.cfg"

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

  #see the original screen texture
  original_screen_tex=view.rendered_tex_no_gui(False)
  original_screen_mat=original_screen_tex.download_to_cv_mat().flip_y().rgb2bgr()
  Gui.show(original_screen_mat, "original_screen_mat")

  #create a new texture and upload into it a cv mat
  new_tex=gl.Texture2D()
  new_tex.upload_from_cv_mat(original_screen_mat)
  new_mat=new_tex.download_to_cv_mat()
  Gui.show(new_mat, "new_mat")


  #make a torch tensor
  # h=original_screen_mat.rows
  # w=original_screen_mat.cols
  # print("original_screen_mat ", original_screen_mat.type_string(), " of size ", h, " ", w )
  # tensor = torch.ones((1,3,h,w), dtype=torch.uint8).cuda()
  # tensor=tensor*0.8

  #make a torch tensor from an image
  mat=Mat()
  # mat.from_file("/home/rosu/work/c_ws/src/easy_pbr/data/uv_checker.png")
  mat.from_file("/home/rosu/work/meetings/weekly_sync/data/week18/2_head_with_shoulders.png")
  tensor=mat2tensor(mat,False).cuda()


  # #copy the tensor to the texture
  new_tex_from_tensor=gl.Texture2D()
  TIME_START("cuda2gl")
  new_tex_from_tensor.from_tensor(tensor)
  TIME_END("cuda2gl")
  TIME_START("gl2mat")
  new_tex_from_tensor_mat=new_tex_from_tensor.download_to_cv_mat()
  TIME_END("gl2mat")
  Gui.show(new_tex_from_tensor_mat, "new_tex_from_tensor_mat")







  view.update()
