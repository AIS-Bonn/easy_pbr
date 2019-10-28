cmake_minimum_required(VERSION 3.14 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable

project(easy_pbr)


include(FetchContent)

###   VARIABLES   ##############################################################
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -Wall -O3") #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(PYTORCH_PATH "/opt/pytorch")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/ros/melodic/\;${PYTORCH_PATH}")
set(CMAKE_CXX_STANDARD 17) #needed also by the pybind so it can build itself in c++14 mode so we can use py:overload_cast
# set(FETCHCONTENT_QUIET NO)


####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
option(CORE_WITH_VIEWER "Use a GL viewer, or if we don't have it available we will still publish to RViz" ON)
option(CORE_WITH_HALIDE "Compile Halide" OFF)
option(CORE_WITH_TORCH "Compile Torch" ON)
option(CORE_WITH_CUDA "Compile CUDA" ON)
option(CORE_WITH_GLM "With GLM for some quality of life functions in EasyGL" OFF)
option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" ON)

#setting a python version to 3.6 for pybind so that the bindings work in 3.6 https://github.com/pybind/pybind11/issues/748
set(PYTHON_VERSION 3.6 CACHE STRING "" FORCE)
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




#pybind attempt 3

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
find_package(OpenCV REQUIRED)
find_package(kqueue REQUIRED)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.73
)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD ) #Imgui will use glad loader
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
)
FetchContent_Declare(
  libigl
  GIT_REPOSITORY https://github.com/libigl/libigl.git
  GIT_TAG        v2.1.0
)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.4.2
)
FetchContent_Declare(
  loguru
  GIT_REPOSITORY https://github.com/RaduAlexandru/loguru.git
)
FetchContent_Declare(
  configuru
  GIT_REPOSITORY https://github.com/RaduAlexandru/Configuru.git
)
FetchContent_Declare(
  better_enums
  GIT_REPOSITORY https://github.com/aantron/better-enums.git
  GIT_TAG        0.11.2
)
FetchContent_Declare(
  concurrent_queue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
)
FetchContent_Declare(
  tiny_ply
  GIT_REPOSITORY https://github.com/ddiakopoulos/tinyply.git
  GIT_TAG        2.2
)
FetchContent_GetProperties(tiny_ply)
if(NOT tiny_ply_POPULATED)
  FetchContent_Populate(tiny_ply)
endif()
FetchContent_Declare(
  tiny_obj
  GIT_REPOSITORY https://github.com/syoyo/tinyobjloader
  GIT_TAG        v2.0.0-rc1 
)
FetchContent_Declare(
  utils
  GIT_REPOSITORY https://github.com/RaduAlexandru/utils.git
)
FetchContent_Declare(
  easy_gl
  GIT_REPOSITORY https://github.com/RaduAlexandru/EasyGL.git
)
# add_subdirectory( ${pybind11_SOURCE_DIR})
##fetched content make availalbe. Order is important. It should go from the one with lest dependencies to the one with most. So if A depend on B, then MakeAvailable should have B first
FetchContent_MakeAvailable(imgui imguizmo pybind11 libigl pybind11 loguru configuru better_enums concurrent_queue tiny_obj  utils easy_gl)


###   INCLUDES   #########################################################
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/extern)
include_directories(${GLFW_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${KQUEUE_INCLUDE_DIRS})
include_directories(${LIBIGL_INCLUDE_DIR})
include_directories(${pybind11_SOURCE_DIR}/include)
include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/examples )
include_directories(${imguizmo_SOURCE_DIR})
include_directories(${loguru_SOURCE_DIR})
include_directories(${configuru_SOURCE_DIR})
include_directories(${better_enums_SOURCE_DIR})
include_directories(${concurrent_queue_SOURCE_DIR})
include_directories(${tiny_ply_SOURCE_DIR}/source)
include_directories(${tiny_obj_SOURCE_DIR})
include_directories(${utils_SOURCE_DIR}/include)
include_directories(${easy_gl_SOURCE_DIR})
include_directories("/usr/include/python3.6m/")



