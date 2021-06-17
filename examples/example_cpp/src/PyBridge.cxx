#include "example_cpp/PyBridge.h"

// #include <torch/extension.h>
// #include "torch/torch.h"
// #include "torch/csrc/utils/pybind.h"

//my stuff
#include "example_cpp/EasyPBRwrapper.h"
#include "easy_pbr/Viewer.h"


// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
// PYBIND11_MAKE_OPAQUE(std::vector<int>); //to be able to pass vectors by reference to functions and have things like push back actually work
// PYBIND11_MAKE_OPAQUE(std::vector<float>, std::allocator<float> >);

namespace py = pybind11;


namespace easy_pbr{

PYBIND11_MODULE(easypbr_wrapper, m) {

    //Viewer
    py::class_<EasyPBRwrapper, std::shared_ptr<EasyPBRwrapper>> (m, "EasyPBRwrapper")
    .def_static("create",  &EasyPBRwrapper::create<const std::string&, const std::shared_ptr<Viewer>& > ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    ;


}

}//namespace easy_pbr
