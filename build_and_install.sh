#!/bin/bash

#https://grokbase.com/t/python/distutils-sig/103351xenv/distutils-installing-header-files-with-setuptools
#this instalation works and it can run with differnt python files that call the easypbr
#however it cannot be disinstalled with pip uninstalla and the header file are left lingering
# python setup.py install --user --single-version-externally-managed --record=record.txt

# https://stackoverflow.com/questions/50101740/install-header-only-library-with-python
#installs correclty and can also be deinstalled completely with pip uninstall
#however it doesnt find  the libeasypbr_cpp.so so wwe might need to set some rpaths in cmake
# python3 -m pip install --user ./ -v
# python3 -m pip install --progress-bar pretty -vvv --prefix=$HOME/local ./ 



python3 -m pip install -v --user --editable ./ 
#https://github.com/pypa/pip/issues/3246
#python3 -m pip install -v --editable ./ 

# python setup.py develop --user  --install-dir=./lib --build-directory=./build