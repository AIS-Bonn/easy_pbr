# cmake_minimum_required(VERSION 3.14 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable
cmake_minimum_required(VERSION 2.8 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable

project(easypbr)


# include(FetchContent)

###   VARIABLES   ##############################################################
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -O3") #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(CMAKE_CXX_STANDARD 17) #needed also by the pybind so it can build itself in c++14 mode so we can use py:overload_cast



####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
option(CORE_WITH_VIEWER "Use a GL viewer, or if we don't have it available we will still publish to RViz" ON)
option(CORE_WITH_HALIDE "Compile Halide" OFF)
option(CORE_WITH_TORCH "Compile Torch" OFF)
option(CORE_WITH_CUDA "Compile CUDA" OFF)
option(CORE_WITH_GLM "With GLM for some quality of life functions in EasyGL" OFF)
option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" OFF)



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
# find_package(PythonLibs REQUIRED)
find_package(GLFW REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(OpenCV REQUIRED)
find_package(LIBIGL REQUIRED)
find_package(PCL 1.3 REQUIRED COMPONENTS common io)
if(CORE_WITH_DIR_WATCHER)
    find_package(kqueue REQUIRED)
endif()
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD ) #Imgui will use glad loader
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/pybind11)
#try to compile with pytorch if you can 
set(PYTORCH_PATH "/opt/pytorch")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ";${PYTORCH_PATH};")
find_package(Torch)
# if(${TORCH_FOUND})
#     # option(CORE_WITH_TORCH "Compile PyTorch" ON)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} ") #to avoid https://github.com/pytorch/pytorch/issues/13541
# else()
#     # option(CORE_WITH_TORCH "Compile PyTorch" OFF)
#     # message("TORCH WAS FOUND")
# endif()
# message("TORCH_FOUND is ", ${TORCH_FOUND})
# message("CMAKE_CXX_FLAGS flags are ", ${CMAKE_CXX_FLAGS})
# message("TORCH_CXX_FLAGS flags are ", ${TORCH_CXX_FLAGS})




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
                        ${PROJECT_SOURCE_DIR}/deps/configuru
                        ${PROJECT_SOURCE_DIR}/deps/libigl/include
                        ${PROJECT_SOURCE_DIR}/deps/better_enums
                        ${PROJECT_SOURCE_DIR}/deps/imguizmo
                        ${PROJECT_SOURCE_DIR}/deps/concurrent_queue
                        ${PROJECT_SOURCE_DIR}/deps/pybind11/include
                        ${PROJECT_SOURCE_DIR}/deps/tiny_ply/source
                        ${PROJECT_SOURCE_DIR}/deps/tiny_obj
                        ${PROJECT_SOURCE_DIR}/deps/utils/include
                        ) # Header folder
