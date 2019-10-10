# - Try to find the HALIDE library
# Once done this will define
#
#  HALIDE_FOUND - system has HALIDE
#  HALIDE_INCLUDE_DIR - **the** HALIDE include directory
#  HALIDE_INCLUDE_DIRS - HALIDE include directories
#  HALIDE_SOURCES - the HALIDE source files

# if(NOT HALIDE_FOUND)
# 	find_path(HALIDE_INCLUDE_DIR
# 		NAMES Halide.h
# 	   	PATHS /media/alex/Data/Programs_linux/Halide/build/include
# 		DOC "The Halide include directory"
# 		NO_DEFAULT_PATH)
#
# 	if(HALIDE_INCLUDE_DIR)
# 	   set(HALIDE_FOUND TRUE)
# 	   set(HALIDE_INCLUDE_DIRS ${HALIDE_INCLUDE_DIR})
# 	else()
# 	   message("+-------------------------------------------------+")
# 	   message("| Halide include not found						  |")
# 	   message("+-------------------------------------------------+")
# 	   message(FATAL_ERROR "")
# 	endif()
#
#
# 	#library
# 	find_library(HALIDE_LIBRARY_DIR
# 		NAMES libHalide.so
# 	   	HINTS /media/alex/Data/Programs_linux/Halide/build/lib/
# 		DOC "The Halide lib directory"
# 		NO_DEFAULT_PATH)
#
# 	if(HALIDE_LIBRARY_DIR)
# 	   set(HALIDE_LIBRARIES "${HALIDE_LIBRARY_DIR}")
# 	else()
# 	  message("+-------------------------------------------------+")
# 	  message("| Halide library not found                   	 |")
# 	  message("+-------------------------------------------------+")
# 	  message(FATAL_ERROR "")
# 	endif()
#
# 	set(HALIDE_DISTRIB_DIR "/media/alex/Data/Programs_linux/Halide")
# 	list(APPEND CMAKE_MODULE_PATH "${HALIDE_INCLUDE_DIR}/../../")
# 	include(halide)
#
# endif()






# #From https://github.com/fish2000/libimread/blob/master/cmake/FindHalide.cmake
# find_path(HALIDE_ROOT_DIR
#     NAMES include/Halide.h include/HalideRuntime.h
# 	PATHS /media/alex/Data/Programs_linux/Halide/build/
# )
#
# find_library(HALIDE_LIBRARIES
#     NAMES Halide
#     HINTS ${HALIDE_ROOT_DIR}/lib
# )
#
# find_path(HALIDE_INCLUDE_DIR
#     NAMES Halide.h HalideRuntime.h
#     HINTS ${HALIDE_ROOT_DIR}/include
# )
#
# include(FindPackageHandleStandardArgs)
# find_package_handle_standard_args(Halide DEFAULT_MSG
#     HALIDE_LIBRARIES
#     HALIDE_INCLUDE_DIR
# )
#
# set(HALIDE_LIBRARY HALIDE_LIBRARIES)
# set(HALIDE_INCLUDE_DIRS HALIDE_INCLUDE_DIR)
#
# mark_as_advanced(
#     HALIDE_ROOT_DIR
#     HALIDE_LIBRARY
#     HALIDE_LIBRARIES
#     HALIDE_INCLUDE_DIR
#     HALIDE_INCLUDE_DIRS
# )
#
# #thi works
# # set(HALIDE_DISTRIB_DIR "${HALIDE_ROOT_DIR}")
# # include("${HALIDE_DISTRIB_DIR}/halide.cmake")
#
# set(HALIDE_DISTRIB_DIR "${HALIDE_ROOT_DIR}/../")
# include("${CMAKE_SOURCE_DIR}/cmake/halide.cmake")




####again after rebuilding halide
#From https://github.com/fish2000/libimread/blob/master/cmake/FindHalide.cmake
find_path(HALIDE_ROOT_DIR
    NAMES include/Halide.h include/HalideRuntime.h
	PATHS /media/alex/Data/Programs_linux/Halide/distrib/
)

find_library(HALIDE_LIBRARIES
    NAMES Halide
    HINTS ${HALIDE_ROOT_DIR}/lib
)

find_path(HALIDE_INCLUDE_DIR
    NAMES Halide.h HalideRuntime.h
    HINTS ${HALIDE_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Halide DEFAULT_MSG
    HALIDE_LIBRARIES
    HALIDE_INCLUDE_DIR
)

set(HALIDE_LIBRARY HALIDE_LIBRARIES)
set(HALIDE_INCLUDE_DIRS HALIDE_INCLUDE_DIR)

mark_as_advanced(
    HALIDE_ROOT_DIR
    HALIDE_LIBRARY
    HALIDE_LIBRARIES
    HALIDE_INCLUDE_DIR
    HALIDE_INCLUDE_DIRS
)

#thi works
# set(HALIDE_DISTRIB_DIR "${HALIDE_ROOT_DIR}")
# include("${HALIDE_DISTRIB_DIR}/halide.cmake")

set(HALIDE_DISTRIB_DIR "${HALIDE_ROOT_DIR}")
include("${HALIDE_ROOT_DIR}/../halide.cmake")
