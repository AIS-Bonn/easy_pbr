#!/usr/bin/env python3.6

import sys
import os
try:
  import torch
except ImportError:
  pass
from easypbr  import *

from easypbr  import *
from os import listdir
from os.path import isfile, join
import natsort

config_file="./config/default_params.cfg"

meshes_path="/media/rosu/Data/data/semantic_kitti/predictions/final_7_amsgrad_iou_sequencial"
files = [f for f in listdir(meshes_path) if isfile(join(meshes_path, f)) and "pred" in f  ]
files.sort()
files=natsort.natsorted(files,reverse=True)
mesh=Mesh()

car=Mesh("/media/rosu/Data/data/3d_objs/objects/old_car2/car.ply")
car.V=car.V*0.04
car.rotate_model_matrix_local([1,0,0],-90)
car.rotate_model_matrix_local([0,1,0],-90)
car.translate_model_matrix([0, -1.1, 0])
Scene.show(car,"car")

labels_file="/media/rosu/Data/data/semantic_kitti/colorscheme_and_labels/labels.txt"
colorscheme_file="/media/rosu/Data/data/semantic_kitti/colorscheme_and_labels/color_scheme.txt"
frequency_file="/media/rosu/Data/data/semantic_kitti/colorscheme_and_labels/frequency_uniform.txt"
idx_unlabeled=0
label_mngr=LabelMngr(labels_file, colorscheme_file, frequency_file, idx_unlabeled )
mesh.m_label_mngr=label_mngr


view=Viewer.create(config_file) #first because it needs to init context
view.m_camera.from_string("56.1016 31.3023 43.6047 -0.185032  0.430075 0.0905343 0.878978 0 0 0 40 0.2 6004.45")
recorder=Recorder()
idx=0

while True:
    mesh.load_from_file(os.path.join(meshes_path, files[idx]) )
    mesh.m_vis.m_point_size=10.0
    Scene.show(mesh,"mesh")
    view.update()

    #move camera
    view.m_camera.orbit_y(0.5)
    # if idx==0:
        # view.m_camera.set_lookat([0.0, 0.0, 0.0])
        # view.m_camera.push_away(0.5)

    recorder.record(view, str(idx)+".jpeg", "/media/rosu/Data/phd/c_ws/src/easy_pbr/recordings/latticenet_seq")

    idx+=1
