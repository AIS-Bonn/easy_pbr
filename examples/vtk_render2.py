#!/usr/bin/env python

import os
import sys

import vtk

import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)



#parts from https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenGL2/Testing/Cxx/TestPBRHdrEnvironment.cxx



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
    texture.InterpolateOn()
    texture.MipmapOn()

    return texture


def ReadHDR(path):

    # hdr_equilateral=GetTexture(path)
    

    #read hdr https://blog.kitware.com/pbrj1/
    reader=vtk.vtkHDRReader()
    reader.SetFileName(path)
    reader.Update()

    texture=vtk.vtkTexture()
    texture.SetColorModeToDirectScalars()
    texture.MipmapOn()
    texture.InterpolateOn()
    texture.SetInputConnection(reader.GetOutputPort())
    texture.Update()
    # hdr_equilateral=texture

    # equi2cube = vtk.vtkEquirectangularToCubeMapTexture()
    # # help(equi2cube)
    # # print("nr outputs", equi2cube.GetNumberOfOutputPorts() )
    # # exit(1)
    # equi2cube.SetInputTexture(hdr_equilateral)
    # equi2cube.SetCubeMapSize(512)

    # return equi2cube
    return texture

# cube_path="/media/rosu/Data/phd/ws/vtk/Testing/Data/skybox"
# hdr_path="/media/rosu/Data/data/hdri_haven/mbpbcRtwt7LykKNWC62lCEpQDJd_ebLn.jpg"
# hdr_path="/media/rosu/Data/data/hdri_haven/nature/epping_forest_02.jpg"
# hdr_path="/media/rosu/Data/data/hdri_haven/nature/epping_forest_01.jpg"
hdr_path="/media/rosu/Data/data/hdri_haven/nature/epping_forest_01_4k.hdr"
cubemap=ReadHDR(hdr_path)
# cubemap = ReadCubeMap(cube_path, '/', '.jpg', 3)
# cubemap = ReadCubeMap(cube_path, '/skybox', '.jpg', 2)

# Load the skybox
# Read it again as there is no deep copy for vtkTexture
# skybox = ReadCubeMap(cube_path, '/', '.jpg', 0)
# skybox = ReadCubeMap(cube_path, '/', '.jpg', 3)
# skybox = ReadCubeMap(cube_path, '/skybox', '.jpg', 2)
# skybox=ReadHDR(hdr_path)
# skybox.InterpolateOn()
# skybox.RepeatOff()
# skybox.EdgeClampOn()


renderer = vtk.vtkRenderer()
renderWindow = vtk.vtkRenderWindow()
renderWindow.AddRenderer(renderer)
interactor = vtk.vtkRenderWindowInteractor()
interactor.SetRenderWindow(renderWindow)


# obj_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/lantern_obj.obj"
# diffuse_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/textures/lantern_Base_Color.jpg"
# normal_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/data/textured/lantern/textures/lantern_Normal_OpenGL.jpg"

# obj_path="/media/rosu/Data/data/3d_objs/3d_scan_store/OBJ/Head/Head.OBJ"
obj_path="/media/rosu/Data/data/3d_objs/3d_scan_store/head.ply"
diffuse_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg"
normal_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg"


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

# reader = vtk.vtkOBJReader()
reader = vtk.vtkPLYReader()
reader.SetFileName(obj_path)
reader.Update()
polydata=reader.GetOutputDataObject(0)

#make it into triangles
triangulator=vtk.vtkTriangleFilter()
triangulator.SetInputData(polydata)
triangulator.Update()
polydata=triangulator.GetOutput()


#compute tangents
tangents=vtk.vtkPolyDataTangents()
tangents.SetInputData(polydata)
tangents.Update()
polydata=tangents.GetOutput()

mapper = vtk.vtkPolyDataMapper()
# mapper.SetInputConnection(reader.GetOutputPort())
mapper.SetInputData(polydata)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

actor.GetProperty().SetInterpolationToPBR()





# configure the basic properties
colors = vtk.vtkNamedColors()
actor.GetProperty().SetColor(colors.GetColor3d('White'))
actor.GetProperty().SetMetallic(0.0)
actor.GetProperty().SetRoughness(0.5)

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
# actor.GetProperty().SetNormalScale(normalScale)

