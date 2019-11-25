#!/bin/bash
unset MACOSX_DEPLOYMENT_TARGET
${PYTHON} setup.py install;


# LIBRARY_PATH="${PREFIX}/lib"
# INCLUDE_PATH="${PREFIX}/include"

# mkdir -p build
# cd build
# cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_INSTALL_LIBDIR="${LIBRARY_PATH}"

# make -j6
# make install