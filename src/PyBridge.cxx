#include "easy_pbr/PyBridge.h"

#ifdef EASYPBR_WITH_TORCH
    #include <torch/extension.h>
    #include "torch/torch.h"
    #include "torch/csrc/utils/pybind.h"

    #include "UtilsPytorch.h"
#endif

// #include "pybind11_tests.h"
// #include "constructor_stats.h"
#include <pybind11/operators.h>
#include <functional>

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
    
//way to declare multiple templaded class with the same functions that are exposed through pybind11 https://stackoverflow.com/a/47749076
template<typename T>
void eigen_affine_bindings(py::module &m, const std::string typestr) {

    using Class =  Eigen::Transform<T,3,Eigen::Affine>;
    std::string pyclass_name = std::string("Affine3") + typestr;
    py::class_<Class> (m, pyclass_name.c_str())
    .def(py::init<>())
    //operators
    .def(py::self * py::self) //multiply a matrix with another one
    //getters
    .def("matrix", [](const Class &m) {  return m.matrix();  } )
    .def("translation", [](const Class &m) {  return m.translation();  } )
    .def("linear", [](const Class &m) {  return m.linear();  } )
    .def("quat", [](const Class &m) {  Eigen::Quaternion<T> q ( m.linear() );    return q.coeffs();   } )
    .def("inverse", [](const Class &m) {  return m.inverse();   } )
    //TODO euler
    //TODO to_xyz_quat_vec
    //setters
    .def("set_identity", [](Class &m) {  m.setIdentity(); return m; } )
    .def("set_translation", [](Class &m, const Eigen::Matrix<T, 3, 1>& t) {  m.translation()=t; return m; } )
    .def("set_linear", [](Class &m, const Eigen::Matrix<T, 3, 3>& r) {  m.linear()=r; return m; } )
    .def("set_quat", [](Class &m, const Eigen::Matrix<T, 4, 1>& q_vec) { 
        Eigen::Quaternion<T> q;  
        q.coeffs()=q_vec;
        m.linear()=q.toRotationMatrix();
        return m;
    } )
    //convenience functions
    .def("translate", [](Class &m, const Eigen::Matrix<T, 3, 1>& t) {  m.translation()+=t; return m; } )
    .def("flip_x", [](Class &m) {  m.matrix()(0,0)= -m.matrix()(0,0);  return m; } )
    .def("flip_y", [](Class &m) {  m.matrix()(1,1)= -m.matrix()(1,1);  return m; } )
    .def("flip_z", [](Class &m) {  m.matrix()(2,2)= -m.matrix()(2,2);  return m; } )
    .def("rotate_axis_angle", [](Class &m, const Eigen::Matrix<T, 3, 1>& axis, const float angle_degrees) { 
        Eigen::Quaternion<T> q = Eigen::Quaternion<T>( Eigen::AngleAxis<T>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );
        Class tf;
        tf.setIdentity();
        tf.linear()=q.toRotationMatrix();
        //compose the old transform with this new one
        m=tf*m;
        return m;
    } )
    .def("rotate_axis_angle_local", [](Class &m, const Eigen::Matrix<T, 3, 1>& axis, const float angle_degrees) { 
        Eigen::Quaternion<T>  q = Eigen::Quaternion<T>( Eigen::AngleAxis<T>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );
        Class rot;
        rot.setIdentity();
        rot.linear()=q.toRotationMatrix();
        Class tf=Eigen::Translation<T,3>(m.translation()) * rot *  Eigen::Translation<T,3>(-m.translation());
        //compose the old transform with this new one
        m=tf*m;
        return m;
    } )
    .def("orbit_y_around_point", [](Class &m, const Eigen::Matrix<T, 3, 1>& point, const float angle_degrees) { 
        Eigen::Matrix<T, 3, 1> axis_y;
        axis_y << 0,1,0; 
        Eigen::Quaternion<T> q_y = Eigen::Quaternion<T>( Eigen::AngleAxis<T>( angle_degrees*M_PI/180.0 ,  axis_y.normalized() ) );
        //we apply rotations around the lookat point so we have to substract, apply rotation and then add back the lookat point
        Eigen::Transform<T,3,Eigen::Affine> model_matrix_rotated = Eigen::Translation< T, 3 >(point) * q_y * Eigen::Translation< T, 3 >(-point) * m  ;
        m=model_matrix_rotated;
        return model_matrix_rotated; 
    } )
    //cast
    .def("to_float", [](const Class &m) {  return m.template cast<float>();  }  ) //template case es needed because of https://stackoverflow.com/a/48029026
    .def("to_double", [](const Class &m) {  return m.template cast<double>();  }  ) //template case es needed because of https://stackoverflow.com/a/48029026
    .def("clone", [](const Class &m) {  Class m_new=m; return m_new;  }  )
    ;
}