renderer.UseImageBasedLightingOn()
renderer.SetEnvironmentTexture(cubemap)
# renderer.SetTwoSidedLighting(False)
# renderer.SetBackground(colors.GetColor3d("BkgColor"))
renderer.GetEnvMapIrradiance().SetIrradianceStep(0.3)
renderer.AddActor(actor)

# Comment out if you don't want a skybox
skyboxActor = vtk.vtkOpenGLSkybox()
skyboxActor.SetTexture(cubemap)
skyboxActor.SetFloorRight(0.0, 0.0, 1.0)
skyboxActor.SetProjection(vtk.vtkSkybox.Sphere)
renderer.AddActor(skyboxActor)



#ssao  https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenGL2/Testing/Cxx/TestSSAOPass.cxx
# basicPasses=vtk.vtkRenderStepsPass()
#from https://discourse.vtk.org/t/enabling-self-occlusion-shadows-wrt-image-based-skybox-lighting/4358/2
# passes = vtk.vtkRenderPassCollection()
# passes.AddItem(vtk.vtkRenderStepsPass())
# seq = vtk.vtkSequencePass()
# seq.SetPasses(passes)
# ssao=vtk.vtkSSAOPass()
# ssao.SetDelegatePass(seq)
# ssao.SetRadius(0.035)
# ssao.SetKernelSize(128)
# ssao.BlurOff() # do not blur occlusion
# renderer.SetPass(ssao)



# #set tonemapping https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenGL2/Testing/Cxx/TestToneMappingPass.cxx
cameraP=vtk.vtkCameraPass()
seq=vtk.vtkSequencePass()
opaque=vtk.vtkOpaquePass()
lights=vtk.vtkLightsPass()

passes=vtk.vtkRenderPassCollection()
passes.AddItem(lights)
passes.AddItem(opaque)
seq.SetPasses(passes)
cameraP.SetDelegatePass(seq)

toneMappingP=vtk.vtkToneMappingPass()
toneMappingP.SetToneMappingType(vtk.vtkToneMappingPass.GenericFilmic)
toneMappingP.SetGenericFilmicDefaultPresets()
toneMappingP.SetDelegatePass(cameraP)
toneMappingP.SetExposure(0.2)
# vtkOpenGLRenderer::SafeDownCast(renderer)->SetPass(toneMappingP);
renderer.SetPass(toneMappingP)










#lights 
# view.spotlight_with_idx(0).from_string("1.28357 1.02985 1.09627 -0.219563  0.406239   0.10122 0.881201        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(0).m_power=11
# view.spotlight_with_idx(0).m_color=[160/255, 225/255, 225/255]
#light 0
light_0 = vtk.vtkLight()
light_0.SetPositional(1)
light_0.SetPosition(1.28357, 1.02985, 1.09627 )
light_0.SetColor(160/255, 225/255, 225/255)
light_0.SetIntensity(11/1)
#light 1
# view.spotlight_with_idx(1).from_string("-1.11644  1.35694 0.953531 -0.309229 -0.393641 -0.142557 0.853874        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(1).m_power=11
# view.spotlight_with_idx(1).m_color=[255/255, 225/255, 225/255]
light_1 = vtk.vtkLight()
light_1.SetPositional(1)
light_1.SetPosition(-1.11644,  1.35694, 0.953531  )
light_1.SetColor(255/255, 225/255, 225/255)
light_1.SetIntensity(11/1)
#light 2
# view.spotlight_with_idx(2).from_string("0.00953877    1.36971   -1.45745 -0.00112774    0.938742    0.344605 0.00307224        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(2).m_power=40
# view.spotlight_with_idx(2).m_color=[90/255, 221/255, 255/255]
light_2 = vtk.vtkLight()
light_2.SetPositional(1)
light_2.SetPosition(0.00953877,    1.36971 ,  -1.45745   )
light_2.SetColor(90/255, 221/255, 255/255)
light_2.SetIntensity(40/1)


#assign lights to renderer 
renderer.AddLight(light_0)
renderer.AddLight(light_1)
renderer.AddLight(light_2)


#Set camera 
# view.m_camera.from_string("-0.614212  0.293787  0.377881 -0.0415488  -0.463654 -0.0217731 0.884773 -0.00559545    0.224117  -0.0433487 32 0.0320167 32.0167")
# renderer.GetActiveCamera().SetPosition(-0.614212,  0.293787,  0.377881)
# renderer.GetActiveCamera().SetUseHorizontalViewAngle(40)





interactor.SetRenderWindow(renderWindow)

renderWindow.Render()
interactor.Start()