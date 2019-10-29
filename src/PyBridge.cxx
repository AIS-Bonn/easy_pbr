#include "easy_pbr/PyBridge.h"

// #include <torch/extension.h>
// #include "torch/torch.h"
// #include "torch/csrc/utils/pybind.h"

//my stuff 
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Mesh.h"
#include "easy_pbr/Scene.h"
#include "easy_pbr/LabelMngr.h"


// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
// PYBIND11_MAKE_OPAQUE(std::vector<int>); //to be able to pass vectors by reference to functions and have things like push back actually work 
// PYBIND11_MAKE_OPAQUE(std::vector<float>, std::allocator<float> >);

namespace py = pybind11;




PYBIND11_MODULE(EasyPBR, m) {
 
    //Viewer
    py::class_<Viewer, std::shared_ptr<Viewer>> (m, "Viewer")
    // .def(py::init<const std::string>())
    .def_static("create",  &Viewer::create<const std::string> ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    .def("update", &Viewer::update, py::arg("fbo_id") = 0)
    ;

    //Scene
    py::class_<Scene> (m, "Scene") 
    .def(py::init<>())
    .def_static("show",  py::overload_cast<const Mesh&, const std::string>(&Scene::show) )
    .def_static("get_mesh_with_name",  &Scene::get_mesh_with_name )
    .def_static("does_mesh_with_name_exist",  &Scene::does_mesh_with_name_exist)
    ;

    //LabelMngr
    py::class_<LabelMngr, std::shared_ptr<LabelMngr>> (m, "LabelMngr")
    // .def(py::init<>()) //canot initialize it because it required in the construction a configuru::Config object and that is not exposed in python
    .def("nr_classes", &LabelMngr::nr_classes )
    .def("get_idx_unlabeled", &LabelMngr::get_idx_unlabeled )
    .def("class_frequencies", &LabelMngr::class_frequencies )
    ;

    //VisOptions
    py::class_<VisOptions> (m, "VisOptions")
    .def(py::init<>())
    .def_readwrite("m_is_visible", &VisOptions::m_is_visible)
    .def_readwrite("m_show_points", &VisOptions::m_show_points)
    .def_readwrite("m_show_lines", &VisOptions::m_show_lines)
    .def_readwrite("m_show_mesh", &VisOptions::m_show_mesh)
    .def_readwrite("m_show_wireframe", &VisOptions::m_show_wireframe)
    .def_readwrite("m_show_surfels", &VisOptions::m_show_surfels)
    .def_readwrite("m_point_size", &VisOptions::m_point_size)
    .def_readwrite("m_line_width", &VisOptions::m_line_width)
    // .def_readwrite("m_color_type", &VisOptions::m_color_type) // doesnt really work because I would have to also expose a lot of the better enum library, but I'll make some funcs instead
    .def_readwrite("m_point_color", &VisOptions::m_point_color)
    .def_readwrite("m_line_color", &VisOptions::m_line_color)
    .def_readwrite("m_solid_color", &VisOptions::m_solid_color)
    .def_readwrite("m_metalness", &VisOptions::m_metalness)
    .def_readwrite("m_roughness", &VisOptions::m_roughness)
    .def("set_color_solid", &VisOptions::set_color_solid )
    .def("set_color_pervertcolor", &VisOptions::set_color_pervertcolor )
    .def("set_color_texture", &VisOptions::set_color_texture )
    .def("set_color_semanticpred", &VisOptions::set_color_semanticpred )
    .def("set_color_semanticgt", &VisOptions::set_color_semanticgt )
    .def("set_color_normalvector", &VisOptions::set_color_normalvector )
    ;


    //Mesh
    py::class_<Mesh, std::shared_ptr<Mesh>> (m, "Mesh")
    .def(py::init<>())
    .def(py::init<std::string>())
    .def("load_from_file", &Mesh::load_from_file )
    .def("save_to_file", &Mesh::save_to_file )
    .def("clone", &Mesh::clone )
    .def("add", &Mesh::add )
    .def("is_empty", &Mesh::is_empty )
    .def("create_box_ndc", &Mesh::create_box_ndc )
    .def("create_floor", &Mesh::create_floor )
    .def_readwrite("id", &Mesh::id)
    .def_readwrite("name", &Mesh::name)
    .def_readwrite("m_width", &Mesh::m_width)
    .def_readwrite("m_height", &Mesh::m_height)
    .def_readwrite("m_vis", &Mesh::m_vis)
    .def_readwrite("m_force_vis_update", &Mesh::m_force_vis_update)
    .def_readwrite("V", &Mesh::V)
    .def_readwrite("F", &Mesh::F)
    .def_readwrite("C", &Mesh::C)
    .def_readwrite("E", &Mesh::E)
    .def_readwrite("D", &Mesh::D)
    .def_readwrite("NF", &Mesh::NF)
    .def_readwrite("NV", &Mesh::NV)
    .def_readwrite("UV", &Mesh::UV)
    .def_readwrite("V_tangent_u", &Mesh::V_tangent_u)
    .def_readwrite("V_lenght_v", &Mesh::V_length_v)
    .def_readwrite("L_pred", &Mesh::L_pred)
    .def_readwrite("L_gt", &Mesh::L_gt)
    .def_readwrite("I", &Mesh::I)
    .def_readwrite("m_label_mngr", &Mesh::m_label_mngr )
    .def("rotate_x_axis", &Mesh::rotate_x_axis )
    .def("rotate_y_axis", &Mesh::rotate_y_axis )
    .def("move_in_x", &Mesh::move_in_x )
    .def("move_in_y", &Mesh::move_in_y )
    .def("move_in_z", &Mesh::move_in_z )
    ;


}