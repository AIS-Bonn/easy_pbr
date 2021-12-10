cmake_minimum_required(VERSION 2.8 FATAL_ERROR) # we need at least 3.11 because we use the FetchContent feature to manage dependencies and at least 3.14 because we use FetchContent_MakeAvailable

project(easypbr_wrapper)


# include(FetchContent)

###   VARIABLES   ##############################################################
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -Wall -O3") #we need c++17 because this solves alignment issues with eigen http://eigen.tuxfamily.org/bz/show_bug.cgi?id=1409
set(CMAKE_CXX_STANDARD 17) #needed also by the pybind so it can build itself in c++14 mode so we can use py:overload_cast
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


####   GLOBAL OPTIONS   ###https://stackoverflow.com/questions/15201064/cmake-conditional-preprocessor-define-on-code
# option(CORE_WITH_DIR_WATCHER "Compile with the dir_watcher dependency from emildb" ON)


######   PACKAGES   ############################################################
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(OpenCV REQUIRED)
find_package(EasyPBR REQUIRED)
find_package(kqueue)
add_subdirectory(${EASYPBR_SRC_PATH}/deps/pybind11  [EXCLUDE_FROM_ALL])







###   SOURCES   #################################################################
set(MY_SRC
    ${PROJECT_SOURCE_DIR}/src/EasyPBRwrapper.cxx
)
if(${KQUEUE_FOUND})
    set(DEPS_SRC ${DEPS_SRC} ${EASYPBR_SRC_PATH}/extern/dir_watcher/dir_watcher.cpp)
endif()




#main lib
add_library( easypbr_wrapper_cpp SHARED ${MY_SRC}  ${DEPS_SRC}  )



###   INCLUDES   #########################################################
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
                        ${PROJECT_SOURCE_DIR}/deps
                        ${PROJECT_SOURCE_DIR}/deps/json/include
                        ) # Header folder
target_include_directories(easypbr_wrapper_cpp PUBLIC ${PROJECT_INCLUDE_DIR} )
target_include_directories(easypbr_wrapper_cpp PUBLIC ${GLFW_INCLUDE_DIR})
target_include_directories(easypbr_wrapper_cpp PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(easypbr_wrapper_cpp PUBLIC ${EIGEN3_INCLUDE_DIR})
target_include_directories(easypbr_wrapper_cpp PUBLIC ${EASYPBR_INCLUDE_DIR})
target_include_directories(easypbr_wrapper_cpp PUBLIC ${KQUEUE_INCLUDE_DIRS})


##pybind
pybind11_add_module(easypbr_wrapper ${PROJECT_SOURCE_DIR}/src/PyBridge.cxx )

###   EXECUTABLE   #######################################
add_executable(run_easypbr_wrapper ${PROJECT_SOURCE_DIR}/src/main.cxx
)




###   SET ALL THE GLOBAL OPTIONS #########################################
if(${KQUEUE_FOUND})
    message("USING DIR_WATCHER")
    target_compile_definitions(easypbr_wrapper_cpp PUBLIC WITH_DIR_WATCHER)
else()
    message("NOT USING DIR_WATCHER")
endif()

#definitions for cmake variables that are necesarry during runtime
target_compile_definitions(easypbr_wrapper_cpp PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr
target_compile_definitions(easypbr_wrapper_cpp PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}") # points to the CMakeList folder of whichever project included easy_pbr
target_compile_definitions(run_easypbr_wrapper PRIVATE PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}") #point to the cmakelist folder of the easy_pbr






###   LIBS   ###############################################
set(LIBS -lpthread -ldl) #because loguru needs them
if(${KQUEUE_FOUND})
    set(LIBS  ${LIBS} ${KQUEUE_LIBRARIES})
endif()
set(LIBS ${LIBS} ${OpenCV_LIBS}  ${Boost_LIBRARIES}  ${EASYPBR_LIBRARY}  )


target_link_libraries(easypbr_wrapper_cpp ${LIBS} )
target_link_libraries(easypbr_wrapper PRIVATE easypbr_wrapper_cpp )
target_link_libraries(run_easypbr_wrapper PRIVATE easypbr_wrapper_cpp  )
