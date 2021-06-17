#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/default_params.cfg"

view=Viewer.create(config_file)
# Scene.set_floor_visible(False)


mesh=Mesh()
mesh.V=[        #fill up the vertices of the mesh as a matrix of Nx3
    [0,0,0],
    [0,1,0],
    [0.2,2,0],
    [-0.4, 2.2, -0.1],
]
mesh.E=[        #fill up the edges as a Nx2 matrix of integers which point into mesh.V. Represents the start and endpoint of the line
    [0,1],
    [1,2],
    [1,3]
]
mesh.C=[    #lines can also have colors. Here we set colors per each vertex and the line gets the color of the endpoint
    [1,0,0],
    [0,1,0],
    [0,0,1],
    [0,1,1]
]
mesh.m_vis.set_color_pervertcolor()
mesh.m_vis.m_show_lines=True
mesh.m_vis.m_show_points=True
mesh.m_vis.m_line_width=6.0
mesh.m_vis.m_point_size=20.0


Scene.show(mesh,"mesh")

while True:
    view.update()