include_directories(${PROJECT_INCLUDE_DIR} )
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${LIBIGL_INCLUDE_DIR})
include_directories(${KQUEUE_INCLUDE_DIRS})
include_directories(${PCL_INCLUDE_DIRS})
include_directories(${TORCH_INCLUDE_DIRS})
# message("torch include dir is " ${TORCH_INCLUDE_DIRS})




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
# file(GLOB IMGUI_SRC ${imgui_SOURCE_DIR}/*.c* ${imgui_SOURCE_DIR}/examples/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
file(GLOB IMGUI_SRC ${PROJECT_SOURCE_DIR}/deps/imgui/*.c* ${PROJECT_SOURCE_DIR}/deps/imgui/examples/imgui_impl_glfw.cpp ${PROJECT_SOURCE_DIR}/deps/imgui/examples/imgui_impl_opengl3.cpp ${PROJECT_SOURCE_DIR}/deps/imguizmo/ImGuizmo.cpp
)
#tinyply because libigl readPLY has a memory leak 
set(DEPS_SRC ${DEPS_SRC} ${PROJECT_SOURCE_DIR}/deps/tiny_ply/source/tinyply.cpp)
if(CORE_WITH_DIR_WATCHER)
    set(DEPS_SRC ${DEPS_SRC} ${PROJECT_SOURCE_DIR}/extern/dir_watcher/dir_watcher.cpp)
endif()


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
if(${TORCH_FOUND})
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

###   MAIN LIB  ####################
# add_library( easy_pbr ${MY_SRC}  deps/glad/glad.c ${DEPS_SRC} ${IMGUI_SRC}  )
add_library( easypbr_cpp SHARED ${MY_SRC} extern/glad/glad.c ${DEPS_SRC}  ${IMGUI_SRC}  )

##pybind
pybind11_add_module(easypbr ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx )
#pybind with a supporting shared library https://github.com/pybind/cmake_example/issues/11
# set_target_properties(easypbr PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
# set_target_properties(easypbr PROPERTIES SKIP_BUILD_RPATH FALSE)
set_target_properties(easypbr PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
# message("--------origin-----------$ORIGIN", ${ORIGIN})
message("--------SHADERS_PATH-----------------------------", ${EASYPBR_SHADERS_PATH})
if(UNIX AND NOT APPLE)
    # set_target_properties(easypbr PROPERTIES INSTALL_RPATH "$ORIGIN/relative/path/to/supporting/library_dir")
    set_target_properties(easypbr PROPERTIES INSTALL_RPATH "$ORIGIN/")
elseif(APPLE)
    # set_target_properties(easypbr PROPERTIES INSTALL_RPATH "@loader_path/relative/path/to/supporting/library_dir")
    set_target_properties(easypbr PROPERTIES INSTALL_RPATH "@loader_path/")
endif()
set_target_properties(easypbr PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

###   EXECUTABLE   #######################################
add_executable(run_easypbr ${PROJECT_SOURCE_DIR}/src/main.cxx  )


###   LIBS   ###############################################
set(LIBS -lpthread -ldl) #because loguru needs them
if(CORE_WITH_DIR_WATCHER)
    set(LIBS  ${LIBS} ${KQUEUE_LIBRARIES})
endif()
if(${TORCH_FOUND})
    set(LIBS ${LIBS} ${TORCH_LIBRARIES} )
endif()
set(LIBS ${LIBS} Eigen3::Eigen ${GLFW_LIBRARIES} ${OpenCV_LIBS} ${Boost_LIBRARIES} ${PCL_LIBRARIES} igl::core  )


target_link_libraries(easypbr_cpp ${LIBS} )
target_link_libraries(easypbr PRIVATE easypbr_cpp)
target_link_libraries(run_easypbr PRIVATE easypbr_cpp )


#definitions for cmake variables that are necesarry during runtime
# target_compile_definitions(easypbr_cpp PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
# target_compile_definitions(easypbr_cpp PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr
# target_compile_definitions(easypbr_cpp PRIVATE AWESOMEFONT_DIR="${PROJECT_SOURCE_DIR}/extern/imgui_ext/fontawesome-webfont.ttf")
# target_compile_definitions(easypbr_cpp PRIVATE PROGGY_DIR="${imgui_SOURCE_DIR}/misc/fonts/ProggyClean.ttf")

#  target_compile_definitions(easypbr_cpp PRIVATE DATA_DIR="${DATA_DIR}")
# target_compile_definitions(easypbr_cpp PRIVATE EASYPBR_SHADERS_PATH="${EASYPBR_SHADERS_PATH}")

target_compile_definitions(easypbr_cpp PRIVATE EASYPBR_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
target_compile_definitions(easypbr_cpp PRIVATE EASYPBR_SHADERS_PATH="${PROJECT_SOURCE_DIR}/shaders")
# target_compile_definitions(easypbr_cpp PRIVATE DEFAULT_CONFIG="${PROJECT_SOURCE_DIR}/config/default_params.cfg")
target_compile_definitions(easypbr_cpp PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
add_definitions( -DDEFAULT_CONFIG="${PROJECT_SOURCE_DIR}/config/default_params.cfg" )

target_compile_definitions(easypbr_cpp PRIVATE -DTRILIBRARY -DANSI_DECLARATORS)


#install the libraries in a default search path
# message("INSTALLING IN " ${CMAKE_INSTALL_LIBDIR})
# install(TARGETS easypbr DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(TARGETS easypbr_cpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(TARGETS run_easypbr DESTINATION bin)


# install(TARGETS easypbr
#     LIBRARY DESTINATION lib
# )
# install(TARGETS easypbr_cpp
#     LIBRARY DESTINATION lib
# )
# install(DIRECTORY
#     include/easy_pbr/
#     DESTINATION include/easy_pbr
# )