PYBIND11_MODULE(easypbr, m) {

    py::class_<cv::Mat> (m, "Mat")
    .def("flip_y", [](cv::Mat &m) {  cv::Mat flipped; cv::flip(m, flipped, 0); m=flipped; return flipped;  } )
    .def("flip_x", [](cv::Mat &m) {  cv::Mat flipped; cv::flip(m, flipped, 1); m=flipped; return flipped;  } )
    .def_readonly("rows", &cv::Mat::rows )
    .def_readonly("cols", &cv::Mat::cols )
    .def("channels", &cv::Mat::channels )
    .def("empty", &cv::Mat::empty )
    // .def("rows", [](const cv::Mat &m) {  return m.rows;  }  )
    ;
    // py::class_<Eigen::Affine3f> (m, "Eigen::Affine3f")
    // .def(py::init<>())
    // //operators
    // .def(py::self * py::self) //multiply a matrix with another one
    // //getters
    // .def("matrix", [](const Eigen::Affine3f &m) {  return m.matrix();  } )
    // .def("translation", [](const Eigen::Affine3f &m) {  return m.translation();  } )
    // .def("linear", [](const Eigen::Affine3f &m) {  return m.linear();  } )
    // // .def("matrix", &Eigen::Affine3f::matrix, py::return_value_policy::reference_internal )
    // // .def("matrix", &Eigen::Affine3f::matrix )
    // // .def("translation", &Eigen::Affine3f::translation )
    // // .def("linear", &Eigen::Affine3f::linear )
    // //setters

    // //translational

    // //convenience functions for translating and rotating


    // .def("to_double", [](const Eigen::Affine3f &m) {  return m.cast<double>();  }  )
    // ;


    // py::class_<Eigen::Affine3d> (m, "Eigen::Affine3d")
    // .def(py::init<>())
    // //operators
    // .def(py::self * py::self) //multiply a matrix with another one
    // //getters
    // .def("matrix", [](const Eigen::Affine3d &m) {  return m.matrix();  } )
    // .def("translation", [](const Eigen::Affine3d &m) {  return m.translation();  } )
    // .def("linear", [](const Eigen::Affine3d &m) {  return m.linear();  } )
    // .def("quat", [](const Eigen::Affine3d &m) {  Eigen::Quaterniond q ( m.linear() );    return q.coeffs();   } )
    // //TODO euler
    // //TODO to_xyz_quat_vec
    // //setters
    // .def("set_translation", [](Eigen::Affine3d &m, const Eigen::Vector3d& t) {  m.translation()=t;  } )
    // .def("set_linear", [](Eigen::Affine3d &m, const Eigen::Matrix3d& t) {  m.linear()=t;  } )
    // .def("set_quat", [](Eigen::Affine3d &m, const Eigen::Vector4d& q_vec) { 
    //     Eigen::Quaterniond q;  
    //     q.coeffs()=q_vec;
    //     m.linear()=q.toRotationMatrix();
    // } )
    // //convenience functions
    // .def("rotate_axis_angle", [](Eigen::Affine3d &m, const Eigen::Vector3d& axis, const float angle_degrees) { 
    //     Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );
    //     Eigen::Affine3d tf;
    //     tf.setIdentity();
    //     tf.linear()=q.toRotationMatrix();
    //     //compose the old transform with this new one
    //     m=tf*m;
    // } )
    // .def("rotate_axis_angle_local", [](Eigen::Affine3d &m, const Eigen::Vector3d& axis, const float angle_degrees) { 
    //     Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );
    //     Eigen::Affine3d rot;
    //     rot.setIdentity();
    //     rot.linear()=q.toRotationMatrix();
    //     Eigen::Affine3d tf=Eigen::Translation3d(m.translation()) * rot *  Eigen::Translation3d(-m.translation());
    //     //compose the old transform with this new one
    //     m=tf*m;
    // } )
    // //cast
    // .def("to_float", [](const Eigen::Affine3d &m) {  return m.cast<float>();  }  )
    // ;



        
    // template<typename T>
    // void declare_array(py::module &m, std::string &typestr) {
    //     // using Class = Array2D<T>;
    //     // std::string pyclass_name = std::string("Array2D") + typestr;
    //     // py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())
    //     // .def(py::init<>())
    //     // .def(py::init<Class::xy_t, Class::xy_t, T>())
    //     // .def("size",      &Class::size)
    //     // .def("width",     &Class::width)
    //     // .def("height",    &Class::height);
    // }
   
    eigen_affine_bindings<float>(m, "f");
    eigen_affine_bindings<double>(m, "d");

    m.def("cv_imread", [](const std::string path) {  cv::Mat mat=cv::imread(path); return mat;  } );

    //Frame
    py::class_<Frame> (m, "Frame")
    .def(py::init<>())
    // .def_readwrite("rgb_32f", &Frame::rgb_32f) //not possible in pybind. You would need to wrap the opencv into a matrix type or soemthing like that
    .def("create_frustum_mesh", &Frame::create_frustum_mesh, py::arg("scale_multiplier") = 1.0, py::arg("show_texture")=true)
    .def("subsample", &Frame::subsample )
    .def("depth2world_xyz_mat", &Frame::depth2world_xyz_mat )
    .def("depth2world_xyz_mesh", &Frame::depth2world_xyz_mesh )
    .def("pixels2dirs_mesh", &Frame::pixels2dirs_mesh )
    .def("pixels2_euler_angles_mesh", &Frame::pixels2_euler_angles_mesh )
    .def("pixels2coords", &Frame::pixels2coords )
    // .def("rotate_y_axis", &Frame::rotate_y_axis )
    // .def("backproject_depth", &Frame::backproject_depth )
    .def("assign_color", &Frame::assign_color )
    // .def("pixel_world_direction", &Frame::pixel_world_direction )
    // .def("pixel_world_direction_euler_angles", &Frame::pixel_world_direction_euler_angles )
    .def("rgb_with_valid_depth", &Frame::rgb_with_valid_depth )
    .def("compute_uv", &Frame::compute_uv )
    .def("pos_in_world", &Frame::pos_in_world )
    .def("look_dir", &Frame::look_dir )
    // .def("load_images", &Frame::load_images )
    .def("load_images", [](Frame &frame) {  frame.load_images(frame); } )
    .def_readwrite("is_shell", &Frame::is_shell )
    .def_readwrite("tf_cam_world", &Frame::tf_cam_world )
    .def_readwrite("K", &Frame::K )
    .def_readwrite("width", &Frame::width )
    .def_readwrite("height", &Frame::height )
    .def_readwrite("gray_8u", &Frame::gray_8u )
    .def_readwrite("rgb_8u", &Frame::rgb_8u )
    .def_readwrite("rgb_32f", &Frame::rgb_32f )
    .def_readwrite("gray_32f", &Frame::gray_32f )
    .def_readwrite("grad_x_32f", &Frame::grad_x_32f )
    .def_readwrite("grad_y_32f", &Frame::grad_y_32f )
    .def_readwrite("depth", &Frame::depth )
    .def_readwrite("normal_32f", &Frame::normal_32f )
    .def_readwrite("mask", &Frame::mask )
    .def_readwrite("frame_idx", &Frame::frame_idx )
    .def_readwrite("cam_id", &Frame::cam_id )
    ;

    //convenience functions to transform from mat or eigen to tensors
    #ifdef EASYPBR_WITH_TORCH
        m.def("mat2tensor", &mat2tensor);
        m.def("tensor2mat", &tensor2mat);
        m.def("eigen2tensor", &eigen2tensor);
        m.def("eigen2mat", &eigen2mat);
        m.def("tensor2eigen", &tensor2eigen);
        m.def("vec2tensor", &vec2tensor);
        m.def("cuda_clear_cache", &cuda_clear_cache);
    #endif
 
    //Viewer
    py::class_<Viewer, std::shared_ptr<Viewer>> (m, "Viewer")
    // .def(py::init<const std::string>())
    .def_static("create",  &Viewer::create<const std::string>, py::arg("config_file") = DEFAULT_CONFIG ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    .def("update", &Viewer::update, py::arg("fbo_id") = 0)
    .def("draw", &Viewer::draw, py::arg("fbo_id") = 0)
    .def("clear_framebuffers", &Viewer::clear_framebuffers)
    .def("load_environment_map", &Viewer::load_environment_map )
    .def("spotlight_with_idx", &Viewer::spotlight_with_idx )
    .def("gbuffer_mat_with_name", &Viewer::gbuffer_mat_with_name )
    .def_readwrite("m_kernel_radius", &Viewer::m_kernel_radius )
    .def_readwrite("m_enable_culling", &Viewer::m_enable_culling )
    .def_readwrite("m_enable_edl_lighting", &Viewer::m_enable_edl_lighting )
    // .def("print_pointers", &Viewer::print_pointers )
    // .def("set_position", &Viewer::set_position )
    // .def("check_position", &Viewer::check_position )
    .def_readwrite("m_swap_buffers", &Viewer::m_swap_buffers )
    .def_readwrite("m_camera", &Viewer::m_camera )
    .def_readwrite("m_default_camera", &Viewer::m_default_camera )
    .def_readwrite("m_recorder", &Viewer::m_recorder )
    .def_readwrite("m_viewport_size", &Viewer::m_viewport_size )
    .def_readwrite("m_nr_drawn_frames", &Viewer::m_nr_drawn_frames )
    ;

    //Gui
    py::class_<Gui, std::shared_ptr<Gui> > (m, "Gui") 
    // .def_static("show_rgb",  []( const Frame& frame, const std::string name ) { Gui::show(frame.rgb_32f, name); }) //making function for eahc one because the frame cannot expose to python the cv mat
    .def_static("show", &Gui::show )
    ;



    //Camera
    py::class_<Camera, std::shared_ptr<Camera>> (m, "Camera")
    .def(py::init<>())
    .def("model_matrix", &Camera::model_matrix )
    .def("model_matrix_affine", &Camera::model_matrix_affine )
    .def("view_matrix", &Camera::view_matrix )
    .def("view_matrix_affine", &Camera::view_matrix_affine )
    .def("intrinsics", &Camera::intrinsics )
    .def("position", &Camera::position )
    .def("lookat", &Camera::lookat )
    .def("direction", &Camera::direction )
    .def("up", &Camera::up )
    .def("cam_axes", &Camera::cam_axes )
    .def("dist_to_lookat", &Camera::dist_to_lookat )
    .def("set_model_matrix", &Camera::set_model_matrix )
    .def("set_position", &Camera::set_position )
    .def("set_quat", &Camera::set_quat )
    .def("set_lookat", &Camera::set_lookat )
    .def("set_dist_to_lookat", &Camera::set_dist_to_lookat )
    .def("push_away", &Camera::push_away )
    .def("push_away_by_dist", &Camera::push_away_by_dist )
    .def("orbit_y", &Camera::orbit_y )
    .def("orbit_axis_angle", &Camera::orbit_axis_angle )
    .def("rotate", &Camera::rotate )
    .def("rotate_axis_angle", &Camera::rotate_axis_angle )
    .def("flip_around_x", &Camera::flip_around_x )
    .def("from_frame", &Camera::from_frame )
    .def("from_string", &Camera::from_string )
    .def("create_frustum_mesh", &Camera::create_frustum_mesh)
    .def("transform_model_matrix", &Camera::transform_model_matrix)
    .def("clone", &Camera::clone)
    .def_readwrite("m_exposure", &Camera::m_exposure )
    .def_readwrite("m_near", &Camera::m_near )
    .def_readwrite("m_far", &Camera::m_far )
    .def_readwrite("m_fov", &Camera::m_fov )
    .def_readwrite("m_model_matrix", &Camera::m_model_matrix )
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
    .def_static("hide_all",  &Scene::hide_all )
    .def_static("show_all",  &Scene::show_all )
    .def_static("get_mesh_with_idx",  &Scene::get_mesh_with_idx )
    .def_static("get_mesh_with_name",  &Scene::get_mesh_with_name )
    .def_static("get_scale", &Scene::get_scale, py::arg("use_mutex") = true )
    .def_static("does_mesh_with_name_exist",  &Scene::does_mesh_with_name_exist)
    .def_static("remove_meshes_starting_with_name",  &Scene::remove_meshes_starting_with_name)
    .def_static("add_mesh",  &Scene::add_mesh)
    .def_static("set_floor_visible",  &Scene::set_floor_visible)
    .def_static("nr_meshes",  &Scene::nr_meshes)
    ;

    //LabelMngr
    py::class_<LabelMngr, std::shared_ptr<LabelMngr>> (m, "LabelMngr")
    // .def(py::init<>()) //canot initialize it because it required in the construction a configuru::Config object and that is not exposed in python
    .def(py::init<const std::string, const std::string, const std::string, const int>()) 
    .def(py::init<const int, const int>() ) 
    .def("nr_classes", &LabelMngr::nr_classes )
    .def("get_idx_unlabeled", &LabelMngr::get_idx_unlabeled )
    .def("class_frequencies", &LabelMngr::class_frequencies )
    .def("idx2label", &LabelMngr::idx2label )
    .def("label2idx", &LabelMngr::label2idx )
    .def("compact", &LabelMngr::compact )
    .def("set_color_scheme", &LabelMngr::set_color_scheme )
    .def("set_color_for_label_with_idx", &LabelMngr::set_color_for_label_with_idx )
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
    .def_readwrite("m_overlay_points", &VisOptions::m_overlay_points)
    .def_readwrite("m_overlay_lines", &VisOptions::m_overlay_lines)
    .def_readwrite("m_points_as_circle", &VisOptions::m_points_as_circle)
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
    .def("set_color_height", &VisOptions::set_color_height )
    .def("set_color_intensity", &VisOptions::set_color_intensity )
    .def("set_color_uv", &VisOptions::set_color_uv )
    ;


    //Mesh
    py::class_<Mesh, std::shared_ptr<Mesh>> (m, "Mesh")
    .def(py::init<>())
    .def(py::init<std::string>())
    .def("load_from_file", &Mesh::load_from_file )
    .def("save_to_file", &Mesh::save_to_file )
    .def("sanity_check", &Mesh::sanity_check )
    .def("clone", &Mesh::clone )
    .def("add", &Mesh::add )
    .def("is_empty", &Mesh::is_empty )
    .def("preallocate_V", &Mesh::preallocate_V )
    .def("create_box_ndc", &Mesh::create_box_ndc )
    .def("create_box", &Mesh::create_box )
    .def("create_floor", &Mesh::create_floor )
    .def("create_sphere", &Mesh::create_sphere )
    .def_readwrite("id", &Mesh::id)
    .def_readwrite("name", &Mesh::name)
    .def_readwrite("m_width", &Mesh::m_width)
    .def_readwrite("m_height", &Mesh::m_height)
    .def_readwrite("m_vis", &Mesh::m_vis)
    .def_readwrite("m_force_vis_update", &Mesh::m_force_vis_update)
    .def_readwrite("m_is_dirty", &Mesh::m_is_dirty)
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
    // .def("model_matrix_d", [](const Mesh &m) {
    //         return m.m_model_matrix;
    //     }
    // )
    // .def("model_matrix_f", [](const Mesh &m) {
    //         return m.m_model_matrix.cast<float>();
    //     }
    // )
    // .def_readwrite("m_model_matrix", &Mesh::m_model_matrix)
    // .def("model_matrix", &Mesh::model_matrix )
    // .def("set_model_matrix", &Mesh::set_model_matrix )
    // .def_property("model_matrix", &Mesh::model_matrix, &Mesh::set_model_matrix)
    .def_property("model_matrix", &Mesh::model_matrix_ref, &Mesh::set_model_matrix)
    .def_property("cur_pose", &Mesh::cur_pose_ref, &Mesh::set_cur_pose)
    .def("get_scale", &Mesh::get_scale )
    .def("color_solid2pervert", &Mesh::color_solid2pervert )
    .def("translate_model_matrix", &Mesh::translate_model_matrix )
    .def("transform_vertices_cpu", &Mesh::transform_vertices_cpu )
    // .def("rotate_model_matrix", &Mesh::rotate_model_matrix )
    // .def("rotate_model_matrix_local", py::overload_cast<const Eigen::Vector3d&, const float >  (&Mesh::rotate_model_matrix_local) )
    .def("apply_model_matrix_to_cpu", &Mesh::apply_model_matrix_to_cpu )
    // .def("set_model_matrix", &Mesh::set_model_matrix )
    // .def("model_matrix_as_xyz_and_quaternion", &Mesh::model_matrix_as_xyz_and_quaternion )
    // .def("model_matrix_as_xyz_and_rpy", &Mesh::model_matrix_as_xyz_and_rpy )
    // .def("premultiply_model_matrix", &Mesh::premultiply_model_matrix )
    // .def("postmultiply_model_matrix", &Mesh::postmultiply_model_matrix )
    .def("recalculate_normals", &Mesh::recalculate_normals )
    .def("flip_normals", &Mesh::flip_normals )
    .def("decimate", &Mesh::decimate )
    .def("upsample", &Mesh::upsample )
    .def("remove_vertices_at_zero", &Mesh::remove_vertices_at_zero )
    .def("compute_tangents", &Mesh::compute_tangents, py::arg("tangent_length") = 1.0)
    .def("estimate_normals_from_neighbourhood", &Mesh::estimate_normals_from_neighbourhood )
    .def("compute_distance_to_mesh", &Mesh::compute_distance_to_mesh )

    // .def("compute_tangents", py::overload_cast<const float>(&Mesh::compute_tangents), py::arg("tangent_length") = 1.0)
    .def("create_grid", &Mesh::create_grid )
    // .def("rotate_x_axis", &Mesh::rotate_x_axis )
    // .def("rotate_y_axis", &Mesh::rotate_y_axis )
    .def("random_subsample", &Mesh::random_subsample )
    .def("normalize_size", &Mesh::normalize_size )
    .def("normalize_position", &Mesh::normalize_position )
    // .def("move_in_x", &Mesh::move_in_x )
    // .def("move_in_y", &Mesh::move_in_y )
    // .def("move_in_z", &Mesh::move_in_z )
    .def("add_child", &Mesh::add_child )
    .def("radius_search", &Mesh::radius_search )
    .def("color_from_label_indices", &Mesh::color_from_label_indices )
    .def("set_diffuse_tex",  py::overload_cast<const std::string, const int> (&Mesh::set_diffuse_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )  //https://github.com/pybind/pybind11/issues/876
    .def("set_metalness_tex", py::overload_cast<const std::string, const int > (&Mesh::set_metalness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_roughness_tex", py::overload_cast<const std::string, const int > (&Mesh::set_roughness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_gloss_tex", py::overload_cast<const std::string, const int > (&Mesh::set_gloss_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_normals_tex", py::overload_cast<const std::string, const int > (&Mesh::set_normals_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_diffuse_tex",  py::overload_cast<const cv::Mat&, const int > (&Mesh::set_diffuse_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_metalness_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_metalness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_roughness_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_roughness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_gloss_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_gloss_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_normals_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_normals_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    ;

    //Recorder
    py::class_<Recorder, std::shared_ptr<Recorder>> (m, "Recorder")
    // .def(py::init<>())
    .def("record", py::overload_cast<const std::string, const std::string >(&Recorder::record) )
    .def("snapshot", py::overload_cast<const std::string, const std::string >(&Recorder::snapshot) )
    ;

    //Profiler
    py::class_<radu::utils::Profiler_ns::Profiler> (m, "Profiler") 
    .def_static("is_profiling_gpu", &radu::utils::Profiler_ns::is_profiling_gpu )
    .def_static("set_profile_gpu", &radu::utils::Profiler_ns::set_profile_gpu )
    .def_static("start",  []( std::string name ) { TIME_START(name); })
    // .def_static("start_and_sync_cuda",  []( std::string name ) { TIME_START(name); })
    .def_static("end",  []( std::string name ) { TIME_END(name); })
    // .def_static("end_and_sync_cuda",  []( std::string name ) { Profiler_ns::sync_cuda(name);  TIME_END(name); })
    .def_static("pause",  []( std::string name ) { TIME_PAUSE(name); })
    .def_static("print_all_stats", &radu::utils::Profiler_ns::Profiler::print_all_stats)
    // .def_static("scope",  []( std::string name ) { TIME_SCOPE(name); }) //DOESNT work because scoping in python doesnt work like that. Rather the scope will die as soon as this function is finished
    ;

    //Recorder
    py::class_<radu::utils::ColorMngr, std::shared_ptr<radu::utils::ColorMngr>> (m, "ColorMngr")
    .def(py::init<>())
    .def("magma_color", &radu::utils::ColorMngr::magma_color )
    .def("plasma_color", &radu::utils::ColorMngr::plasma_color )
    .def("viridis_color", &radu::utils::ColorMngr::viridis_color )
    ;


}

} //namespace easy_pbr
