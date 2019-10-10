cmake_minimum_required(VERSION 2.8.3)
project(surfel_renderer)



###   VARIABLES   ##############################################################
# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include
                        ${CMAKE_SOURCE_DIR}/deps
                        ${CMAKE_SOURCE_DIR}/deps/imgui
                        ${CMAKE_SOURCE_DIR}/deps/imgui_ext
                        ${CMAKE_SOURCE_DIR}/deps/imgui/examples/
                        ${CMAKE_SOURCE_DIR}/deps/loguru
                        ${CMAKE_SOURCE_DIR}/deps/dir_watcher
                        ${CMAKE_SOURCE_DIR}/deps/EasyGL
                        ${CMAKE_SOURCE_DIR}/deps/EasyPytorch
                        ${CMAKE_SOURCE_DIR}/deps/EasyCuda
                        ${CMAKE_SOURCE_DIR}/deps/cv_lib
                        ${CMAKE_SOURCE_DIR}/deps/Configuru
                        ${CMAKE_SOURCE_DIR}/deps/libigl/include
                        ${CMAKE_SOURCE_DIR}/deps/better-enums
                        ${CMAKE_SOURCE_DIR}/deps/ImGuizmo/
                        ${CMAKE_SOURCE_DIR}/deps/tiny-process-library
                        ${CMAKE_SOURCE_DIR}/deps/concurrentqueue
                        ${CMAKE_SOURCE_DIR}/deps/cnpy
                        ${CMAKE_SOURCE_DIR}/deps/json11
                        ${CMAKE_SOURCE_DIR}/deps/pybind11/include
                        ${CMAKE_SOURCE_DIR}/deps/nanoflann/include
                        ${CMAKE_SOURCE_DIR}/deps/jitify
                        ${CMAKE_SOURCE_DIR}/deps/tinyply/source
                        )		# Header folder
#we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-format-security -Wno-cpp")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3") #optimizations
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/ros/melodic/\;/opt/conda/envs/pt/lib/python3.6/site-packages/torch")
set(PYTORCH_PATH "/opt/pytorch")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/ros/melodic/\;${PYTORCH_PATH}")
set(CMAKE_CXX_STANDARD 17) #needed also by the pybind so it can build itself in c++14 mode so we can use py:overload_cast



####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
option(CORE_WITH_VIEWER "Use a GL viewer, or if we don't have it available we will still publish to RViz" ON)
option(CORE_WITH_HALIDE "Compile Halide" OFF)
option(CORE_WITH_TORCH "Compile Torch" ON)
option(CORE_WITH_CUDA "Compile CUDA" ON)
option(CORE_WITH_GLM "With GLM for some quality of life functions in EasyGL" OFF)
option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" ON)



set(PYTHON_VERSION 3.6 CACHE STRING "" FORCE)


#pybind package
#setting a python version to 3.6 for pybind so that the bindings work in 3.6 https://github.com/pybind/pybind11/issues/748
unset(PYTHONINTERP_FOUND CACHE)
unset(PYTHON_EXECUTABLE CACHE)
unset(PYTHON_VERSION_STRING CACHE)
unset(PYTHON_VERSION_MAJOR CACHE)
unset(PYTHON_VERSION_MINOR CACHE)
unset(PYTHON_VERSION_PATCH CACHE)
unset(PYBIND11_INCLUDE_DIR CACHE)
unset(PYTHON_INCLUDE_DIRS CACHE)
unset(PYTHON_LIBRARIES CACHE)
unset(PYTHON_MODULE_PREFIX CACHE)
unset(PYTHON_MODULE_EXTENSION CACHE)
unset(PYTHON_LIBRARY CACHE)
set(PYBIND11_PYTHON_VERSION 3.6 CACHE STRING "" FORCE) 
message("PYBIND11_PYTHON_VERSION is" , ${PYBIND11_PYTHON_VERSION})

add_subdirectory( ${CMAKE_SOURCE_DIR}/deps/pybind11)


#pybind attempt 3
pybind11_add_module(DataLoaderTest ${CMAKE_SOURCE_DIR}/src/pycom/PyCom.cxx )
target_compile_definitions(DataLoaderTest PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")


# libigl options: choose between header only and compiled static library
# Header-only is preferred for small projects. For larger projects the static build
# considerably reduces the compilation times
option(LIBIGL_USE_STATIC_LIBRARY "Use libigl as static library" OFF)
option(LIBIGL_WITH_ANTTWEAKBAR       "Use AntTweakBar"    OFF)
option(LIBIGL_WITH_CGAL              "Use CGAL"           OFF)
option(LIBIGL_WITH_COMISO            "Use CoMiso"         OFF)
option(LIBIGL_WITH_CORK              "Use Cork"           OFF)
option(LIBIGL_WITH_EMBREE            "Use Embree"         ON)
option(LIBIGL_WITH_LIM               "Use LIM"            OFF)
option(LIBIGL_WITH_MATLAB            "Use Matlab"         OFF)
option(LIBIGL_WITH_MOSEK             "Use MOSEK"          OFF)
option(LIBIGL_WITH_OPENGL            "Use OpenGL"         OFF)
option(LIBIGL_WITH_OPENGL_GLFW       "Use GLFW"           OFF)
option(LIBIGL_WITH_OPENGL_GLFW_IMGUI "Use ImGui"          OFF)
option(LIBIGL_WITH_PNG               "Use PNG"            OFF)
option(LIBIGL_WITH_PYTHON            "Use Python"         OFF)
option(LIBIGL_WITH_TETGEN            "Use Tetgen"         OFF)
option(LIBIGL_WITH_TRIANGLE          "Use Triangle"       ON)
option(LIBIGL_WITH_VIEWER            "Use OpenGL viewer"  OFF)
option(LIBIGL_WITH_XML               "Use XML"            OFF)




######   PACKAGES   ############################################################
find_package(catkin REQUIRED)
find_package(bsplines REQUIRED)
find_package(sm_kinematics REQUIRED)
find_package(sparse_block_matrix REQUIRED)
find_package(custom_msgs REQUIRED)
find_package(catkin REQUIRED COMPONENTS roscpp std_msgs cv_bridge pcl_ros image_transport 
    bsplines
    sm_kinematics
    sparse_block_matrix
    custom_msgs
)
# find_package(catkin_simple REQUIRED)
# catkin_simple(ALL_DEPS_REQUIRED)
find_package(GLFW REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(LIBIGL REQUIRED) #for calculating normals for each mesh so we can view them 
if(CORE_WITH_VIEWER)
    add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD ) #Imgui will use glad loader
endif()
if(CORE_WITH_HALIDE)
    find_package(Halide REQUIRED)
endif()
if(CORE_WITH_TORCH)
    find_package(Torch REQUIRED)
endif()
if(CORE_WITH_CUDA)
    find_package(CUDA REQUIRED)
endif()
if(CORE_WITH_GLM)
    find_package(GLM REQUIRED)
endif()
if(CORE_WITH_DIR_WATCHER)
    find_package(kqueue REQUIRED)
endif()
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# message(${CUDA_INCLUDE_DIRS})
# message(${CUDA_LIBRARIES})


###   INCLUDES   #########################################################
include_directories(${PROJECT_INCLUDE_DIR} ${catkin_INCLUDE_DIRS})
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${LIBIGL_INCLUDE_DIR})
include_directories(${HALIDE_INCLUDE_DIR})
include_directories(${TORCH_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})
add_definitions(${GLM_DEFINITIONS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${KQUEUE_INCLUDE_DIRS})
catkin_package( INCLUDE_DIRS include  CATKIN_DEPENDS roscpp std_msgs cv_bridge pcl_ros image_transport
    bsplines
    sm_kinematics
    sparse_block_matrix
    custom_msgs
)



### For YouCompeleteMe vim plugin And for aucomplete in Visual Studio Code ####
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON") # for YouCompleteMe plugging in vim <http://bastian.rieck.ru/blog/posts/2015/ycm_cmake/
IF( EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()


###   SOURCES   #################################################################
#imgui stuff
if(CORE_WITH_VIEWER)
    file(GLOB IMGUI_SRC ${CMAKE_SOURCE_DIR}/deps/imgui/*.c* ${CMAKE_SOURCE_DIR}/deps/imgui/examples/imgui_impl_glfw.cpp ${CMAKE_SOURCE_DIR}/deps/imgui/examples/imgui_impl_opengl3.cpp ${CMAKE_SOURCE_DIR}/deps/ImGuizmo/ImGuizmo.cpp
    )
endif()
set(MY_SRC  
            ${CMAKE_SOURCE_DIR}/src/main.cxx
            ${CMAKE_SOURCE_DIR}/src/core/Core.cxx
            ${CMAKE_SOURCE_DIR}/src/viewer/Scene.cxx
            ${CMAKE_SOURCE_DIR}/src/core/MeshCore.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/RosBagPlayer.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderPNG.cxx 
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderCloudRos.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderImgRos.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderSemanticKitti.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderModelNet40.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderShapeNetPartSeg.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderToyExample.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderRueMonge.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderStanfordIndoor.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderScanNet.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataLoaderVolRef.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/DataTransformer.cxx 
            ${CMAKE_SOURCE_DIR}/src/data_loader/Frame.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/LabelMngr.cxx
            ${CMAKE_SOURCE_DIR}/src/data_loader/ceres_spline_opt.cpp
            #plugins that I am trying out 
            ${CMAKE_SOURCE_DIR}/src/lattice/LatticeCPU_test.cxx
            ${CMAKE_SOURCE_DIR}/src/lattice/VoxelGrid.cxx
            ${CMAKE_SOURCE_DIR}/src/lattice/Voxel.cxx
            ${CMAKE_SOURCE_DIR}/src/cloud_segmenter/CloudSegmenter.cxx
            ${CMAKE_SOURCE_DIR}/src/mesher/Mesher.cxx
            ${CMAKE_SOURCE_DIR}/src/mesher/Edge.cxx
            ${CMAKE_SOURCE_DIR}/src/mesher/triangle_utils.cpp
            ${CMAKE_SOURCE_DIR}/src/ball_detector/BallDetector.cxx
            ${CMAKE_SOURCE_DIR}/src/texturer/Texturer.cxx
            ${CMAKE_SOURCE_DIR}/src/roscom/RosPub.cxx
            ${CMAKE_SOURCE_DIR}/src/agregator/Agregator.cxx
            ${CMAKE_SOURCE_DIR}/src/fire_detector/FireDetector.cxx
            ${CMAKE_SOURCE_DIR}/src/fire_detector/AABB.cxx
            ${CMAKE_SOURCE_DIR}/src/utils/ColormapMngr.cxx
            ${CMAKE_SOURCE_DIR}/src/dynamics_detector/DynamicsDetector.cxx
            ${CMAKE_SOURCE_DIR}/src/misc_modules/CloudComparer.cxx
            ${CMAKE_SOURCE_DIR}/src/misc_modules/PermutoLatticePlotter.cxx
            ${CMAKE_SOURCE_DIR}/src/misc_modules/TrainParams.cxx
            ${CMAKE_SOURCE_DIR}/src/misc_modules/EvalParams.cxx
            ${CMAKE_SOURCE_DIR}/src/misc_modules/ModelParams.cxx
            )
if(CORE_WITH_VIEWER)
    set(MY_SRC ${MY_SRC} ${CMAKE_SOURCE_DIR}/src/viewer/Viewer.cxx
                         ${CMAKE_SOURCE_DIR}/src/viewer/Gui.cxx
                         ${CMAKE_SOURCE_DIR}/src/viewer/Camera.cxx
                         ${CMAKE_SOURCE_DIR}/src/viewer/Recorder.cxx
                         ${CMAKE_SOURCE_DIR}/src/viewer/Mesh.cxx
                         ${CMAKE_SOURCE_DIR}/src/viewer/SpotLight.cxx
                         )
endif()
if(CORE_WITH_HALIDE)
    set(MY_SRC ${MY_SRC} ${CMAKE_SOURCE_DIR}/src/halide/DynaVolHalide.cxx )
endif()
if(CORE_WITH_TORCH)
    set(MY_SRC ${MY_SRC} #${CMAKE_SOURCE_DIR}/src/cnn/CNN.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/ModelCore.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/OneConv.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/ThreeFC.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/TinyConv.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/ThreeFCFunctional.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/SurfelNN.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/SurfelNNConv.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/Surfel3D.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/models/TestExtensions.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/LatticeModule.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/LatticeFunc.cxx

                         #${CMAKE_SOURCE_DIR}/src/cnn/modules/DummyExtensions.cxx
                         #${CMAKE_SOURCE_DIR}/src/cnn/LNN.cxx
                         )
endif()
if(CORE_WITH_CUDA)
    set(MY_SRC ${MY_SRC} 
                         ${CMAKE_SOURCE_DIR}/src/lattice/LatticeGPU_test.cxx
                         )
endif()
if(CORE_WITH_DIR_WATCHER)
    set(DEPS_SRC ${DEPS_SRC} ${CMAKE_SOURCE_DIR}/deps/dir_watcher/dir_watcher.cpp)
endif()
#cnpy for reading npz files (python numpy format) like the one from the semantic_kitti dataset
set(DEPS_SRC ${DEPS_SRC} ${CMAKE_SOURCE_DIR}/deps/cnpy/cnpy.cpp)
#json for readin some files from the shapenet part segmentation dataset
set(DEPS_SRC ${DEPS_SRC} ${CMAKE_SOURCE_DIR}/deps/json11/json11.cpp)
#tinyply because libigl readPLY has a memory leak 
set(DEPS_SRC ${DEPS_SRC} ${CMAKE_SOURCE_DIR}/deps/tinyply/source/tinyply.cpp)





###   SET ALL THE GLOBAL OPTIONS #########################################
if(CORE_WITH_VIEWER)
    message("USING GL Viewer")
    add_definitions(-DWITH_VIEWER)
else()
    message("USING OFFSCREEN")
endif()
if(CORE_WITH_HALIDE)
    message("USING HALIDE")
    add_definitions(-DWITH_HALIDE)
else()
    message("NOT USING HALIDE")
endif()
if(CORE_WITH_TORCH)
    message("USING TORCH")
    add_definitions(-DWITH_TORCH)
else()
    message("NOT USING TORCH")
endif()
if(CORE_WITH_CUDA)
    message("USING CUDA")
    add_definitions(-DWITH_CUDA)
else()
    message("NOT USING CUDA")
endif()
if(CORE_WITH_GLM)
    message("USING GLM")
    add_definitions(-DWITH_GLM)
else()
    message("NOT USING GLM")
endif()
if(CORE_WITH_DIR_WATCHER)
    message("USING DIR_WATCHER")
    add_definitions(-DWITH_DIR_WATCHER)
else()
    message("NOT USING DIR_WATCHER")
endif()


###   MAIN LIB  ####################3
add_library( MAIN_LIB ${MY_SRC}  deps/glad/glad.c ${DEPS_SRC} ${IMGUI_SRC}  )


###   EXECUTABLE   #######################################
add_executable(surfel_renderer ${CMAKE_SOURCE_DIR}/src/null.cxx  )
set_target_properties(surfel_renderer PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)



###   LIBS   ###############################################
set(LIBS ${LIBS} Eigen3::Eigen ${catkin_LIBRARIES}  ${GLFW_LIBRARIES} ${OpenCV_LIBS} ${CUDA_LIBRARIES} igl::core igl::triangle igl::embree )
if(CORE_WITH_VIEWER)
    set(LIBS ${LIBS})
endif()
if(CORE_WITH_HALIDE)
    set(LIBS ${LIBS} ${HALIDE_LIBRARIES} "-lX11"  "-lz -ltinfo -lglfw -luuid -lfreetype" )
endif()
if(CORE_WITH_TORCH)
    # set(LIBS ${LIBS} ${TORCH_LIBRARIES} ${FREETYPE_LIBRARIES} )
    set(LIBS ${LIBS} ${TORCH_LIBRARIES} )
endif()
if(CORE_WITH_DIR_WATCHER)
    set(LIBS ${LIBS} ${KQUEUE_LIBRARIES})
endif()

###    CUDA modules  ######################
if(CORE_WITH_CUDA)
    #configure the include paths of jitify so that the kernels can properly include other ones
    configure_file(${CMAKE_SOURCE_DIR}/include/surfel_renderer/jitify_helper/jitify_options.hpp.in ${CMAKE_SOURCE_DIR}/include/surfel_renderer/jitify_helper/jitify_options.hpp)
    install(FILES "${CMAKE_SOURCE_DIR}/include/surfel_renderer/jitify_helper/jitify_options.hpp" DESTINATION ${CMAKE_SOURCE_DIR}/include/surfel_renderer/jitify_helper/)

    # set(CUDA_NVCC_FLAGS "-gencode arch=compute_30 -code=sm_30" CACHE STRING "nvcc flags" FORCE)
    set(CUDA_NVCC_FLAGS "-arch=sm_60" CACHE STRING "nvcc flags" FORCE)
    message("NVCC flags are ", ${CUDA_NVCC_FLAGS})
    set(CUDA_VERBOSE_BUILD ON CACHE BOOL "nvcc verbose" FORCE) 
    set(LIB_TYPE STATIC)
    cuda_add_library(LatticeGPU ${LIB_TYPE}  ${CMAKE_SOURCE_DIR}/src/lattice/Lattice.cu ${CMAKE_SOURCE_DIR}/src/lattice/HashTable.cu  )
    set(LIBS ${LIBS} LatticeGPU)

 
endif()






###   TINY PROCESS LIBRARY   #########################################
set (TINY_PROCESS_LIB_SOURCES   ${CMAKE_SOURCE_DIR}/deps/tiny-process-library/process.cpp ${CMAKE_SOURCE_DIR}/deps/tiny-process-library/process_unix.cpp )
add_library( TINY_PROCESS_LIB ${TINY_PROCESS_LIB_SOURCES} )
target_link_libraries(TINY_PROCESS_LIB ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(TINY_PROCESS_LIB PROPERTIES LINKER_LANGUAGE C)
set(LIBS ${LIBS} TINY_PROCESS_LIB)

###   LINKING   #################################################
target_link_libraries(MAIN_LIB  ${LIBS})
target_link_libraries(surfel_renderer MAIN_LIB )
add_dependencies(surfel_renderer ${catkin_EXPORTED_TARGETS})

#definitions for cmake variables that are necesarry during runtime
# target_compile_definitions(surfel_renderer PRIVATE AWESOMEFONT_DIR="${CMAKE_SOURCE_DIR}/deps/imgui_ext/fontawesome-webfont.ttf")
# target_compile_definitions(surfel_renderer PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(MAIN_LIB PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(MAIN_LIB PRIVATE AWESOMEFONT_DIR="${CMAKE_SOURCE_DIR}/deps/imgui_ext/fontawesome-webfont.ttf")
target_compile_definitions(MAIN_LIB PRIVATE -DTRILIBRARY -DANSI_DECLARATORS)
if(CORE_WITH_CUDA)
    target_compile_definitions(LatticeGPU PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

# ###   pybind 
# pybind11_add_module(DataLoaderTest ${CMAKE_SOURCE_DIR}/src/pycom/PyCom.cxx )
# target_compile_definitions(DataLoaderTest PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(DataLoaderTest MAIN_LIB )



# ##pybind attempt 2 like in here # https://github.com/pybind/pybind11/issues/748 
# set(_target_name "DataLoaderTest")
# add_library(${_target_name} MODULE ${CMAKE_SOURCE_DIR}/src/pycom/PyCom.cxx )
# target_include_directories(${_target_name} PRIVATE ${PYBIND11_INCLUDE_DIR})
# target_include_directories(${_target_name} PRIVATE ${PYTHON_INCLUDE_DIRS})
# set_target_properties(${_target_name} PROPERTIES PREFIX ""
#                                                  SUFFIX ".so"
#                                                  )
# target_compile_definitions(${_target_name}  PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(${_target_name} MAIN_LIB )




#compile first the halide ahead-of-time functions
if(CORE_WITH_HALIDE)

    include(HalideGenerator) #some convenience functions to get the path to where halide places the generator .h files

    file(GLOB GENS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/halide_funcs/*_generator.cxx")
    foreach(GEN_SRC ${GENS})
        string(REPLACE "_generator.cxx" "" GEN_NAME "${GEN_SRC}") #the gen_src will be halide_funcs/X_generator.cxx. Remove the generator and the halide_funcs
        string(REPLACE "halide_funcs/" "" GEN_NAME "${GEN_NAME}")
        # message("gen_name is " ${GEN_NAME} )
        halide_library("${GEN_NAME}"
                        SRCS ${GEN_SRC}
                        HALIDE_TARGET_FEATURES openglcompute profile 
                        # GENERATOR_DEPS  "-lX11"
                        FILTER_DEPS "-lX11 -lGL -lGLU"
                        # INCLUDES ${OPENGL_INCLUDE_DIRS}
                        EXTRA_OUTPUTS schedule html
                        GENERATOR_ARGS auto_schedule=false
                                        machine_params=32,16777216,40

            )
        target_link_libraries(surfel_renderer PUBLIC "${GEN_NAME}" ${HALIDE_LIBRARIES})

        #get the output of the .h files and include that too
        halide_generator_output_path(${GEN_NAME} halide_gen_path)
        # message("halide_gen_path ", ${halide_gen_path})
        include_directories(${halide_gen_path})
    endforeach()
endif()
