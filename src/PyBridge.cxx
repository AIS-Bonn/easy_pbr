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
#include "easy_gl/UtilsGL.h"
#include "Profiler.h"

#include "opencv_utils.h"
#include "string_utils.h"

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


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
    .def("from_matrix", [](Class &m, const Eigen::Matrix<T, 4, 4>& mat) {  m.matrix()=mat; return m; } )
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
    .def("flip_x", [](Class &m) {  m.matrix().col(0)= -m.matrix().col(0);  return m; } )
    .def("flip_y", [](Class &m) {  m.matrix().col(1)= -m.matrix().col(1);  return m; } )
    .def("flip_z", [](Class &m) {  m.matrix().col(2)= -m.matrix().col(2);  return m; } )
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
    .def(py::init<>())
    .def(py::init([](std::string file_path) {
        // return std::unique_ptr<Example>(new Example(arg));
        cv::Mat m;
        std::string filepath_trim= radu::utils::trim_copy(file_path);
        std::string file_path_abs;
        if (fs::path( filepath_trim ).is_relative()){
            file_path_abs=(fs::path(PROJECT_SOURCE_DIR) / filepath_trim).string();
        }else{
            file_path_abs=filepath_trim;
        }
        m=cv::imread(file_path_abs, cv::IMREAD_UNCHANGED);  
        return m;
    }))
    .def("create", [](cv::Mat &m, int rows, int cols, int channels) {
        if (channels==1) m=cv::Mat(rows,cols,CV_32FC1);
        if (channels==2) m=cv::Mat(rows,cols,CV_32FC2);
        if (channels==3) m=cv::Mat(rows,cols,CV_32FC3);
        if (channels==4) m=cv::Mat(rows,cols,CV_32FC4);
        return m;
      } )
    .def("from_file", [](cv::Mat &m, const std::string file_path) { 
        std::string filepath_trim= radu::utils::trim_copy(file_path);
        std::string file_path_abs;
        if (fs::path( filepath_trim ).is_relative()){
            file_path_abs=(fs::path(PROJECT_SOURCE_DIR) / filepath_trim).string();
        }else{
            file_path_abs=filepath_trim;
        }

        m=cv::imread(file_path_abs, cv::IMREAD_UNCHANGED);  
    } )
    .def("to_file", [](cv::Mat &m, const std::string path) {  cv::imwrite(path,m);  } )
    .def("from_numpy", [](cv::Mat &m, py::array &arr) { // from https://stackoverflow.com/a/49693704
        CHECK(arr.ndim()==3) <<"We assume 3 dimensions to the image corresponding to h,w,c. But instead it is "<< arr.ndim();
        // VLOG(1) << "Mat from numpy"; 

        //get nr of channels
        int nr_channels=arr.shape(2);
        CHECK(nr_channels==1 || nr_channels==2 || nr_channels==3 || nr_channels==4) << "Assuming nr of channels is 1,2,3 or 4. But it is " << nr_channels;
        //get h and width
        int h=arr.shape(0);
        int w=arr.shape(1);



        //get type
        int mat_type=-1;
        if(arr.dtype().is(  py::dtype("int32"))){
            switch ( nr_channels ) {
                case 1: mat_type = CV_32SC1; break;
                case 2: mat_type = CV_32SC2; break;
                case 3: mat_type = CV_32SC3; break;
                case 4: mat_type = CV_32SC4; break;
                default:   mat_type=-1; break;
            }
        }else if(arr.dtype().is(  py::dtype("int8"))){
            switch ( nr_channels ) {
                case 1: mat_type = CV_8UC1; break;
                case 2: mat_type = CV_8UC2; break;
                case 3: mat_type = CV_8UC3; break;
                case 4: mat_type = CV_8UC4; break;
                default:   mat_type=-1; break;
            }
        }else if(arr.dtype().is(  py::dtype("float32"))){
            switch ( nr_channels ) {
                case 1: mat_type = CV_32FC1; break;
                case 2: mat_type = CV_32FC2; break;
                case 3: mat_type = CV_32FC3; break;
                case 4: mat_type = CV_32FC4; break;
                default:   mat_type=-1; break;
            }
        }else{
            LOG(FATAL) << "Unknown numpy type";
        }

        //wrap buffer
        py::buffer_info buf = arr.request();
        m=cv::Mat(h,w, mat_type, buf.ptr);


        
        } )
    .def("set_alpha_to_value", [](cv::Mat &m, const int val) {
        CHECK(m.channels()==4) << "Mat needs to have 4 channels and it only has " << m.channels();
        cv::Mat mat;
        cv::Mat channels[4];
        split(m, channels);
        merge(channels,3,mat);
        mat.setTo(cv::Scalar::all(val), (channels[3]<255));
        m=mat;
      } )
    .def("set_value_to_alpha", [](cv::Mat &m, const int val) {
        CHECK(m.channels()==3) << "Mat needs to have 3 channels and it only has " << m.channels();
        // cv::Mat mat;
        // cv::cvtColor(m, mat, CV_8UC4);
        cv::Mat mat=cv::Mat(m.rows,m.cols,CV_8UC4);
        // find all white pixel and set alpha value to zero:
        for (int y = 0; y < mat.rows; ++y){
            for (int x = 0; x < mat.cols; ++x){
                cv::Vec3b & pixel = m.at<cv::Vec3b>(y, x);
                cv::Vec4b & pixel_out = mat.at<cv::Vec4b>(y, x);
                pixel_out[0] = pixel[0];
                pixel_out[1] = pixel[1];
                pixel_out[2] = pixel[2];
                pixel_out[3] = 255;
                // if pixel is the value
                if (pixel[0] == val && pixel[1] == val && pixel[2] == val)
                {
                    // set alpha to zero:
                    pixel_out[3] = 0;
                }
            }
        }
        m=mat;
        return mat;
      } )
    .def("clone", [](cv::Mat &m) {  return m.clone();  } )
    .def("set_val", [](cv::Mat &m, const float val) {  m = cv::Scalar(val); } )
    .def("set_val", [](cv::Mat &m, const float r, const float g) {  m = cv::Scalar(r,g); } )
    .def("set_val", [](cv::Mat &m, const float r, const float g, const float b) {  m = cv::Scalar(b,g,r); } )
    .def("set_val", [](cv::Mat &m, const float r, const float g, const float b, const float alpha) {  m = cv::Scalar(b,g,r, alpha); } )
    .def("hsv2rgb", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_HSV2RGB); return res; } )
    .def("hsv2bgr", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_HSV2BGR); return res; } )
    .def("rgb2bgr", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_RGB2BGR); return res; } )
    .def("bgr2rgb", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_BGR2RGB); return res; } )
    .def("rgba2bgra", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_RGBA2BGRA); return res; } )
    .def("bgra2rgba", [](cv::Mat &m) {  cv::Mat res; cv::cvtColor(m, res, cv::COLOR_BGRA2RGBA); return res; } )
    .def("rgb2rgba", [](cv::Mat &m) {   cv::Mat res; cv::cvtColor(m, res, cv::COLOR_RGB2RGBA); return res; } )
    .def("bgr2bgra", [](cv::Mat &m) {   cv::Mat res; cv::cvtColor(m, res, cv::COLOR_BGR2BGRA); return res; } )
    .def("rgba2rgb", [](cv::Mat &m) {   cv::Mat res; cv::cvtColor(m, res, cv::COLOR_RGBA2RGB); return res; } )
    .def("bgra2bgr", [](cv::Mat &m) {   cv::Mat res; cv::cvtColor(m, res, cv::COLOR_BGRA2BGR); return res; } )
    .def("normalize_range", [](cv::Mat &m) {   cv::Mat res;     cv::normalize(m, res, 0, 1, cv::NORM_MINMAX, CV_32F);     return res; } )
    // .def("rgba2rgbblack", [](cv::Mat &m ) {  m=cv::imread(path, cv::IMREAD_UNCHANGED);  } )
    .def("flip_y", [](cv::Mat &m) {  cv::Mat flipped; cv::flip(m, flipped, 0); m=flipped; return flipped;  } )
    .def("flip_x", [](cv::Mat &m) {  cv::Mat flipped; cv::flip(m, flipped, 1); m=flipped; return flipped;  } )
    .def_readonly("rows", &cv::Mat::rows )
    .def_readonly("cols", &cv::Mat::cols )
    .def("channels", &cv::Mat::channels )
    .def("empty", &cv::Mat::empty )
    .def("to_cv8u", [](cv::Mat &m) {  cv::Mat out; m.convertTo(out, CV_8U, 255, 0);  m=out; return out;  } )
    .def("blend", [](cv::Mat &cur, cv::Mat &src2, float alpha ) { cv::Mat dst; cv::addWeighted( cur, alpha, src2, 1-alpha, 0.0, dst);  return dst;   } )
    .def("hconcat", [](cv::Mat &cur, cv::Mat &src1 ) { cv::Mat dst; cv::hconcat(cur,src1,dst);  return dst;   } )
    .def("vconcat", [](cv::Mat &cur, cv::Mat &src1 ) { cv::Mat dst; cv::vconcat(cur,src1,dst);  return dst;   } )
    .def("type_string", [](cv::Mat &m) {  return radu::utils::type2string(m.type());  } )
    ;


    eigen_affine_bindings<float>(m, "f");
    eigen_affine_bindings<double>(m, "d");

    m.def("cv_imread", [](const std::string path) {  cv::Mat mat=cv::imread(path); return mat;  } );

    //Frame
    py::class_<Frame, std::shared_ptr<Frame> > (m, "Frame")
    .def(py::init<>())
    // .def_readwrite("rgb_32f", &Frame::rgb_32f) //not possible in pybind. You would need to wrap the opencv into a matrix type or soemthing like that
    .def("create_frustum_mesh", &Frame::create_frustum_mesh, py::arg("scale_multiplier") = 1.0, py::arg("show_texture")=true, py::arg("texture_max_size")=256 )
    .def("random_crop", &Frame::random_crop )
    .def("crop", &Frame::crop )
    .def("get_valid_crop", &Frame::get_valid_crop )
    .def("enlarge_crop_to_size", &Frame::enlarge_crop_to_size )
    .def("subsample", &Frame::subsample, py::arg().noconvert(), py::arg("subsample_imgs") = true )
    .def("upsample", &Frame::upsample, py::arg().noconvert(), py::arg("upsample_imgs") = true )
    .def("depth2world_xyz_mat", &Frame::depth2world_xyz_mat )
    .def("depth2world_xyz_mesh", &Frame::depth2world_xyz_mesh )
    .def("pixels2dirs_mesh", &Frame::pixels2dirs_mesh )
    .def("pixels2_euler_angles_mesh", &Frame::pixels2_euler_angles_mesh )
    .def("pixels2coords", &Frame::pixels2coords )
    .def("unproject", &Frame::unproject )
    .def("project", &Frame::project )
    .def("undistort", &Frame::undistort )
    .def("has_right_stereo_pair", &Frame::has_right_stereo_pair )
    .def("right_stereo_pair", &Frame::right_stereo_pair )
    .def("set_right_stereo_pair", &Frame::set_right_stereo_pair )
    .def("rectify_stereo_pair", &Frame::rectify_stereo_pair, py::arg("offset_disparity")=0  )
    .def("draw_projected_line", &Frame::draw_projected_line,  py::arg().noconvert(), py::arg().noconvert(), py::arg("thickness")=1 )
    // .def("rotate_y_axis", &Frame::rotate_y_axis )
    // .def("backproject_depth", &Frame::backproject_depth )
    .def("rotate_clockwise_90", &Frame::rotate_clockwise_90 )
    .def("from_camera", &Frame::from_camera, py::arg().noconvert(), py::arg().noconvert(), py::arg().noconvert(), py::arg("flip_z_axis")=true, py::arg("flip_y_axis")=true )
    .def("assign_color", &Frame::assign_color )
    // .def("pixel_world_direction", &Frame::pixel_world_direction )
    // .def("pixel_world_direction_euler_angles", &Frame::pixel_world_direction_euler_angles )
    .def("rgb_with_valid_depth", &Frame::rgb_with_valid_depth )
    .def("compute_uv", &Frame::compute_uv )
    .def("naive_splat", &Frame::naive_splat )
    .def("pos_in_world", &Frame::pos_in_world )
    .def("look_dir", &Frame::look_dir )
    .def("name", &Frame::name )
    .def("unload_images", &Frame::unload_images )
    .def("load_images", [](Frame &frame) {  frame.load_images(frame); } )
    .def("has_extra_field", &Frame::has_extra_field )
    .def("add_extra_field", &Frame::add_extra_field<int> )
    .def("add_extra_field", &Frame::add_extra_field<float> )
    .def("add_extra_field", &Frame::add_extra_field<double> )
    .def("add_extra_field", &Frame::add_extra_field<std::string> )
    .def("add_extra_field", &Frame::add_extra_field<Eigen::MatrixXi> )
    .def("add_extra_field", &Frame::add_extra_field<Eigen::MatrixXf> )
    .def("add_extra_field", &Frame::add_extra_field<Eigen::MatrixXd> )
    .def("get_extra_field_int", &Frame::get_extra_field<int> )
    .def("get_extra_field_float", &Frame::get_extra_field<float> )
    .def("get_extra_field_double", &Frame::get_extra_field<double> )
    .def("get_extra_field_string", &Frame::get_extra_field<std::string> )
    .def("get_extra_field_mat", &Frame::get_extra_field<cv::Mat> )
    .def("get_extra_field_matrixXi", &Frame::get_extra_field<Eigen::MatrixXi> )
    .def("get_extra_field_matrixXf", &Frame::get_extra_field<Eigen::MatrixXf> )
    .def("get_extra_field_matrixXd", &Frame::get_extra_field<Eigen::MatrixXd> )
    .def("get_extra_field_mesh", &Frame::get_extra_field< std::shared_ptr<Mesh> > )
    #ifdef EASYPBR_WITH_TORCH
        .def("get_extra_field_tensor", &Frame::get_extra_field< torch::Tensor > )
    #endif
    .def_readwrite("m_right_stereo_pair", &Frame::m_right_stereo_pair )
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
    .def_readwrite("depth_along_ray", &Frame::depth_along_ray )
    .def_readwrite("confidence", &Frame::confidence )
    .def_readwrite("normal_32f", &Frame::normal_32f )
    .def_readwrite("mask", &Frame::mask )
    .def_readwrite("frame_idx", &Frame::frame_idx )
    .def_readwrite("cam_id", &Frame::cam_id )
    .def_readwrite("rgb_path", &Frame::rgb_path )
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
    .def("rendered_tex_no_gui", &Viewer::rendered_tex_no_gui, py::return_value_policy::reference )
    .def("rendered_tex_with_gui", &Viewer::rendered_tex_with_gui, py::return_value_policy::reference )
    .def("rendered_mat_no_gui", &Viewer::rendered_mat_no_gui )
    .def("rendered_mat_with_gui", &Viewer::rendered_mat_with_gui )
    .def("upload_single_mesh_to_gpu", &Viewer::upload_single_mesh_to_gpu )
    .def("load_trajectory", &Viewer::load_trajectory )
    .def("play_trajectory", &Viewer::play_trajectory )
    .def_readwrite("m_debug", &Viewer::m_debug )
    .def_readwrite("m_use_offscreen", &Viewer::m_use_offscreen )
    .def_readwrite("m_kernel_radius", &Viewer::m_kernel_radius )
    .def_readwrite("m_enable_culling", &Viewer::m_enable_culling )
    .def_readwrite("m_enable_edl_lighting", &Viewer::m_enable_edl_lighting )
    .def_readwrite("m_enable_ssao", &Viewer::m_enable_ssao )
    // .def("print_pointers", &Viewer::print_pointers )
    // .def("set_position", &Viewer::set_position )
    // .def("check_position", &Viewer::check_position )
    .def_readwrite("m_swap_buffers", &Viewer::m_swap_buffers )
    .def_readwrite("m_camera", &Viewer::m_camera )
    .def_readwrite("m_default_camera", &Viewer::m_default_camera )
    .def_readwrite("m_recorder", &Viewer::m_recorder )
    .def_readwrite("m_viewport_size", &Viewer::m_viewport_size )
    .def_readwrite("m_nr_drawn_frames", &Viewer::m_nr_drawn_frames )
    .def_readwrite("m_background_color", &Viewer::m_background_color )
    .def_readwrite("m_camera_translation_speed_multiplier", &Viewer::m_camera_translation_speed_multiplier )
    .def_readwrite("m_gui", &Viewer::m_gui )
    .def_readwrite("m_recording_path", &Viewer::m_recording_path )
    .def_readwrite("m_record_gui", &Viewer::m_record_gui )
    .def_readwrite("m_record_with_transparency", &Viewer::m_record_with_transparency )
    ;

    //Gui
    py::class_<Gui, std::shared_ptr<Gui> > (m, "Gui")
    // .def_static("show_rgb",  []( const Frame& frame, const std::string name ) { Gui::show(frame.rgb_32f, name); }) //making function for eahc one because the frame cannot expose to python the cv mat
    .def_static("show", &Gui::show,  py::arg().noconvert(), py::arg().noconvert(),
                                     py::arg("cv_mat_1") = cv::Mat(), py::arg("name_1") = "",
                                     py::arg("cv_mat_2") = cv::Mat(), py::arg("name_2") = "")
    .def_static("show_gl_texture", &Gui::show_gl_texture )
    .def("selected_mesh_idx", &Gui::selected_mesh_idx )
    .def_readwrite("m_traj_should_draw", &Gui::m_traj_should_draw )
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
    .def("orbit_x", &Camera::orbit_x )
    .def("orbit_y", &Camera::orbit_y )
    .def("orbit_z", &Camera::orbit_z )
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
    .def_readwrite("m_use_fixed_proj_matrix", &Camera::m_use_fixed_proj_matrix )
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
    .def_readwrite("m_force_cast_shadow", &VisOptions::m_force_cast_shadow)
    .def_readwrite("m_show_points", &VisOptions::m_show_points)
    .def_readwrite("m_show_lines", &VisOptions::m_show_lines)
    .def_readwrite("m_show_normals", &VisOptions::m_show_normals)
    .def_readwrite("m_show_mesh", &VisOptions::m_show_mesh)
    .def_readwrite("m_show_wireframe", &VisOptions::m_show_wireframe)
    .def_readwrite("m_show_surfels", &VisOptions::m_show_surfels)
    .def_readwrite("m_overlay_points", &VisOptions::m_overlay_points)
    .def_readwrite("m_overlay_lines", &VisOptions::m_overlay_lines)
    .def_readwrite("m_points_as_circle", &VisOptions::m_points_as_circle)
    .def_readwrite("m_point_size", &VisOptions::m_point_size)
    .def_readwrite("m_line_width", &VisOptions::m_line_width)
    .def_readwrite("m_is_line_dashed", &VisOptions::m_is_line_dashed)
    .def_readwrite("m_dash_size", &VisOptions::m_dash_size)
    .def_readwrite("m_gap_size", &VisOptions::m_gap_size)
    // .def_readwrite("m_color_type", &VisOptions::m_color_type) // doesnt really work because I would have to also expose a lot of the better enum library, but I'll make some funcs instead
    .def_readwrite("m_point_color", &VisOptions::m_point_color)
    .def_readwrite("m_line_color", &VisOptions::m_line_color)
    .def_readwrite("m_solid_color", &VisOptions::m_solid_color)
    .def_readwrite("m_metalness", &VisOptions::m_metalness)
    .def_readwrite("m_roughness", &VisOptions::m_roughness)
    .def_readwrite("m_opacity", &VisOptions::m_opacity)
    .def_readwrite("m_use_custom_shader", &VisOptions::m_use_custom_shader)
    .def("set_color_solid", &VisOptions::set_color_solid )
    .def("set_color_pervertcolor", &VisOptions::set_color_pervertcolor )
    .def("set_color_texture", &VisOptions::set_color_texture )
    .def("set_color_semanticpred", &VisOptions::set_color_semanticpred )
    .def("set_color_semanticgt", &VisOptions::set_color_semanticgt )
    .def("set_color_normalvector", &VisOptions::set_color_normalvector )
    .def("set_color_height", &VisOptions::set_color_height )
    .def("set_color_intensity", &VisOptions::set_color_intensity )
    .def("set_color_uv", &VisOptions::set_color_uv )
    .def("set_color_normalvector_viewcoords", &VisOptions::set_color_normalvector_viewcoords )
    ;


    //Mesh
    py::class_<Mesh, std::shared_ptr<Mesh>> (m, "Mesh")
    .def(py::init<>())
    .def(py::init<std::string>())
    // .def_static("create",  &Mesh::create<const std::string> ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    // .def_static("create",  &Mesh::create<> ) //for templated methods like this one we need to explicitly instantiate one of the arguments
    .def("load_from_file", &Mesh::load_from_file )
    .def("read_obj", &Mesh::read_obj, py::arg().noconvert(), py::arg("load_vti") = false, py::arg("load_vni") = false   )
    .def("save_to_file", &Mesh::save_to_file )
    .def("sanity_check", &Mesh::sanity_check )
    .def("clone", &Mesh::clone )
    // .def("add", &Mesh::add )
    .def("add", py::overload_cast<Mesh&>(&Mesh::add) )
    .def("add", py::overload_cast<const std::vector<std::shared_ptr<Mesh>>&>(&Mesh::add) )
    .def("is_empty", &Mesh::is_empty )
    .def("preallocate_V", &Mesh::preallocate_V )
    .def("create_quad", &Mesh::create_quad )
    .def("create_box_ndc", &Mesh::create_box_ndc )
    .def("create_box", &Mesh::create_box )
    .def("create_floor", &Mesh::create_floor )
    .def("create_sphere", &Mesh::create_sphere )
    .def("create_cylinder", &Mesh::create_cylinder )
    .def("create_line_strip_from_points", &Mesh::create_line_strip_from_points )
    .def("has_extra_field", &Mesh::has_extra_field )
    .def("add_extra_field", &Mesh::add_extra_field<int> )
    .def("add_extra_field", &Mesh::add_extra_field<float> )
    .def("add_extra_field", &Mesh::add_extra_field<double> )
    .def("add_extra_field", &Mesh::add_extra_field<std::string> )
    .def("add_extra_field", &Mesh::add_extra_field<cv::Mat> )
    .def("add_extra_field", &Mesh::add_extra_field<Eigen::MatrixXi> )
    .def("add_extra_field", &Mesh::add_extra_field<Eigen::MatrixXf> )
    .def("add_extra_field", &Mesh::add_extra_field<Eigen::MatrixXd> )
    .def("get_extra_field_int", &Mesh::get_extra_field<int> )
    .def("get_extra_field_float", &Mesh::get_extra_field<float> )
    .def("get_extra_field_double", &Mesh::get_extra_field<double> )
    .def("get_extra_field_string", &Mesh::get_extra_field<std::string> )
    .def("get_extra_field_mat", &Mesh::get_extra_field<cv::Mat> )
    .def("get_extra_field_matrixXi", &Mesh::get_extra_field<Eigen::MatrixXi> )
    .def("get_extra_field_matrixXf", &Mesh::get_extra_field<Eigen::MatrixXf> )
    .def("get_extra_field_matrixXd", &Mesh::get_extra_field<Eigen::MatrixXd> )
    .def_readwrite("id", &Mesh::id)
    .def_readwrite("name", &Mesh::name)
    .def_readwrite("m_width", &Mesh::m_width)
    .def_readwrite("m_height", &Mesh::m_height)
    .def_readwrite("m_vis", &Mesh::m_vis)
    .def_readwrite("m_force_vis_update", &Mesh::m_force_vis_update)
    .def_readwrite("m_is_dirty", &Mesh::m_is_dirty)
    .def_readwrite("m_is_shadowmap_dirty", &Mesh::m_is_shadowmap_dirty)
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
    .def_readwrite("V_bitangent_v", &Mesh::V_bitangent_v)
    .def_readwrite("L_pred", &Mesh::L_pred)
    .def_readwrite("L_gt", &Mesh::L_gt)
    .def_readwrite("I", &Mesh::I)
    .def_readwrite("VTI", &Mesh::VTI)
    .def_readwrite("VNI", &Mesh::VNI)
    .def_readwrite("m_label_mngr", &Mesh::m_label_mngr )
    .def_readwrite("m_min_max_y_for_plotting", &Mesh::m_min_max_y_for_plotting )
    .def_readwrite("m_disk_path", &Mesh::m_disk_path)
    .def_readwrite("custom_render_func", &Mesh::custom_render_func)
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
    .def("transform_model_matrix", &Mesh::transform_model_matrix) // This transform also the children of this mesh
    .def_property("model_matrix", &Mesh::model_matrix_ref, &Mesh::set_model_matrix)
    .def_property("cur_pose", &Mesh::cur_pose_ref, &Mesh::set_cur_pose)
    .def("set_model_matrix_from_string", &Mesh::set_model_matrix_from_string )
    .def("get_scale", &Mesh::get_scale )
    .def("color_solid2pervert", &Mesh::color_solid2pervert )
    .def("color_from_mat", &Mesh::color_from_mat )
    .def("translate_model_matrix", &Mesh::translate_model_matrix )
    .def("transform_vertices_cpu", &Mesh::transform_vertices_cpu )
    // .def("rotate_model_matrix", &Mesh::rotate_model_matrix )
    // .def("rotate_model_matrix_local", py::overload_cast<const Eigen::Vector3d&, const float >  (&Mesh::rotate_model_matrix_local) )
    .def("apply_model_matrix_to_cpu", &Mesh::apply_model_matrix_to_cpu )
    .def("scale_mesh", &Mesh::scale_mesh )
    .def("centroid", &Mesh::centroid )
    // .def("set_model_matrix", &Mesh::set_model_matrix )
    // .def("model_matrix_as_xyz_and_quaternion", &Mesh::model_matrix_as_xyz_and_quaternion )
    // .def("model_matrix_as_xyz_and_rpy", &Mesh::model_matrix_as_xyz_and_rpy )
    // .def("premultiply_model_matrix", &Mesh::premultiply_model_matrix )
    // .def("postmultiply_model_matrix", &Mesh::postmultiply_model_matrix )
    .def("recalculate_normals", &Mesh::recalculate_normals )
    .def("flip_normals", &Mesh::flip_normals )
    .def("recalculate_min_max_height", &Mesh::recalculate_min_max_height )
    .def("decimate", &Mesh::decimate )
    .def("upsample", &Mesh::upsample )
    .def("remove_marked_vertices", &Mesh::remove_marked_vertices )
    .def("flip_winding", &Mesh::flip_winding )
    .def("to_3D", &Mesh::to_3D )
    .def("to_2D", &Mesh::to_2D )
    .def("remove_vertices_at_zero", &Mesh::remove_vertices_at_zero )
    .def("remove_duplicate_vertices", &Mesh::remove_duplicate_vertices )
    .def("undo_remove_duplicate_vertices", &Mesh::undo_remove_duplicate_vertices )
    .def("compute_tangents", &Mesh::compute_tangents, py::arg("tangent_length") = 1.0)
    #ifdef EASYPBR_WITH_PCL
    .def("estimate_normals_from_neighbourhood", &Mesh::estimate_normals_from_neighbourhood )
    #endif
    .def("compute_distance_to_mesh", &Mesh::compute_distance_to_mesh )
    #ifdef EASYPBR_WITH_EMBREE
        .def("compute_embree_ao", &Mesh::compute_embree_ao )
    #endif
    .def("fix_oversplit_due_to_blender_uv", &Mesh::fix_oversplit_due_to_blender_uv )

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
    .def("set_diffuse_tex",  py::overload_cast<const std::string, const int, const bool> (&Mesh::set_diffuse_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )  //https://github.com/pybind/pybind11/issues/876
    .def("set_metalness_tex", py::overload_cast<const std::string, const int, const bool> (&Mesh::set_metalness_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )
    .def("set_roughness_tex", py::overload_cast<const std::string, const int, const bool> (&Mesh::set_roughness_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )
    .def("set_smoothness_tex", py::overload_cast<const std::string, const int, const bool> (&Mesh::set_smoothness_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )
    .def("set_normals_tex", py::overload_cast<const std::string, const int, const bool> (&Mesh::set_normals_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )
    .def("set_matcap_tex", py::overload_cast<const std::string, const int, const bool> (&Mesh::set_matcap_tex), py::arg().noconvert(),  py::arg("subsample") = 1, py::arg("read_alpha") = false   )
    .def("set_diffuse_tex",  py::overload_cast<const cv::Mat&, const int > (&Mesh::set_diffuse_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_metalness_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_metalness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_roughness_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_roughness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_smoothness_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_smoothness_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_normals_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_normals_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    .def("set_matcap_tex", py::overload_cast<const cv::Mat&, const int > (&Mesh::set_matcap_tex), py::arg().noconvert(),  py::arg("subsample") = 1  )
    ;

    //Recorder
    py::class_<Recorder, std::shared_ptr<Recorder>> (m, "Recorder")
    // .def(py::init<>())
    .def("record", py::overload_cast<const std::string, const std::string >(&Recorder::record) )
    .def("snapshot", py::overload_cast<const std::string, const std::string >(&Recorder::snapshot) )
    .def("write_without_buffering", &Recorder::write_without_buffering )
    .def("is_finished", &Recorder::is_finished )
    .def("nr_images_recorded", &Recorder::nr_images_recorded )
    .def("start_recording", &Recorder::start_recording )
    .def("pause_recording", &Recorder::pause_recording )
    .def("stop_recording", &Recorder::stop_recording )
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
    .def_static("get_elapsed_ms", &radu::utils::Profiler_ns::Profiler::get_elapsed_ms)
    .def_static("get_elapsed_ns", &radu::utils::Profiler_ns::Profiler::get_elapsed_ns)
    .def_static("clear", &radu::utils::Profiler_ns::Profiler::clear)
    ;

    //Recorder
    py::class_<radu::utils::ColorMngr, std::shared_ptr<radu::utils::ColorMngr>> (m, "ColorMngr")
    .def(py::init<>())
    .def("magma_color", &radu::utils::ColorMngr::magma_color )
    .def("plasma_color", &radu::utils::ColorMngr::plasma_color )
    .def("viridis_color", &radu::utils::ColorMngr::viridis_color )
    .def("mat2color", &radu::utils::ColorMngr::mat2color )
    .def("eigen2color", &radu::utils::ColorMngr::eigen2color )
    ;

    //utilsgl EVERYTHING hre is in another submodule
    py::module gl = m.def_submodule("gl", "GL is a submodule of easypbr");

    gl.def("intrinsics_to_opengl_proj", &intrinsics_to_opengl_proj );
    gl.def("opengl_proj_to_intrinsics", &opengl_proj_to_intrinsics );

    //texture2d everything in a new submodule
    py::class_<gl::Texture2D> (gl, "Texture2D")
    .def(py::init<>())
    .def("upload_from_cv_mat", &gl::Texture2D::upload_from_cv_mat, py::arg().noconvert(),  py::arg("flip_red_blue") = true, py::arg("store_as_normalized_vals") = true  )
    .def("download_to_cv_mat", &gl::Texture2D::download_to_cv_mat, py::arg("lvl") = 0, py::arg("denormalize") = false  )
    #ifdef EASYPBR_WITH_TORCH
        .def("enable_cuda_transfer", &gl::Texture2D::enable_cuda_transfer )
        .def("disable_cuda_transfer", &gl::Texture2D::disable_cuda_transfer )
        .def("from_tensor", &gl::Texture2D::from_tensor,  py::arg().noconvert(), py::arg("flip_red_blue") = false, py::arg("store_as_normalized_vals") = true )
        .def("to_tensor", &gl::Texture2D::to_tensor )
    #endif
    ;

    py::class_<gl::Buf> (gl, "Buf")
    .def(py::init<>())
    #ifdef EASYPBR_WITH_TORCH
        .def("set_target_array_buffer", &gl::Buf::set_target_array_buffer )
        .def("set_target_element_array_buffer", &gl::Buf::set_target_element_array_buffer )
        .def("enable_cuda_transfer", &gl::Buf::enable_cuda_transfer )
        .def("disable_cuda_transfer", &gl::Buf::disable_cuda_transfer )
        .def("from_tensor", &gl::Buf::from_tensor )
        // .def("to_tensor", [](gl::Buf &buf, py::object dtype){
        //            torch::ScalarType type = torch::python::detail::py_object_to_dtype(dtype);
        //            return buf.to_tensor(type);
        //      } ) //from https://discuss.pytorch.org/t/how-to-pass-python-device-and-dtype-to-c/69577/6

        .def("to_tensor", [](gl::Buf &buf, py::object dtype){
                   const torch::ScalarType type = torch::python::detail::py_object_to_dtype(dtype);
                   if (type==torch::kFloat32){
                        return buf.to_tensor<float>();
                   }else if (type==torch::kInt32){
                        return buf.to_tensor<int>();
                   }else{
                        LOG(FATAL) << "The type of tensor you are asking for is not implemented in the PyBridge. You need to add another explicit template specialization for it in the pybridge and also at the end of the Buf.cxx file.";
                        return buf.to_tensor<float>(); //this line will never actually be reached but it stops the compiler from complainng for the function not having a return value
                   }
                //    return buf.to_tensor< decltype(c10::impl::ScalarTypeToCPPType<type::t>()) >();
             } ) //from https://discuss.pytorch.org/t/how-to-pass-python-device-and-dtype-to-c/69577/6
    #endif
    ;


}

} //namespace easy_pbr
