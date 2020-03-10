# EasyPBR example C++


This example shows how to use an external C++ application that uses EasyPBR as a library to render a custom mesh.
The architecture works with a callback system, therefore the C++ application defines functions that executre before the rendering happens (things like setting objects in the scene and maniuplating them) and what happens after the rendering is finished (like retriving the final render and write it to disk). 
Behind the hood, EasyPBR takes care of uploading data to GPU and rendering an image on each iteration.


