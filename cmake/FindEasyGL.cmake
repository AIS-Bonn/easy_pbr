if(EasyGL_FOUND)
    return()
endif()

find_path(EasyGL_INCLUDE_DIR easy_gl/Texture2D.h
    PATHS
        ${PROJECT_SOURCE_DIR}/deps/easy_gl/include
    PATH_SUFFIXES include
)

if(EasyGL_INCLUDE_DIR)
    set(EasyGL_FOUND TRUE)
endif()


list(APPEND CMAKE_MODULE_PATH "${EasyGL_INCLUDE_DIR}/../cmake")
message("CMAKE_MODULE_PATH" ${CMAKE_MODULE_PATH} )
include(easygl)
