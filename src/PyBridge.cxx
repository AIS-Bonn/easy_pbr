#include "easy_pbr/PyBridge.h"

#ifdef WITH_TORCH
    #include <torch/extension.h>
    #include "torch/torch.h"
    #include "torch/csrc/utils/pybind.h"
#endif

//my stuff 
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Gui.h"
#include "easy_pbr/Mesh.h"
#include "easy_pbr/Scene.h"
#include "easy_pbr/LabelMngr.h"
#include "easy_pbr/Recorder.h"
#include "easy_pbr/Camera.h"
#include "easy_pbr/SpotLight.h"
#include "easy_pbr/Frame.h"
#include "Profiler.h"


// https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
// PYBIND11_MAKE_OPAQUE(std::vector<int>); //to be able to pass vectors by reference to functions and have things like push back actually work 
// PYBIND11_MAKE_OPAQUE(std::vector<float>, std::allocator<float> >);

namespace py = pybind11;


namespace easy_pbr{

PYBIND11_MODULE(easypbr, m) {

    py::class_<cv::Mat> (m, "Mat")
    ;

    //Frame
    py::class_<Frame> (m, "Frame")
    .def(py::init<>())
    // .def_readwrite("rgb_32f", &Frame::rgb_32f) //not possible in pybind. You would need to wrap the opencv into a matrix type or soemthing like that
    .def("create_frustum_mesh", &Frame::create_frustum_mesh, py::arg("scale_multiplier") = 1.0)
    .def("rotate_y_axis", &Frame::rotate_y_axis )
    .def("backproject_depth", &Frame::backproject_depth )
    .def("assign_color", &Frame::assign_color )
    .def("pixel_world_direction", &Frame::pixel_world_direction )
    .def("pos_in_world", &Frame::pos_in_world )
    .def_readwrite("width", &Frame::width )
    .def_readwrite("height", &Frame::height )
    .def_readwrite("rgb_8u", &Frame::rgb_8u )
    .def_readwrite("rgb_32f", &Frame::rgb_32f )
    .def_readwrite("gray_32f", &Frame::gray_32f )
    .def_readwrite("depth", &Frame::depth )
    #ifdef WITH_TORCH
        .def("rgb2tensor", &Frame::rgb2tensor )
        .def("depth2tensor", &Frame::depth2tensor )
        .def("tensor2rgb", &Frame::tensor2rgb )
        .def("tensor2gray", &Frame::tensor2gray )
        .def("tensor2depth", &Frame::tensor2depth )
    #endif
    ;
 
    //Viewer
    py::class_<Viewer, std::shared_ptr<Viewer>> (m, "Viewer")
    // .def(py::init<const std::string>())
    .def_static("create",  &Viewer::create<const std::string>, py::arg("config_file") = DEFAULT_CONFIG ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    .def("update", &Viewer::update, py::arg("fbo_id") = 0)
    .def("load_environment_map", &Viewer::load_environment_map )
    .def("spotlight_with_idx", &Viewer::spotlight_with_idx )
    // .def("print_pointers", &Viewer::print_pointers )
    // .def("set_position", &Viewer::set_position )
    // .def("check_position", &Viewer::check_position )
    .def_readwrite("m_camera", &Viewer::m_camera )
    .def_readonly("m_viewport_size", &Viewer::m_viewport_size )
    ;

    //Gui
    py::class_<Gui, std::shared_ptr<Gui> > (m, "Gui") 
    // .def_static("show_rgb",  []( const Frame& frame, const std::string name ) { Gui::show(frame.rgb_32f, name); }) //making function for eahc one because the frame cannot expose to python the cv mat
    .def_static("show", &Gui::show )
    ;



    //Camera
    py::class_<Camera, std::shared_ptr<Camera>> (m, "Camera")
    .def(py::init<>())
    .def("position", &Camera::position )
    .def("set_position", &Camera::set_position )
    .def("set_lookat", &Camera::set_lookat )
    .def("push_away", &Camera::push_away )
    .def("push_away_by_dist", &Camera::push_away_by_dist )
    .def("orbit_y", &Camera::orbit_y )
    .def("from_string", &Camera::from_string )
    .def("create_frustum_mesh", &Camera::create_frustum_mesh)
    .def_readwrite("m_near", &SpotLight::m_near )
    .def_readwrite("m_far", &SpotLight::m_far )
    ;

    //Spotlight
    py::class_<SpotLight, Camera, std::shared_ptr<SpotLight>  > (m, "SpotLight")
    // py::class_<SpotLight, Camera, std::shared_ptr<Camera> > (m, "SpotLight")
    // .def(py::init<const std::string>())
    .def_readwrite("m_power", &SpotLight::m_power )
    .def_readwrite("m_color", &SpotLight::m_color )
    ;

    //Scene
    py::class_<Scene> (m, "Scene") 
    .def(py::init<>())
    // .def_static("show",  py::overload_cast<const Mesh&, const std::string>(&Scene::show) )
    .def_static("show",  py::overload_cast<const std::shared_ptr<Mesh>, const std::string>(&Scene::show) )
    .def_static("get_mesh_with_name",  &Scene::get_mesh_with_name )
    .def_static("does_mesh_with_name_exist",  &Scene::does_mesh_with_name_exist)
    .def_static("add_mesh",  &Scene::add_mesh)
    ;

    //LabelMngr
    py::class_<LabelMngr, std::shared_ptr<LabelMngr>> (m, "LabelMngr")
    // .def(py::init<>()) //canot initialize it because it required in the construction a configuru::Config object and that is not exposed in python
    .def(py::init<const std::string, const std::string, const std::string, const int>()) 
    .def("nr_classes", &LabelMngr::nr_classes )
    .def("get_idx_unlabeled", &LabelMngr::get_idx_unlabeled )
    .def("class_frequencies", &LabelMngr::class_frequencies )
    .def("idx2label", &LabelMngr::idx2label )
    .def("label2idx", &LabelMngr::label2idx )
    .def("compact", &LabelMngr::compact )
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
    .def_readwrite("m_min_max_y_for_plotting", &Mesh::m_min_max_y_for_plotting )
    .def_readwrite("m_disk_path", &Mesh::m_disk_path)
    .def("get_scale", &Mesh::get_scale )
    .def("color_solid2pervert", &Mesh::color_solid2pervert )
    .def("translate_model_matrix", &Mesh::translate_model_matrix )
    .def("rotate_model_matrix", &Mesh::rotate_model_matrix )
    .def("rotate_model_matrix_local", py::overload_cast<const Eigen::Vector3d&, const float >  (&Mesh::rotate_model_matrix_local) )
    .def("apply_model_matrix_to_cpu", &Mesh::apply_model_matrix_to_cpu )
    .def("recalculate_normals", &Mesh::recalculate_normals )
    .def("compute_tangents", &Mesh::compute_tangents, py::arg("tangent_length") = 1.0)
    // .def("rotate_x_axis", &Mesh::rotate_x_axis )
    // .def("rotate_y_axis", &Mesh::rotate_y_axis )
    .def("random_subsample", &Mesh::random_subsample )
    // .def("move_in_x", &Mesh::move_in_x )
    // .def("move_in_y", &Mesh::move_in_y )
    // .def("move_in_z", &Mesh::move_in_z )
    .def("add_child", &Mesh::add_child )
    .def("radius_search", &Mesh::radius_search )
    .def("color_from_label_indices", &Mesh::color_from_label_indices )
    .def("set_diffuse_tex",  py::overload_cast<const std::string > (&Mesh::set_diffuse_tex) )
    .def("set_metalness_tex", py::overload_cast<const std::string > (&Mesh::set_metalness_tex) )
    .def("set_roughness_tex", py::overload_cast<const std::string > (&Mesh::set_roughness_tex) )
    // .def("set_normals_tex", &Mesh::set_normals_tex )
    ;

    //Recorder
    py::class_<Recorder, std::shared_ptr<Recorder>> (m, "Recorder")
    // .def(py::init<>())
    .def("record", py::overload_cast<const std::string, const std::string >(&Recorder::record) )
    ;

    //Profiler
    py::class_<radu::utils::Profiler_ns::Profiler> (m, "Profiler") 
    .def_static("is_profiling_gpu", &radu::utils::Profiler_ns::is_profiling_gpu )
    .def_static("start",  []( std::string name ) { TIME_START(name); })
    // .def_static("start_and_sync_cuda",  []( std::string name ) { TIME_START(name); })
    .def_static("end",  []( std::string name ) { TIME_END(name); })
    // .def_static("end_and_sync_cuda",  []( std::string name ) { Profiler_ns::sync_cuda(name);  TIME_END(name); })
    .def_static("pause",  []( std::string name ) { TIME_PAUSE(name); })
    .def_static("print_all_stats", &radu::utils::Profiler_ns::Profiler::print_all_stats)
    // .def_static("scope",  []( std::string name ) { TIME_SCOPE(name); }) //DOESNT work because scoping in python doesnt work like that. Rather the scope will die as soon as this function is finished
    ;


}

} //namespace easy_pbr
