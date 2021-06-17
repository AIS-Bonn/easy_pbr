#!/usr/bin/env python3.6

import sys
import os
easy_pbr_path= os.path.join( os.path.dirname( os.path.realpath(__file__) ) , '../build')
sys.path.append( easy_pbr_path )
from easypbr  import *

config_file="empty.cfg"

# cloud=Mesh("/media/rosu/Data/data/ais_kitchen/kitchen_meshes/kitchen_gall.ply")
# cloud=Mesh("/media/rosu/Data/data/ais_kitchen/kitchen_meshes/kitchen_wrobel.ply")
# cloud=Mesh("/media/rosu/Data/data/ais_kitchen/kitchen_meshes/kitchen_ais.ply")
cloud=Mesh("/media/rosu/Data/data/ais_kitchen/kitchen_meshes/kitchen_2105.ply")
cloud.random_subsample(0.8)
cloud.rotate_x_axis(180)
cloud.m_vis.m_show_mesh=False
cloud.m_vis.m_show_points=True

# pred=Mesh("/media/rosu/Data/data/ais_kitchen/predictions/scannet_trained_after_submission_fixed_pointnet/0gall_pred.ply")
# pred=Mesh("/media/rosu/Data/data/ais_kitchen/predictions/scannet_trained_after_submission_fixed_pointnet/0wrobel_pred.ply")
# pred=Mesh("/media/rosu/Data/data/ais_kitchen/predictions/scannet_trained_after_submission_fixed_pointnet/0ais_pred.ply")
pred=Mesh("/media/rosu/Data/data/ais_kitchen/predictions/scannet_trained_after_submission_fixed_pointnet/0_2105_pred.ply")
pred.m_vis.m_show_mesh=False
pred.m_vis.m_show_points=True

labels_file="/media/rosu/Data/data/scannet/colorscheme_and_labels/labels.txt"
colorscheme_file="/media/rosu/Data/data/scannet/colorscheme_and_labels/color_scheme.txt"
frequency_file="/media/rosu/Data/data/scannet/colorscheme_and_labels/frequency_uniform.txt"
idx_unlabeled=0
label_mngr=LabelMngr(labels_file, colorscheme_file, frequency_file, idx_unlabeled )
label_mngr.compact("UNUSED")

pred.m_label_mngr=label_mngr
cloud.m_min_max_y_for_plotting=pred.m_min_max_y_for_plotting

Scene.show(pred,"pred")
Scene.show(cloud,"cloud")


view=Viewer.create(config_file) #first because it needs to init context
while True:
    view.update()
