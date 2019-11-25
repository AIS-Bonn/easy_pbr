#!/usr/bin/env python3.6

import sys
import os
easy_pbr_path= os.path.join( os.path.dirname( os.path.realpath(__file__) ) , '../build')
# easy_pbr_path= "/home/rosu/Downloads/conda/lib/python3.7/site-packages/lib/"
# easy_pbr_path= "/opt/conda/envs/pt/lib/"
# easy_pbr_path= "/tmp/tmpwpby4mtf/lib/"
sys.path.append( easy_pbr_path )
from easypbr  import *
from os import listdir
from os.path import isfile, join
import natsort 

config_file="latticenet.cfg"

meshes_path="/media/rosu/Data/data/semantic_kitti/predictions/final_7_amsgrad_iou_sequencial"
files = [f for f in listdir(meshes_path) if isfile(join(meshes_path, f)) and "pred" in f  ]
files.sort()
files=natsort.natsorted(files,reverse=True)

mesh=Mesh()


view=Viewer.create(config_file) #first because it needs to init context
idx=0

while True:
    mesh.load_from_file(os.path.join(meshes_path, files[idx]) )
    Scene.show(mesh,"mesh")
    view.update()

    idx+=1

