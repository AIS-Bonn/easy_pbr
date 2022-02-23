#pragma once

#include <Eigen/Geometry>
#include <opencv2/highgui/highgui.hpp>

#include "easy_pbr/Mesh.h"

#include <any>
#include <stdexcept>


// DO NOT USE A IFDEF because other C++ libs may include this Frame.h without the compile definitions and therefore the Frame.h that was used to compile easypbr and the one included will be different leading to issues
// #ifdef EASYPBR_WITH_TORCH
    // #include "torch/torch.h"
// #endif

namespace radu { namespace utils {
    class RandGenerator;
    }}

namespace easy_pbr{

    class Camera;

class Frame : public std::enable_shared_from_this<Frame>{ //enable_shared_from is required due to pybind https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
public:
    // EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    Frame();

    int width=-1;
    int height=-1;

    cv::Mat rgb_8u;
    cv::Mat rgb_32f; // becasue some algorithms like the cnns require floating point tensors
    // cv::Mat_<cv::Vec4b> rgba_8u; //Opengl likes 4 channels images
    // cv::Mat_<cv::Vec4f> rgba_32f; //Opengl likes 4 channels images
    cv::Mat gray_8u;
    cv::Mat gray_32f;
    cv::Mat grad_x_32f;
    cv::Mat grad_y_32f;
    cv::Mat gray_with_gradients; //gray image and grad_x and grad_y into one RGB32F image, ready to be uploaded to opengl

    cv::Mat thermal_16u;
    cv::Mat thermal_32f;
    cv::Mat thermal_vis_32f; //for showing only we make the thermal into a 3 channel one so imgui shows it in black and white

    cv::Mat normal_32f;

    cv::Mat img_original_size; //the image in the original size, iwthout any subsampling. Usefult for texturing

    cv::Mat mask;
    cv::Mat depth;
    cv::Mat confidence;
    unsigned long long int timestamp;
    Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
    Eigen::Matrix<float, 5, 1> distort_coeffs = Eigen::Matrix<float, 5, 1>::Zero();
    Eigen::Affine3f tf_cam_world = Eigen::Affine3f::Identity();
    int subsample_factor=1; //the subsample factor (eg, 2,4,8) between the img_original size and the rgb_32f and so on

    std::shared_ptr<Frame> m_right_stereo_pair;

    //paths of the images in case we don't want to load them because it takes too much time. In this case the frame is like a shell of a frame
    bool is_shell; // if the frame is a shell it contains all the parameters of that frame (intrinsics, extrinsics, etc) but NOT the images. The images can be loaded on demand with load_images
    std::string rgb_path;
    std::string thermal_path;
    std::string mask_path;
    std::string depth_path;
    std::string confidence_path;
    std::function<void(Frame& frame)> load_images; //function which when called, will read the paths and fill the corresponding mats

    std::string m_name;
    int cam_id; //id of the camera depending on how many cameras we have (it gos from 0 to 1 in the case of stereo)
    int frame_idx; //frame idx monotonically increasing

    bool is_last=false; //is true when this image is the last in the dataset
    bool is_keyframe=false; //if it is keyframe we would need to create seeds

    std::shared_ptr<radu::utils::RandGenerator> m_rand_gen;

    std::shared_ptr<Mesh> create_frustum_mesh(float scale_multiplier=1.0, bool show_texture=true, const int texture_max_size=256) const;
    void rescale_K(const float scale); //rescaling the image with cv.interarea will shift the pixels so we need to properly move the principal point based on https://dsp.stackexchange.com/questions/6055/how-does-resizing-an-image-affect-the-intrinsic-camera-matrix
    Frame random_crop(const int crop_height, const int crop_width);
    Frame crop(const int start_x, const int start_y, const int crop_width, const int crop_height);
    Frame subsample(const float subsample_factor, bool subsample_imgs=true);
    Frame upsample(const float upsample_factor, bool upsample_imgs=true);
    Frame undistort();
    Frame remap(cv::Mat& rmap1, cv::Mat& rmap2);
    void clone_mats();
    void unload_images(); //releases the memory of all the opencv mats
    std::string name(){ return m_name;};
    bool has_right_stereo_pair(){ return m_right_stereo_pair.get()!=0;};
    void set_right_stereo_pair(Frame& frame);
    std::shared_ptr<Frame> right_stereo_pair();
    std::tuple<std::shared_ptr<easy_pbr::Frame>, std::shared_ptr<easy_pbr::Frame>, float, Eigen::Matrix4d > rectify_stereo_pair(const int offset_disparity=0); //can offset the disparity by some positive value so that the disparity range si smaller

