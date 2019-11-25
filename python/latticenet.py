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

car=Mesh("/media/rosu/Data/data/3d_objs/old_car2/car.ply")
car.V=car.V*0.04
car.rotate_x_axis(-90)
car.rotate_y_axis(-90)
car.move_in_y(-1.1)
Scene.show(car,"car")


view=Viewer.create(config_file) #first because it needs to init context
view.m_camera.from_string("56.1016 31.3023 43.6047 -0.185032  0.430075 0.0905343 0.878978 0 0 0 40 0.2 6004.45")
recorder=Recorder()
idx=0

while True:
    mesh.load_from_file(os.path.join(meshes_path, files[idx]) )
    mesh.m_vis.m_point_size=6.0
    Scene.show(mesh,"mesh")
    view.update()

    #move camera
    view.m_camera.orbit_y(0.5)
    # if idx==0:
        # view.m_camera.set_lookat([0.0, 0.0, 0.0])
        # view.m_camera.push_away(0.5)

    recorder.record(view, str(idx)+".jpg", "/media/rosu/Data/phd/c_ws/src/easy_pbr/recordings/latticenet")

    idx+=1

