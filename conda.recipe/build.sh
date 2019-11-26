#!/bin/bash
unset MACOSX_DEPLOYMENT_TARGET
${PYTHON} setup.py install --single-version-externally-managed --record=record.txt

# LIBRARY_PATH="${PREFIX}/lib"
# mkdir -p build
# cd build
# cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_INSTALL_LIBDIR=lib

# make -j6
# make install
