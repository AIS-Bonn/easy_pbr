#include "easy_pbr/PyBridge.h"

// #include <torch/extension.h>
// #include "torch/torch.h"
// #include "torch/csrc/utils/pybind.h"

//my stuff 
// #include "surfel_renderer/data_loader/DataLoaderSemanticKitti.h"
// #include "surfel_renderer/data_loader/DataLoaderShapeNetPartSeg.h"
// #include "surfel_renderer/data_loader/DataLoaderToyExample.h"
// #include "surfel_renderer/data_loader/DataLoaderStanfordIndoor.h"
// #include "surfel_renderer/data_loader/DataLoaderScanNet.h"
// #include "surfel_renderer/data_loader/DataLoaderVolRef.h"
// #include "surfel_renderer/core/MeshCore.h"
#include "easy_pbr/Viewer.h"
// #include "surfel_renderer/viewer/Camera.h"
// #include "surfel_renderer/viewer/Recorder.h"
// #include "surfel_renderer/viewer/Gui.h"
// #include "surfel_renderer/viewer/Scene.h"
// #include "surfel_renderer/lattice/VoxelGrid.h"
// #include "surfel_renderer/data_loader/Frame.h"
// #include "surfel_renderer/data_loader/DataLoaderImgRos.h"
// #include "surfel_renderer/data_loader/DataLoaderCloudRos.h"
// #include "surfel_renderer/data_loader/LabelMngr.h"
// #include "surfel_renderer/data_loader/RosBagPlayer.h"
// #include "surfel_renderer/texturer/Texturer.h"
// #include "surfel_renderer/cloud_segmenter/CloudSegmenter.h"
// #include "surfel_renderer/agregator/Agregator.h"
// #include "surfel_renderer/mesher/Mesher.h"
// #include "surfel_renderer/fire_detector/FireDetector.h"
// #include "surfel_renderer/dynamics_detector/DynamicsDetector.h"
// #include "surfel_renderer/misc_modules/CloudComparer.h"
// #include "surfel_renderer/ball_detector/BallDetector.h"
// #include "surfel_renderer/lattice/LatticeCPU_test.h"
// #include "surfel_renderer/lattice/LatticeGPU_test.h"
// #include "surfel_renderer/lattice/Lattice.cuh"
// #include "surfel_renderer/lattice/HashTable.cuh"
// #include "surfel_renderer/misc_modules/PermutoLatticePlotter.h"
// #include "surfel_renderer/misc_modules/TrainParams.h"
// #include "surfel_renderer/misc_modules/EvalParams.h"
// #include "surfel_renderer/misc_modules/ModelParams.h"
// #include "surfel_renderer/cnn/LNN.h"
// #include "surfel_renderer/utils/MiscUtils.h"
// #include "surfel_renderer/utils/Profiler.h"



// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
// PYBIND11_MAKE_OPAQUE(std::vector<int>); //to be able to pass vectors by reference to functions and have things like push back actually work 
// PYBIND11_MAKE_OPAQUE(std::vector<float>, std::allocator<float> >);

namespace py = pybind11;






PYBIND11_MODULE(EasyPBR, m) {
 
    //Viewer
    py::class_<Viewer> (m, "Viewer") 
    .def(py::init<const std::string>())
    .def("update", &Viewer::update, py::arg("fbo_id") = 0)
    ;


}