#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *
import numpy as np



def map_range_np( input_val, input_start, input_end,  output_start,  output_end):
    # input_clamped=torch.clamp(input_val, input_start, input_end)
    input_clamped=np.clip(input_val, input_start, input_end)
    # input_clamped=torch.clamp(input_val, input_start, input_end)
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input_clamped - input_start)



config_file="./config/colormap.cfg"
# config_file="./config/default_params.cfg"

view=Viewer.create(config_file)


#get a mesh and paint the height by color
mesh=Mesh("./data/bunny.ply")
Scene.show(mesh,"mesh")

#color based on height
V=mesh.V
V_height=V[:,1:2].copy()
min=np.amin(V_height)
max=np.amax(V_height)
V_height=(V_height-min)/(max-min)
V_height=map_range_np(V_height, 0.0, 1.0, 0.5, 1.0)
colormngr=ColorMngr()
# C=colormngr.eigen2color(V_height, "viridis")
C=colormngr.eigen2color(V_height, "pubu")
# C=np.repeat(colors1d, 3, axis=1)
print("C ",C.shape)
print("C is ", C)
mesh.C=C
mesh.m_vis.set_color_pervertcolor()




while True:
    view.update()
