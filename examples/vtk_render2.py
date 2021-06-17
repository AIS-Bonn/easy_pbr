#!/usr/bin/env python

import os
import sys

import vtk
import easypbr

import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

ms_acum=0
nr_iters=0

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


def CallbackFunction(obj, ev ):
    # print("awd")
    # fps=1.0/obj.GetLastRenderTimeInSeconds()
    # print("fps is ", fps)
    ms= obj.GetLastRenderTimeInSeconds()*1000
    # ms_acum=ms_acum+ms
    # nr_iters+=1
    # print("avg ms ", ms_acum/nr_iters )
    # print("ms is ", ms)
    print("ms is ", ms )

def make_actor(mesh_path, diffuse_path, normal_path ):
    if diffuse_path!="none":
        albedo = GetTexture(diffuse_path)
        albedo.UseSRGBColorSpaceOn()
        normal = GetTexture(normal_path)

    # reader = vtk.vtkOBJReader()
    reader = vtk.vtkPLYReader()
    reader.SetFileName(mesh_path)
    reader.Update()
    polydata=reader.GetOutputDataObject(0)

    # #make it into triangles
    # triangulator=vtk.vtkTriangleFilter()
    # triangulator.SetInputData(polydata)
    # triangulator.Update()
    # polydata=triangulator.GetOutput()


    #compute tangents
    if diffuse_path!="none":
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

    colors = vtk.vtkNamedColors()
    actor.GetProperty().SetColor(colors.GetColor3d('White'))
    actor.GetProperty().SetMetallic(0.0)
    actor.GetProperty().SetRoughness(0.5)

    # configure textures (needs tcoords on the mesh)
    if diffuse_path!="none":
        actor.GetProperty().SetBaseColorTexture(albedo)
    # help(actor.GetProperty())
    # exit(1)
    # actor.GetProperty().SetORMTexture(material)
    # actor.GetProperty().SetOcclusionStrength(occlusionStrength)

    # actor.GetProperty().SetEmissiveTexture(emissive)
    # actor.GetProperty().SetEmissiveFactor(emissiveFactor)

    # needs tcoords, normals and tangents on the mesh
    if diffuse_path!="none":
        actor.GetProperty().SetNormalTexture(normal)
    actor.GetProperty().BackfaceCullingOn()

    return actor

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
# obj_path="/media/rosu/Data/data/3d_objs/3d_scan_store/head.ply"
# diffuse_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Colour_8k.jpg"
# normal_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Head/JPG/Normal Map_SubDivision_1.jpg"
# jacket_obj_path="/media/rosu/Data/data/3d_objs/3d_scan_store/jacket.ply"
# jacket_diffuse_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Colour.jpg"
# jacket_normal_path="/media/rosu/Data/data/3d_objs/3d_scan_store/JPG Textures/Jacket/JPG/Jacket_Normal.jpg"
#goliath
obj_path="/media/rosu/Data/phd/c_ws/src/easy_pbr/goliath_subsampled.ply"
diffuse_path="none"
normal_path="none"

for i in range(1):
    head_actor=make_actor(obj_path, diffuse_path, normal_path)
    # jacket_actor=make_actor(jacket_obj_path, jacket_diffuse_path, jacket_normal_path)
    # transform = vtk.vtkTransform()
    # transform.PostMultiply()
    # transform.Translate(i*0.5, 0, 0)
    # head_actor.SetUserTransform(transform)
    # jacket_actor.SetUserTransform(transform)

    renderer.AddActor(head_actor)
    # renderer.AddActor(jacket_actor)

renderer.UseImageBasedLightingOn()
renderer.SetEnvironmentTexture(cubemap)
# renderer.SetTwoSidedLighting(False)
# renderer.SetBackground(colors.GetColor3d("BkgColor"))
renderer.GetEnvMapIrradiance().SetIrradianceStep(0.3)

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

#shadows  https://gitlab.kitware.com/vtk/vtk/-/blob/master/Rendering/OpenGL2/Testing/Cxx/TestShadowMapPass.cxx
shadows=vtk.vtkShadowMapPass()
shadows.GetShadowMapBakerPass().SetResolution(1024)
# to cancel self->shadowing
# shadows.GetShadowMapBakerPass().SetPolygonOffsetFactor(3.1)
# shadows.GetShadowMapBakerPass().SetPolygonOffsetUnits(10.0)
seq=vtk.vtkSequencePass()
passes=vtk.vtkRenderPassCollection()
passes.AddItem(lights)
passes.AddItem(opaque)
passes.AddItem(shadows.GetShadowMapBakerPass())
passes.AddItem(shadows)
seq.SetPasses(passes)
cameraP=vtk.vtkCameraPass()
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
light_0.SetFocalPoint(0, 0, 0)
# light_0.SetPositional(1)
light_0.SetPosition(1.28357, 1.02985, 1.09627 )
light_0.SetColor(160/255, 225/255, 225/255)
light_0.SetIntensity(11/1)
#light 1
# view.spotlight_with_idx(1).from_string("-1.11644  1.35694 0.953531 -0.309229 -0.393641 -0.142557 0.853874        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(1).m_power=11
# view.spotlight_with_idx(1).m_color=[255/255, 225/255, 225/255]
light_1 = vtk.vtkLight()
light_1.SetFocalPoint(0, 0, 0)
# light_1.SetPositional(1)
light_1.SetPosition(-1.11644,  1.35694, 0.953531  )
light_1.SetColor(255/255, 225/255, 225/255)
light_1.SetIntensity(11/1)
#light 2
# view.spotlight_with_idx(2).from_string("0.00953877    1.36971   -1.45745 -0.00112774    0.938742    0.344605 0.00307224        0 0.132991        0 40 0.191147 19.1147")
# view.spotlight_with_idx(2).m_power=40
# view.spotlight_with_idx(2).m_color=[90/255, 221/255, 255/255]
light_2 = vtk.vtkLight()
light_2.SetFocalPoint(0, 0, 0)
# light_2.SetPositional(1)
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


#get a callback for the endevent to print framerate https://vtk.org/Wiki/VTK/Examples/Cxx/Utilities/FrameRate
#more code in https://vtk.org/Wiki/VTK/Examples/Python/Interaction/MouseEventsObserver
renderer.AddObserver( vtk.vtkCommand.EndEvent , CallbackFunction)



#make rnederer render constantly as fast as it can
# renderer.InteractiveOff()
# renderer.SetSwapControl(0)


#set ssao
# renderer.SetUseSSAO(True)
# renderer.SetSSAORadius(0.035)
# renderer.SetSSAOBlur(True)


# renderWindow.SetSwapBuffers(False)

interactor.SetRenderWindow(renderWindow)

easypbr.Profiler.set_profile_gpu(True)
while True:
    easypbr.Profiler.start("forward")
    renderWindow.Render()
    easypbr.Profiler.end("forward")
    easypbr.Profiler.print_all_stats()

# renderWindow.Render()
# interactor.Start()
