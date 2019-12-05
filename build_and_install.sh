#!/bin/bash

#https://grokbase.com/t/python/distutils-sig/103351xenv/distutils-installing-header-files-with-setuptools
python setup.py install --user --single-version-externally-managed --record=record.txt

# https://stackoverflow.com/questions/50101740/install-header-only-library-with-python
# python3 -m pip install --user ./