#!/bin/bash

echo "==================POST-----------------------------------------------------------------------------" > $PREFIX/.messages.txt

#copy the config and the shaders into the build prefix
ls >> $PREFIX/.messages.txt




# #move the libeasypbr_cpp to a place where the linker can find it, alternativelly we could have also set rpaths inside the meta.yaml
# cd $PREFIX/lib/python3.6/
# # ls >> $PREFIX/.messages.txt
# # find ./ -name "libeasypbr_cpp.so" >> $PREFIX/.messages.txt
# EASYPBR_CPP_PATH=$(find ./ -name libeasypbr_cpp.so)
# # NR_FINDS=${find ./ -name libeasypbr_cpp.so | grep . -c}
# # if [ $NR_FINDS -eq 1 ]
#     # then
#         # echo "MOVING"  >> $PREFIX/.messages.txt
# mv "$EASYPBR_CPP_PATH" $PREFIX/lib
