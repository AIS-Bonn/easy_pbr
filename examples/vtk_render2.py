#!/usr/bin/env python

import os
import sys

import vtk

import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)





def ReadCubeMap(folderRoot, fileRoot, ext, key):
    """
    Read the cube map.
    :param folderRoot: The folder where the cube maps are stored.
    :param fileRoot: The root of the individual cube map file names.
    :param ext: The extension of the cube map files.
    :param key: The key to data used to build the full file name.
    :return: The cubemap texture.
    """
    # A map of cube map naming conventions and the corresponding file name
    # components.
    fileNames = {
        0: ['right', 'left', 'top', 'bottom', 'front', 'back'],
        1: ['posx', 'negx', 'posy', 'negy', 'posz', 'negz'],
        2: ['-px', '-nx', '-py', '-ny', '-pz', '-nz'],
        3: ['0', '1', '2', '3', '4', '5']}
    if key in fileNames:
        fns = fileNames[key]
    else:
        print('ReadCubeMap(): invalid key, unable to continue.')
        sys.exit()
    texture = vtk.vtkTexture()
    texture.CubeMapOn()
    # Build the file names.
    for i in range(0, len(fns)):
        fns[i] = folderRoot + fileRoot + fns[i] + ext
        if not os.path.isfile(fns[i]):
            print('Nonexistent texture file:', fns[i])
            return texture
    i = 0
    for fn in fns:
        # Read the images
        readerFactory = vtk.vtkImageReader2Factory()
        imgReader = readerFactory.CreateImageReader2(fn)
        imgReader.SetFileName(fn)

        flip = vtk.vtkImageFlip()
        flip.SetInputConnection(imgReader.GetOutputPort())
        flip.SetFilteredAxis(1)  # flip y axis
        texture.SetInputConnection(i, flip.GetOutputPort(0))
        i += 1
    return texture


def GetTexture(file_name):
    """
    Read an image and convert it to a texture
    :param file_name: The image path.
    :return: The texture.
    """
    # Read the image which will be the texture
    path, extension = os.path.splitext(file_name)
    extension = extension.lower()
    # Make the extension lowercase
    extension = extension.lower()
    validExtensions = ['.jpg', '.png', '.bmp', '.tiff', '.pnm', '.pgm', '.ppm']
    texture = vtk.vtkTexture()
    if not os.path.isfile(file_name):
        print('Nonexistent texture file:', file_name)
        return texture
    if extension not in validExtensions:
        print('Unable to read the texture file:', file_name)
        return texture
    # Read the images
    readerFactory = vtk.vtkImageReader2Factory()
    imgReader = readerFactory.CreateImageReader2(file_name)
    imgReader.SetFileName(file_name)

    texture.SetInputConnection(imgReader.GetOutputPort())
    texture.Update()

    return texture


def ReadHDR(path):

    hdr_equilateral=GetTexture(path)

    equi2cube = vtk.vtkEquirectangularToCubeMapTexture()
    # help(equi2cube)
    # print("nr outputs", equi2cube.GetNumberOfOutputPorts() )
    # exit(1)
    equi2cube.SetInputTexture(hdr_equilateral)
    equi2cube.SetCubeMapSize(512)


    return equi2cube

cube_path="/media/rosu/Data/phd/ws/vtk/Testing/Data/skybox"
hdr_path="/media/rosu/Data/data/hdri_haven/mbpbcRtwt7LykKNWC62lCEpQDJd_ebLn.jpg"
cubemap=ReadHDR(hdr_path)
# cubemap = ReadCubeMap(cube_path, '/', '.jpg', 3)
# cubemap = ReadCubeMap(cube_path, '/skybox', '.jpg', 2)

# Load the skybox
# Read it again as there is no deep copy for vtkTexture
# skybox = ReadCubeMap(cube_path, '/', '.jpg', 0)
# skybox = ReadCubeMap(cube_path, '/', '.jpg', 3)
# skybox = ReadCubeMap(cube_path, '/skybox', '.jpg', 2)
skybox=ReadHDR(hdr_path)
skybox.InterpolateOn()
skybox.RepeatOff()
skybox.EdgeClampOn()


renderer = vtk.vtkRenderer()
renderWindow = vtk.vtkRenderWindow()
renderWindow.AddRenderer(renderer)
interactor = vtk.vtkRenderWindowInteractor()
interactor.SetRenderWindow(renderWindow)


obj_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/lantern_obj.obj"
diffuse_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/textures/lantern_Base_Color.jpg"
normal_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/textures/lantern_Normal_OpenGL.jpg"

# obj_path="/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Head/Head.OBJ"
# diffuse_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg"
# normal_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg"


#get textures 
# material = GetTexture(material_fn)
albedo = GetTexture(diffuse_path)
albedo.UseSRGBColorSpaceOn()
normal = GetTexture(normal_path)


# Build the pipeline
# mapper = vtk.vtkPolyDataMapper()
# mapper.SetInputData(source)

# actor = vtk.vtkActor()
# actor.SetMapper(mapper)

reader = vtk.vtkOBJReader()
reader.SetFileName(obj_path)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(reader.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

actor.GetProperty().SetInterpolationToPBR()

# configure the basic properties
colors = vtk.vtkNamedColors()
actor.GetProperty().SetColor(colors.GetColor3d('White'))
actor.GetProperty().SetMetallic(1.0)
actor.GetProperty().SetRoughness(0.1)

# configure textures (needs tcoords on the mesh)
actor.GetProperty().SetBaseColorTexture(albedo)
# help(actor.GetProperty())
# exit(1)
# actor.GetProperty().SetORMTexture(material)
# actor.GetProperty().SetOcclusionStrength(occlusionStrength)

# actor.GetProperty().SetEmissiveTexture(emissive)
# actor.GetProperty().SetEmissiveFactor(emissiveFactor)

# needs tcoords, normals and tangents on the mesh
normalScale = 1.0
actor.GetProperty().SetNormalTexture(normal)
actor.GetProperty().SetNormalScale(normalScale)

renderer.UseImageBasedLightingOn()
renderer.SetEnvironmentTexture(cubemap)
renderer.SetBackground(colors.GetColor3d("BkgColor"))
renderer.AddActor(actor)

# Comment out if you don't want a skybox
skyboxActor = vtk.vtkSkybox()
skyboxActor.SetTexture(skybox)
renderer.AddActor(skyboxActor)




interactor.SetRenderWindow(renderWindow)

renderWindow.Render()
interactor.Start()