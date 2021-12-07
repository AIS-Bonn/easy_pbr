#!/usr/bin/env python3

try:
  import torch
except ImportError:
  pass
from easypbr  import *


view=Viewer.create()

#show a mesh
mesh=Mesh("./data/bunny.ply")
Scene.show(mesh,"mesh")


mat=Mat("./data/uv_checker.png")


#show one img
Gui.show(mat, "mat1")

#show another image
view.update()
original_screen_tex=view.rendered_tex_no_gui(False)
original_screen_mat=original_screen_tex.download_to_cv_mat().flip_y().rgb2bgr()
Gui.show(original_screen_mat, "mat2")

#show two images and flip betwen them
Gui.show(mat, "mat1", original_screen_mat, "mat2")

#show three images and flip betwen them
mat3=original_screen_mat.clone().flip_x()
Gui.show(mat, "mat1", original_screen_mat, "mat2", mat3, "mat3")




while True:

    # #show one img
    # Gui.show(mat, "mat1")

    # #show another image
    # original_screen_tex=view.rendered_tex_no_gui(False)
    # original_screen_mat=original_screen_tex.download_to_cv_mat().flip_y().rgb2bgr()
    # Gui.show(original_screen_mat, "mat2")

    # #show two images and flip betwen them
    # Gui.show(mat, "mat1", original_screen_mat, "mat2")


    view.update()
