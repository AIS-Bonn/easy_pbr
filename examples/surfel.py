#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *

config_file="./config/surfel.cfg"

view=Viewer.create(config_file) 
view.m_camera.from_string("-4.46597 0.988445  5.19445 -0.0671241  -0.346759 -0.0248882 0.935217 0 0 0 30 0.286421 286.421")

#bunny 
mesh=Mesh("/media/rosu/Data/phd/c_ws/src/easy_pbr/data/bunny.ply")
mesh.recalculate_normals()
mesh.compute_tangents(0.003)
# mesh.compute_tangents(0.001)
mesh.m_vis.m_show_surfels=True
mesh.m_vis.m_show_mesh=False
Scene.show(mesh,"mesh")


# angle_rot=-30
# #one surfel
# mesh=Mesh()
# mesh.V=[        #fill up the vertices of the mesh as a matrix of Nx3
#     [0,0,0],
#     [5,0,0]
# ]
# mesh.NV=[        
#     [0,0,1],
#     [0,0,1]
# ]
# mesh.rotate_model_matrix([1,0,0],angle_rot)
# mesh.compute_tangents()
# mesh.m_vis.m_show_surfels=True
# mesh.m_vis.m_show_mesh=False

# #surfel border
# border=Mesh()
# v=0.72
# border.V=[        #fill up the vertices of the mesh as a matrix of Nx3
#     [-v,-v,0],
#     [v,-v,0],
#     [v,v,0],
#     [-v,v,0]
# ]
# border.E=[        
#     [0,1],
#     [1,2],
#     [2,3],
#     [3,0]
# ]
# border.m_vis.m_show_lines=True
# border.m_vis.m_show_points=True
# border.m_vis.m_line_width=20
# border.m_vis.m_point_size=30
# border.m_vis.m_line_color=[0,0,0]
# border.m_vis.m_point_color=[0,0,0]
# border.rotate_model_matrix_local([0,0,1],45)
# border.rotate_model_matrix_local([1,0,0],angle_rot)

# #make a basis vector for N, T and B
# basis=Mesh()
# d=0.7
# basis.V=[        #fill up the vertices of the mesh as a matrix of Nx3
#     [0,0,0],
#     [0,0,d],
#     [-d,0,0],
#     [0,d,0]
# ]
# basis.E=[        
#     [0,1],
#     [0,2],
#     [0,3],
# ]
# basis.rotate_model_matrix([1,0,0],angle_rot)
# basis.m_vis.m_show_lines=True
# basis.m_vis.m_line_width=30

# Scene.show(mesh,"mesh")
# Scene.show(border,"border")
# Scene.show(basis,"basis")


#hide the gird floor
grid_floor=Scene.get_mesh_with_name("grid_floor")
grid_floor.m_vis.m_is_visible=False

while True:
    view.update()