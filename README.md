# EasyPBR

<p align="middle">
  <!-- <img src="imgs/anatomy_crop_2.png" width="250" /> -->
  <img src="imgs/house_crop_2.png" width="260" />
  <!-- <figcaption> Your text </figcaption> -->
  <!-- <img src="imgs/buburuza_crop_2.png" width="250" />  -->
  <!-- <img src="imgs/lucy_pc_crop.png" width="210" />  -->
  <img src="imgs/bunny_pc_7_crop.png" width="250" /> 
  <!-- <figcaption> Your text </figcaption> -->
  <!-- <img src="imgs/house_crop_2.png" width="250" /> -->
  <img src="imgs/shader_ball_crop.png" width="250" />
</p>

EasyPBR is a lightweight Physically-Based Renderer designed for ease of use and fast prototyping. It abstracts away most of the complexity of rendering allowing the user to display high quality images in only a couple of lines of code. By default, EasyPBR tries to choose the parameters in order to best render a particular scene but if more control is needed, the whole rendering process is controllable through either a Python script or C++ program. EasyPBR is written in C++ and OpenGL and provides Python bindings to most of it's functionality.

<!-- ![Image description](imgs/stadium.png) | ![Image description](imgs/mnt_valley_dirt.png) -->



# Example
    from easypbr  import *
    
    view=Viewer.create("default_params.cfg")
    mesh = Mesh("./data/lucy.obj")
    Scene.show(mesh, "mesh")
    
    while True:
        view.update()

# Features
- Deferred rendering 
- Physically-based shading
- Screen Space Ambient Occlusion (SSAO)
- Shadow Mapping with Percentage Closer Filtering for soft shadows
- Image Based Lighting (IBL) using HDR maps
- Eye Dome Lighting for point cloud rendering
- Various tonemapping operators (Rheinhard, Unreal, ACES)
- Shader hotloading 
- Various modes of visualization (mesh, wireframe, point cloud, surfel)
- Loading of multiple meshes formats (.obj, .ply, .pcd, .off)
- Drag and drop of meshes and HDR maps directly in the viewer
- Easy extension and usage from other packages

# Install 
### Dependencies:
```sh
$ sudo apt-get install python3-pip python3-setuptools libglfw3-dev libboost-all-dev libeigen3-dev libpcl-dev libopencv-dev
```
### Optional dependencies: 
Allow for shader hotloading, so changes to .glsl files are automatically recompiled and used while the program is running:
```sh   
$ sudo apt-get install libkqueue-dev
```

### Build and install: 
```sh
$ git clone --recursive https://github.com/RaduAlexandru/easy_pbr.git
$ cd easy_pbr
$ ./build_and_install.sh
```

