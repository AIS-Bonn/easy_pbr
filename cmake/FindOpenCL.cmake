##******************************************************************************
##  Copyright(C) 2014-2017 Intel Corporation. All Rights Reserved.
##
##  The source code, information  and  material ("Material") contained herein is
##  owned  by Intel Corporation or its suppliers or licensors, and title to such
##  Material remains  with Intel Corporation  or its suppliers or licensors. The
##  Material  contains proprietary information  of  Intel or  its  suppliers and
##  licensors. The  Material is protected by worldwide copyright laws and treaty
##  provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
##  modified, published, uploaded, posted, transmitted, distributed or disclosed
##  in any way  without Intel's  prior  express written  permission. No  license
##  under  any patent, copyright  or  other intellectual property rights  in the
##  Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
##  implication, inducement,  estoppel or  otherwise.  Any  license  under  such
##  intellectual  property  rights must  be express  and  approved  by  Intel in
##  writing.
##
##  *Third Party trademarks are the property of their respective owners.
##
##  Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
##  this  notice or  any other notice embedded  in Materials by Intel or Intel's
##  suppliers or licensors in any way.
##
##******************************************************************************
##  Content: Intel(R) Media SDK Samples projects creation and build
##******************************************************************************

find_path( OPENCL_INCLUDE CL/opencl.h PATHS /usr/include /opt/intel/opencl/include)
find_library( OPENCL_LIBRARY libOpenCL.so PATHS /usr/lib /opt/intel/opencl/)

set( OCL_LIBS "" )

if ( NOT OPENCL_INCLUDE MATCHES NOTFOUND )
    if ( NOT OPENCL_LIBRARY MATCHES NOTFOUND )
        set ( OPENCL_FOUND TRUE )

        get_filename_component( OPENCL_LIBRARY_PATH ${OPENCL_LIBRARY} PATH )

        list( APPEND OPENCL_LIBS OpenCL )
    endif()
endif()

if ( NOT DEFINED OPENCL_FOUND )
    message( STATUS "OpenCL was not found (optional). The following will not be built: rotate_opencl plugin.")
else ()
    message( STATUS "OpenCL was found here: ${OPENCL_LIBRARY_PATH} and ${OPENCL_INCLUDE}" )
endif()
