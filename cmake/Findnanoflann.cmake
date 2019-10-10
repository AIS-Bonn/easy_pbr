# - Try to find the nanoflann library
# Once done this will define
#
#  nanoflann_FOUND - system has nanoflann
#  nanoflann_INCLUDE_DIR - **the** nanoflann include directory
#  nanoflann_INCLUDE_DIRS - nanoflann include directories
#  nanoflann_SOURCES - the nanoflann source files

if(NOT nanoflann_FOUND)
	find_path(nanoflann_INCLUDE_DIR
		NAMES nanoflann.hpp
	   	PATHS ${PROJECT_SOURCE_DIR}/deps/nanoflann/include
		DOC "The nanoflann include directory"
		NO_DEFAULT_PATH)

	if(nanoflann_INCLUDE_DIR)
	   set(nanoflann_FOUND TRUE)
	   set(nanoflann_INCLUDE_DIRS ${nanoflann_INCLUDE_DIR})
	else()
	   message("+-------------------------------------------------+")
	   message("| nanoflann not found, please run download_nanoflann.sh |")
	   message("+-------------------------------------------------+")
	   message(FATAL_ERROR "")
	endif()
endif()