###   SOURCES   #################################################################
set(MY_SRC  
    # ${PROJECT_SOURCE_DIR}/src/main.cxx
    ${PROJECT_SOURCE_DIR}/src/Viewer.cxx
    ${PROJECT_SOURCE_DIR}/src/Gui.cxx
    ${PROJECT_SOURCE_DIR}/src/Camera.cxx
    ${PROJECT_SOURCE_DIR}/src/Recorder.cxx
    ${PROJECT_SOURCE_DIR}/src/Mesh.cxx
    ${PROJECT_SOURCE_DIR}/src/MeshGL.cxx
    ${PROJECT_SOURCE_DIR}/src/SpotLight.cxx
    ${PROJECT_SOURCE_DIR}/src/Scene.cxx
    ${PROJECT_SOURCE_DIR}/src/LabelMngr.cxx
    # ${PROJECT_SOURCE_DIR}/src/SyntheticGenerator.cxx
    # ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx
)
file(GLOB IMGUI_SRC ${imgui_SOURCE_DIR}/*.c* ${imgui_SOURCE_DIR}/examples/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
#tinyply because libigl readPLY has a memory leak 
set(DEPS_SRC ${DEPS_SRC} ${tiny_ply_SOURCE_DIR}/source/tinyply.cpp)
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

###   MAIN LIB  ####################
# add_library( easy_pbr ${MY_SRC}  deps/glad/glad.c ${DEPS_SRC} ${IMGUI_SRC}  )
add_library( easy_pbr_lib ${MY_SRC} extern/glad/glad.c ${DEPS_SRC}  ${IMGUI_SRC}  )
set_target_properties(easy_pbr_lib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

##pybind
pybind11_add_module(EasyPBR ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx )
# target_compile_definitions(Viewer_py PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

###   EXECUTABLE   #######################################
add_executable(run ${PROJECT_SOURCE_DIR}/src/main.cxx  )
# set_target_properties(surfel_renderer PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
# add_executable(easy_pbr  ${EXTERN_SRC} ${IMGUI_SRC} src/main.cxx)


###   LIBS   ###############################################
set(LIBS -lpthread -ldl) #because loguru needs them
set(LIBS ${LIBS} Eigen3::Eigen  ${GLFW_LIBRARIES} ${OpenCV_LIBS} ${KQUEUE_LIBRARIES} ${Boost_LIBRARIES} igl::core )


target_link_libraries(easy_pbr_lib ${LIBS} )

# # ######just liktit to easy_PBR but use the same bybridge
# add_library( synthetic_generator_lib ${PROJECT_SOURCE_DIR}/src/SyntheticGenerator.cxx  )
# set_target_properties(synthetic_generator_lib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# target_link_libraries(synthetic_generator_lib easy_pbr_lib )
# target_compile_definitions(synthetic_generator_lib PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
# target_compile_definitions(synthetic_generator_lib PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr


# target_link_libraries(EasyPBR PRIVATE easy_pbr_lib synthetic_generator_lib )
target_link_libraries(EasyPBR PRIVATE easy_pbr_lib )
target_link_libraries(run PRIVATE easy_pbr_lib )


#definitions for cmake variables that are necesarry during runtime
target_compile_definitions(easy_pbr_lib PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
target_compile_definitions(easy_pbr_lib PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr
target_compile_definitions(easy_pbr_lib PRIVATE AWESOMEFONT_DIR="${PROJECT_SOURCE_DIR}/extern/imgui_ext/fontawesome-webfont.ttf")
target_compile_definitions(easy_pbr_lib PRIVATE PROGGY_DIR="${imgui_SOURCE_DIR}/misc/fonts/ProggyClean.ttf")
target_compile_definitions(easy_pbr_lib PRIVATE -DTRILIBRARY -DANSI_DECLARATORS)


#################################################################################
##compile the Synthetic generator as another separate thing
add_library( synthetic_generator_lib ${PROJECT_SOURCE_DIR}/src/SyntheticGenerator.cxx  )
set_target_properties(synthetic_generator_lib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
pybind11_add_module(MBZIRC_CH1 ${PROJECT_SOURCE_DIR}/src/PyBridgeSynth.cxx )

target_link_libraries(synthetic_generator_lib easy_pbr_lib )
target_link_libraries(MBZIRC_CH1 PRIVATE synthetic_generator_lib )

target_compile_definitions(synthetic_generator_lib PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
target_compile_definitions(synthetic_generator_lib PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr

