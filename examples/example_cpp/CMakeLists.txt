cmake_minimum_required(VERSION 2.8 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable

project(easypbr_wrapper)


# include(FetchContent)

###   VARIABLES   ##############################################################
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -Wall -O3") #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(PYTORCH_PATH "/opt/pytorch")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/ros/melodic/\;${PYTORCH_PATH}")
set(CMAKE_CXX_STANDARD 17) #needed also by the pybind so it can build itself in c++14 mode so we can use py:overload_cast
# set(FETCHCONTENT_QUIET NO)


####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
# option(CORE_WITH_VIEWER "Use a GL viewer, or if we don't have it available we will still publish to RViz" ON)
# option(CORE_WITH_HALIDE "Compile Halide" OFF)
# option(CORE_WITH_CUDA "Compile CUDA" OFF)
# option(CORE_WITH_GLM "With GLM for some quality of life functions in EasyGL" OFF)
# option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" OFF)


######   PACKAGES   ############################################################
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(OpenCV REQUIRED)
find_package(EasyPBR REQUIRED)
add_subdirectory(${EASYPBR_SRC_PATH}/deps/pybind11  [EXCLUDE_FROM_ALL])




###   INCLUDES   #########################################################
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
                        ${PROJECT_SOURCE_DIR}/deps
                        ${PROJECT_SOURCE_DIR}/deps/json/include
                        ) # Header folder
include_directories(${PROJECT_INCLUDE_DIR} )
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${EASYPBR_INCLUDE_DIR})


###   SOURCES   #################################################################
set(MY_SRC  
    ${PROJECT_SOURCE_DIR}/src/EasyPBRwrapper.cxx
)


###   SET ALL THE GLOBAL OPTIONS #########################################
# if(CORE_WITH_VIEWER)
#     message("USING GL Viewer")
#     add_definitions(-DWITH_VIEWER)
# else()
#     message("USING OFFSCREEN")
# endif()
# if(CORE_WITH_HALIDE)
#     message("USING HALIDE")
#     add_definitions(-DWITH_HALIDE)
# else()
#     message("NOT USING HALIDE")
# endif()
# if(CORE_WITH_CUDA)
#     message("USING CUDA")
#     add_definitions(-DWITH_CUDA)
# else()
#     message("NOT USING CUDA")
# endif()
# if(CORE_WITH_GLM)
#     message("USING GLM")
#     add_definitions(-DWITH_GLM)
# else()
#     message("NOT USING GLM")
# endif()
# if(CORE_WITH_DIR_WATCHER)
#     message("USING DIR_WATCHER")
#     add_definitions(-DWITH_DIR_WATCHER)
# else()
#     message("NOT USING DIR_WATCHER")
# endif()

#lib 
add_library( all_libs SHARED ${MY_SRC}  ${DEPS_SRC}  )


##pybind
pybind11_add_module(easypbr_wrapper ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx )

###   EXECUTABLE   #######################################
add_executable(run_easypbr_wrapper ${PROJECT_SOURCE_DIR}/src/main.cxx  )


###   LIBS   ###############################################
set(LIBS ${LIBS} ${EASYPBR_LIBRARY} ${OpenCV_LIBS})


target_link_libraries(all_libs ${LIBS} )
target_link_libraries(easypbr_wrapper PRIVATE all_libs )
target_link_libraries(run_easypbr_wrapper PRIVATE all_libs  )


#definitions for cmake variables that are necesarry during runtime
target_compile_definitions(all_libs PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
target_compile_definitions(all_libs PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr

