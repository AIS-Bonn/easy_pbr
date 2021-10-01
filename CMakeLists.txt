cmake_minimum_required(VERSION 2.8 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable

project(easypbr)



###   VARIABLES   ##############################################################
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -Wno-reorder") #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(CMAKE_CXX_STANDARD 17) #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
# option(CORE_WITH_VIEWER "Use a GL viewer, or if we don't have it available we will still publish to RViz" ON)
# option(CORE_WITH_HALIDE "Compile Halide" OFF)
# option(CORE_WITH_TORCH "Compile Torch" OFF)
# option(CORE_WITH_CUDA "Compile CUDA" OFF)
# option(CORE_WITH_GLM "With GLM for some quality of life functions in EasyGL" OFF)
# option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" OFF)



# libigl
# libigl options: choose between header only and compiled static library
# Header-only is preferred for small projects. For larger projects the static build
# considerably reduces the compilation times
option(LIBIGL_USE_STATIC_LIBRARY "Use libigl as static library" OFF)
option(LIBIGL_WITH_ANTTWEAKBAR       "Use AntTweakBar"    OFF)
option(LIBIGL_WITH_CGAL              "Use CGAL"           OFF)
option(LIBIGL_WITH_COMISO            "Use CoMiso"         OFF)
option(LIBIGL_WITH_CORK              "Use Cork"           OFF)
option(LIBIGL_WITH_EMBREE            "Use Embree"         OFF)
option(LIBIGL_WITH_LIM               "Use LIM"            OFF)
option(LIBIGL_WITH_MATLAB            "Use Matlab"         OFF)
option(LIBIGL_WITH_MOSEK             "Use MOSEK"          OFF)
option(LIBIGL_WITH_OPENGL            "Use OpenGL"         OFF)
option(LIBIGL_WITH_OPENGL_GLFW       "Use GLFW"           OFF)
option(LIBIGL_WITH_OPENGL_GLFW_IMGUI "Use ImGui"          OFF)
option(LIBIGL_WITH_PNG               "Use PNG"            OFF)
option(LIBIGL_WITH_PYTHON            "Use Python"         OFF)
option(LIBIGL_WITH_TETGEN            "Use Tetgen"         OFF)
option(LIBIGL_WITH_TRIANGLE          "Use Triangle"       OFF)
option(LIBIGL_WITH_VIEWER            "Use OpenGL viewer"  OFF)
option(LIBIGL_WITH_XML               "Use XML"            OFF)



######   PACKAGES   ############################################################
find_package(GLFW REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs )
find_package(LIBIGL REQUIRED)
find_package(PCL 1.3)
if(${PCL_FOUND})
    link_directories(${PCL_LIBRARY_DIRS})
    add_definitions(${PCL_DEFINITIONS})
endif()
find_package(kqueue)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD ) #Imgui will use glad loader
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/pybind11)
#try to compile with pytorch if you can
# set(PYTORCH_PATH "/opt/pytorch")
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ";${PYTORCH_PATH};")
# get and append paths for finding dep
execute_process( #do it like this https://github.com/facebookresearch/hanabi_SAD/blob/6e4ed590f5912fcb99633f4c224778a3ba78879b/rela/CMakeLists.txt#L10
  COMMAND python -c "import torch; import os; print(os.path.dirname(torch.__file__), end='')"
  OUTPUT_VARIABLE TorchPath
)
list(APPEND CMAKE_PREFIX_PATH ${TorchPath})
find_package(Torch)
# set(TORCH_FOUND False)






###   SOURCES   #################################################################
set(MY_SRC
    ${PROJECT_SOURCE_DIR}/src/Viewer.cxx
    ${PROJECT_SOURCE_DIR}/src/Gui.cxx
    ${PROJECT_SOURCE_DIR}/src/Camera.cxx
    ${PROJECT_SOURCE_DIR}/src/Recorder.cxx
    ${PROJECT_SOURCE_DIR}/src/Mesh.cxx
    ${PROJECT_SOURCE_DIR}/src/MeshGL.cxx
    ${PROJECT_SOURCE_DIR}/src/SpotLight.cxx
    ${PROJECT_SOURCE_DIR}/src/Scene.cxx
    ${PROJECT_SOURCE_DIR}/src/LabelMngr.cxx
    ${PROJECT_SOURCE_DIR}/src/Frame.cxx
)
file(GLOB IMGUI_SRC ${PROJECT_SOURCE_DIR}/deps/imgui/*.c* ${PROJECT_SOURCE_DIR}/deps/imgui/examples/imgui_impl_glfw.cpp ${PROJECT_SOURCE_DIR}/deps/imgui/examples/imgui_impl_opengl3.cpp ${PROJECT_SOURCE_DIR}/deps/imguizmo/ImGuizmo.cpp
)
#tinyply because libigl readPLY has a memory leak
set(DEPS_SRC ${DEPS_SRC} ${PROJECT_SOURCE_DIR}/deps/tiny_ply/source/tinyply.cpp)
# if(CORE_WITH_DIR_WATCHER)
if(${KQUEUE_FOUND})
    set(DEPS_SRC ${DEPS_SRC} ${PROJECT_SOURCE_DIR}/extern/dir_watcher/dir_watcher.cpp)
endif()




###   MAIN LIB  ####################
add_library( easypbr_cpp SHARED ${MY_SRC} extern/glad/glad.c ${DEPS_SRC}  ${IMGUI_SRC}  )


###   INCLUDES   #########################################################
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
                        ${PROJECT_SOURCE_DIR}/extern
                        ${PROJECT_SOURCE_DIR}/deps
                        ${PROJECT_SOURCE_DIR}/deps/imgui
                        ${PROJECT_SOURCE_DIR}/deps/imgui_ext
                        ${PROJECT_SOURCE_DIR}/deps/imgui/examples/
                        ${PROJECT_SOURCE_DIR}/deps/loguru
                        ${PROJECT_SOURCE_DIR}/deps/configuru
                        ${PROJECT_SOURCE_DIR}/deps/easy_gl
                        ${PROJECT_SOURCE_DIR}/deps/easy_pytorch
                        ${PROJECT_SOURCE_DIR}/deps/configuru
                        ${PROJECT_SOURCE_DIR}/deps/libigl/include
                        ${PROJECT_SOURCE_DIR}/deps/better_enums
                        ${PROJECT_SOURCE_DIR}/deps/imguizmo
                        ${PROJECT_SOURCE_DIR}/deps/concurrent_queue
                        ${PROJECT_SOURCE_DIR}/deps/pybind11/include
                        ${PROJECT_SOURCE_DIR}/deps/tiny_ply/source
                        ${PROJECT_SOURCE_DIR}/deps/tiny_obj
                        ${PROJECT_SOURCE_DIR}/deps/utils/include
                        ${PROJECT_SOURCE_DIR}/deps/nanoflann/include
                        ) # Header folder
target_include_directories(easypbr_cpp PUBLIC ${PROJECT_INCLUDE_DIR} )
target_include_directories(easypbr_cpp PUBLIC ${GLFW_INCLUDE_DIR})
# include_directories(${Boost_INCLUDE_DIR})
target_include_directories(easypbr_cpp PUBLIC ${EIGEN3_INCLUDE_DIR})
target_include_directories(easypbr_cpp PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(easypbr_cpp PUBLIC ${LIBIGL_INCLUDE_DIR})
target_include_directories(easypbr_cpp PUBLIC ${TORCH_INCLUDE_DIRS})
if(${PCL_FOUND})
    target_include_directories(easypbr_cpp PUBLIC ${PCL_INCLUDE_DIRS})
endif()
if(${KQUEUE_FOUND})
    target_include_directories(easypbr_cpp PUBLIC ${KQUEUE_INCLUDE_DIRS})
endif()







##pybind
pybind11_add_module(easypbr ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx )


###   EXECUTABLE   #######################################
add_executable(run_easypbr ${PROJECT_SOURCE_DIR}/src/main.cxx  )



###   SET ALL THE GLOBAL OPTIONS #########################################
if(${TORCH_FOUND})
    message("USING TORCH")
    target_compile_definitions(easypbr_cpp PUBLIC EASYPBR_WITH_TORCH)
else()
    message("NOT USING TORCH")
endif()
if(${KQUEUE_FOUND})
    message("USING DIR_WATCHER")
    target_compile_definitions(easypbr_cpp PUBLIC EASYPBR_WITH_DIR_WATCHER)
else()
    message("NOT USING DIR_WATCHER")
endif()
if(${PCL_FOUND})
    message("USING PCL")
    target_compile_definitions(easypbr_cpp PUBLIC EASYPBR_WITH_PCL)
else()
    message("NOT USING PCL")
endif()

#definitions for cmake variables that are necesarry during runtime
target_compile_definitions(easypbr_cpp PUBLIC EASYPBR_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
target_compile_definitions(easypbr_cpp PUBLIC EASYPBR_SHADERS_PATH="${PROJECT_SOURCE_DIR}/shaders")
target_compile_definitions(easypbr_cpp PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
target_compile_definitions(easypbr_cpp PUBLIC -DDEFAULT_CONFIG="${PROJECT_SOURCE_DIR}/config/default_params.cfg" )

target_compile_definitions(easypbr_cpp PRIVATE -DTRILIBRARY -DANSI_DECLARATORS)








###   LIBS   ###############################################
set(LIBS -lpthread -ldl) #because loguru needs them
if(${KQUEUE_FOUND})
    set(LIBS  ${LIBS} ${KQUEUE_LIBRARIES})
endif()
if(${TORCH_FOUND})
    set(LIBS ${LIBS} ${TORCH_LIBRARIES} )
    #torch 1.5.0 and above mess with pybind and we therefore need to link against libtorch_python.so also
    find_library(TORCH_PYTHON_LIBRARY torch_python PATHS "${TORCH_INSTALL_PREFIX}/lib")
    message(STATUS "TORCH_PYTHON_LIBRARY: ${TORCH_PYTHON_LIBRARY}")
    if(TORCH_PYTHON_LIBRARY)
        message(STATUS "Linking to torch_python_library")
        set(LIBS ${LIBS} ${TORCH_PYTHON_LIBRARY} )
    endif()
endif()
if(${PCL_FOUND})
    set(LIBS  ${LIBS} ${PCL_LIBRARIES})
endif()
# set(LIBS ${LIBS} Eigen3::Eigen  ${Boost_LIBRARIES}  igl::core   ${GLFW_LIBRARIES} ${OpenCV_LIBS} ${PCL_LIBRARIES}  )
set(LIBS ${LIBS} Eigen3::Eigen  igl::core ${Boost_LIBRARIES}  ${GLFW_LIBRARIES} ${OpenCV_LIBS}  )


target_link_libraries(easypbr_cpp PUBLIC ${LIBS} )
target_link_libraries(easypbr PRIVATE easypbr_cpp)
target_link_libraries(run_easypbr PRIVATE easypbr_cpp )
