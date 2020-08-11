#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/pbr.cfg"

view=Viewer.create(config_file) 
view.m_camera.from_string("  2.14319 -0.516111   1.65069 -0.00579448    0.356071   0.0022081 0.934438  0.498184 -0.546767   -0.1944 90 0.00761097 7.61097")


grid_size=6
for x in range(grid_size):
    for y in range(grid_size):
        #create a ball at position x,y and metalness and roughness corresponding to x and y
        ball=Mesh("./data/sphere.obj")
        displacement=ball.get_scale()*1.5
        ball.translate_model_matrix([x*displacement,-y*displacement,0]) #translate the sphere to the corresponding position
        ball.m_vis.m_metalness=x/(float(grid_size)-1)
        ball.m_vis.m_roughness=y/(float(grid_size)-1)
        # print("adding ball for x and y", x, " ", y)
        Scene.show(ball,"ball_"+str(x)+"_"+str(y))


#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()


