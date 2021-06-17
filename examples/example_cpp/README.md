# EasyPBR example C++


This example shows how to use an external C++ application that uses EasyPBR as a library to render a custom mesh.
The architecture works with a callback system, therefore the C++ application defines functions that execute before the rendering happens (things like setting objects in the scene and manipulating them) and what happens after the rendering is finished (like retriving the final render and writing it to disk).
Behind the hood, EasyPBR takes care of uploading data to GPU and rendering an image on each iteration.

### Build and install:
To build and install the example, you must have first installed EasyPBR. Afterwards this example can be build with
```sh
$ cd examples/example_cpp
$ make
```

### Running
After building one can run the executable created in the build folder with
```sh
$ ./build/temp.linux-x86_64-3.6/run_easypbr_wrapper
```
