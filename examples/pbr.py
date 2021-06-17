#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/pbr.cfg"

view=Viewer.create(config_file)

#puts the camera in a nicer view than default. You can also comment these two lines and EasyPBR will place the camera by default for you so that the scene is fully visible
view.m_camera.set_position([2.14319, -0.516111,   1.65069 ])
view.m_camera.set_lookat([ 0.498184, -0.546767,   -0.1944   ])


grid_size=6
for x in range(grid_size):
    for y in range(grid_size):
        #create a ball at position x,y and metalness and roughness corresponding to x and y
        ball=Mesh("./data/sphere.obj")
        displacement=ball.get_scale()*1.5
        ball.model_matrix.translate([x*displacement,-y*displacement,0]) #translate the sphere to the corresponding position
        ball.m_vis.m_metalness=x/(float(grid_size)-1)
        ball.m_vis.m_roughness=y/(float(grid_size)-1)
        # print("adding ball for x and y", x, " ", y)
        Scene.show(ball,"ball_"+str(x)+"_"+str(y))


#hide the gird floor
Scene.set_floor_visible(False)

while True:
    view.update()
