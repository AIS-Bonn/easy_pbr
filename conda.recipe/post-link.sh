#!/bin/bash

echo "==================POST-----------------------------------------------------------------------------" > $PREFIX/.messages.txt
cd $PREFIX/lib
# ls >> $PREFIX/.messages.txt
# find ./ -name "libeasypbr_cpp.so" >> $PREFIX/.messages.txt
EASYPBR_CPP_PATH=$(find ./ -name libeasypbr_cpp.so)
# NR_FINDS=${find ./ -name libeasypbr_cpp.so | grep . -c}
# if [ $NR_FINDS -eq 1 ]
    # then
        # echo "MOVING"  >> $PREFIX/.messages.txt
mv "$EASYPBR_CPP_PATH" $PREFIX/lib
