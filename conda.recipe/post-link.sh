#!/bin/bash

# echo POST----------------------------------------------------------------------------- > $PREFIX/.messages.txt
echo  post_link_copy > $PREFIX/.messages.txt
# echo $PY_VER  >> $PREFIX/.messages.txt
# cd $PREFIX/lib/python${PY_VER}/site-packages/
# cd $PREFIX/lib/python3.8/site-packages
cd $PREFIX/lib
# ls >> $PREFIX/.messages.txt
# find ./ -name "libeasypbr_cpp.so" >> $PREFIX/.messages.txt
EASYPBR_CPP_PATH=$(find ./ -name libeasypbr_cpp.so)
NR_FINDS=${find ./ -name libeasypbr_cpp.so | grep . -c}
# echo $EASYPBR_CPP_PATH>> $PREFIX/.messages.txt
if [ $NR_FINDS -eq 1 ]
    then
        mv "$EASYPBR_CPP_PATH" $PREFIX/lib
fi
# pwd >> $PREFIX/.messages.txt
# dmesg