    // void rotate_y_axis(const float rads );
    // Mesh backproject_depth() const;
    // Mesh assign_color(Mesh& cloud);
    // std::shared_ptr<Mesh> pixel_world_direction(); //return a mesh where the V vertices represent directions in world coordiantes in which every pixel of this camera looks through
    // std::shared_ptr<Mesh> pixel_world_direction_euler_angles(); //return a mesh where the V vertices represent the euler angles that each ray through the pixel makes with the negative Z axis of the world
    Frame rotate_clockwise_90();

    void from_camera(const std::shared_ptr<Camera>& cam, const int width, const int height, const bool flip_z_axis=true, const bool flip_y_axis=true);


    //conversions that are useful for treating the data with pytorch for example
    cv::Mat depth2world_xyz_mat() const; //backprojects the depth to the world coodinates and returns a mat of the same size as the depth and with 3 channels cooresponding to the xyz positions in world coords
    std::shared_ptr<Mesh> depth2world_xyz_mesh() const; //backprojects the depth to the world coodinates and point cloud with the XYZ points in world coordinates
    std::shared_ptr<Mesh> pixels2dirs_mesh() const; //return a mesh where the V vertices represent directions in world coordiantes in which every pixel of this camera looks through
    std::shared_ptr<Mesh> pixels2_euler_angles_mesh() const; //return a mesh where the V vertices represent the euler angles that each ray through the pixel makes with the negative Z axis of the world
    std::shared_ptr<Mesh> pixels2coords() const; // return the 2D coords of the pixels in screen coordinates
    std::shared_ptr<Mesh>  assign_color(std::shared_ptr<Mesh>& cloud) const; //grabs a point cloud in world coordinates and assings colors to the points by projecting it into the current color frame
    cv::Mat rgb_with_valid_depth(const Frame& frame_depth) const; //returns a color Mat which the color set to 0 for pixels that have no depth info
    Eigen::MatrixXd  compute_uv(std::shared_ptr<Mesh>& cloud) const; //projects the cloud into the frame and returns the uv coordinates that index into the frame, The uv is in range [0,1] with the zero being top left (so like the opencv and not the opengl)
    cv::Mat naive_splat(std::shared_ptr<Mesh>& cloud, Eigen::MatrixXf values); //project the cloud into the frame and also naively splats the values onto the nearest pixel. Doesn't do any Z buffering or weighting
    Eigen::Vector3d unproject(const float x, const float y, const float depth); //gets a pixel in the 2d plane and unprojects it, putting it somewhere in 3d at a depth of 1
    Eigen::Vector2d project(const Eigen::Vector3d& point_world); //projects from world coordinates to img coordinates 
    cv::Mat draw_projected_line(const Eigen::Vector3d& p0_world, const Eigen::Vector3d p1_world, const int thickness=1); //gets two points describing a line in world coordinates, projects them into the image and then draws a line through them;
    cv::Mat get_rgb_mat(); //returns either the rgb_8u or the rgb_32f, whiever one is not empty, otherwise returns an empty mat


    //getters that are nice to have for python bindings
    Eigen::Vector3f pos_in_world() const;
    Eigen::Vector3f look_dir() const;

    //adding extra field to this can eb done through https://stackoverflow.com/a/50956105
    std::map<std::string, std::any> extra_fields;
    template <typename T>
    void add_extra_field(const std::string name, const T data){
        extra_fields[name] = data;
    }
    bool has_extra_field(const std::string name){
        if ( extra_fields.find(name) == extra_fields.end() ) {
            return false;
        } else {
            return true;
        }
    }
    template <typename T>
    T get_extra_field(const std::string name){
        // CHECK(has_extra_field(name)) << "The field you want to acces with name " << name << " does not exist. Please add it with add_extra_field";
        if (!has_extra_field(name)){
            throw std::runtime_error( "The field you want to acces with name " + name + " does not exist. Please add it with add_extra_field" );
        }
        return std::any_cast<T>(extra_fields[name]);
    }


};

} //namespace easy_pbr
