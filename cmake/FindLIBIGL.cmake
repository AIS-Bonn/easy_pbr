# - Try to find the LIBIGL library
# Once done this will define
#
#  LIBIGL_FOUND - system has LIBIGL
#  LIBIGL_INCLUDE_DIR - **the** LIBIGL include directory
#  LIBIGL_INCLUDE_DIRS - LIBIGL include directories
#  LIBIGL_SOURCES - the LIBIGL source files

# if(NOT LIBIGL_FOUND)
# 	find_path(LIBIGL_INCLUDE_DIR
# 		NAMES igl/readOBJ.h
# 	   	PATHS /home/local/rosu/progs/libigl/include
# 		DOC "The libigl include directory"
# 		NO_DEFAULT_PATH)
#
# 	if(LIBIGL_INCLUDE_DIR)
# 	   set(LIBIGL_FOUND TRUE)
# 	   set(LIBIGL_INCLUDE_DIRS ${LIBIGL_INCLUDE_DIR})
# 	else()
# 	   message("+-------------------------------------------------+")
# 	   message("| libigl not found, please run download_libigl.sh |")
# 	   message("+-------------------------------------------------+")
# 	   message(FATAL_ERROR "")
# 	endif()
# endif()








if(LIBIGL_FOUND)
    return()
endif()

find_path(LIBIGL_INCLUDE_DIR igl/readOBJ.h
    HINTS
        ENV LIBIGL
        ENV LIBIGLROOT
        ENV LIBIGL_ROOT
        ENV LIBIGL_DIR
    PATHS
        ${PROJECT_SOURCE_DIR}/deps/libigl/include
    PATH_SUFFIXES include
)

if(LIBIGL_INCLUDE_DIR)
    set(LIBIGL_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBIGL
    "\nlibigl not found --- You can add it as a submodule it using:\n\tgit add submodule https://github.com/libigl/libigl.git deps/libigl"
    LIBIGL_INCLUDE_DIR)
mark_as_advanced(LIBIGL_INCLUDE_DIR)

list(APPEND CMAKE_MODULE_PATH "${LIBIGL_INCLUDE_DIR}/../cmake")
include(libigl)
