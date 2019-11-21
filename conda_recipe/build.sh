mkdir -p build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PREFIX

make -j6
make install