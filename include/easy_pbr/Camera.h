#pragma once 

#include <memory>

#include <Eigen/Geometry>

//to his the fact that Spotlight derives this and both inherit from enabled_shared_from https://www.codeproject.com/Articles/286304/Solution-for-multiple-enable-shared-from-this-in-i
#include "shared_ptr/EnableSharedFromThis.h"
#include "shared_ptr/SmartPtrBuilder.h"

namespace radu { namespace utils { 
    class RandGenerator; 
    }}

namespace easy_pbr{
class Mesh;
typedef std::shared_ptr<Mesh> MeshSharedPtr;

// class Camera : public std::enable_shared_from_this<Camera>
class Camera : public Generic::EnableSharedFromThis< Camera >
{
public:
    // EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    // UI Enumerations 
    enum class MouseButton {Left, Middle, Right};
    enum class MouseMode { None, Rotation, Zoom, Pan, Translation} mouse_mode;

    struct Trajectory
    {
        bool m_enabled = true;
        float m_transition_duration = 10; // in seconds
    };

    Camera();
    //stores internally something akin to a model_matrix, that puts the camera from the local camera coordinates into the world coordinates. we call this the model matrix and the inverse is the view_matrix

    //getters
    Eigen::Matrix4f model_matrix(); //return the model matrix that places the camera in the world. Equivalent to tf_world_cam (maps from the camera coordinates to the world coordinates)
    Eigen::Matrix4f view_matrix(); //returns the view matrix which moves the world into the camera coordinate system. Equivalent to tf_cam_world
    Eigen::Matrix4f proj_matrix(const Eigen::Vector2f viewport_size);
    Eigen::Matrix4f proj_matrix(const float viewport_width, const float viewport_height); //convenience function that takes the size as two separate arguments
    Eigen::Matrix3f intrinsics(const float viewport_width, const float viewport_height);
    Eigen::Vector3f position(); //position of the center of the camera (the eye)
    Eigen::Vector3f lookat(); //target point around which the camera can rotate
    Eigen::Vector3f direction(); // normalized direction towards which the camera looks
    Eigen::Vector3f up(); //the assumed up vector onto which the camera is aligned. Normally it's (0,1,0)
    Eigen::Matrix3f cam_axes(); //returns in the columns of a 3x3 matrix the 3 axes that form the camera coordinate system (the right, up and backward vectors). We store the backward vector instead of the forward one because we assume a right-handed system so the camera look down the negative z axis
    float dist_to_lookat(); //distance from the camera to the lookat point


    //setters
    void set_lookat(const Eigen::Vector3f& lookat); //updates the orientation according to the up vector so that it points towards lookat
    void set_position(const Eigen::Vector3f& pos); //updates the orientation according to the up vector so that it keeps pointing towards lookat
    void set_up(const Eigen::Vector3f& up);
    void set_dist_to_lookat(const float dist); //sets the lookat at a certain distance along the negative z axis


    //convenience functions
    void transform_model_matrix(const Eigen::Affine3f & delta);
    void move_cam_and_lookat(const Eigen::Vector3f& pos); //moves the camera together with the lookat point
    void dolly(const Eigen::Vector3f& dv); //moves the camera along a certain displacement vector dv expressed in world coordinates
    void push_away(const float s); //moves the camera closer or further from the lookup point. A 's' values of 1 means no movement s>1 means going further and s<1 means closer
    void push_away_by_dist(const float new_dist); //pueshes the camera backwards or forwards until the distance to lookat point matches the new_dist 
    void orbit(const Eigen::Quaternionf& q); //Orbit around the m_lookat by an amount specified by q
    void orbit_y(const float angle_degrees); //orbit around the y axis of the world a certain amount of degrees
    void rotate(const Eigen::Quaternionf& q); //rotates around the central camera position by a quaternion q
    Eigen::Vector3f project(const Eigen::Vector3f point_world, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport); 
    Eigen::Vector3f unproject(const Eigen::Vector3f point_screen, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport); 
    Eigen::Vector3f random_direction_in_frustum(const Eigen::Vector2f viewport_size, const float restrict_x, const float restrict_y); //returns a random direction vector in world coords that is inside the frustum of the camera. Therefore if an object is placed along this direction it will for sure be visible by the camera
    void flip_around_x(); //some cameras assume you look in the positive Z direction so we may need to flip them to look backwards. This rotates 180 degrees around the x axis


    //writing the current camera pose to string so we can use it later
    void print(); //print the current m_transalation and m_rotation_conj, fov znear and zfar in format tx ty tz qx qy qz qw look_at_dist fov znear zfar
    std::string to_string(); ///current m_transalation and m_rotation_conj, fov, znear, zfar in format tx ty tz qx qy qz qw look_at_dist fov znear zfar
    void from_string(const std::string pose); //sets the m_translation and m_rotation_conj in format tx ty tz qx qy qz qw  look_at_dist fov znear zfar

    //callbacks
    void mouse_pressed(const MouseButton mb, const int modifier);
    void mouse_move(const float x, const float y, Eigen::Vector2f viewport_size);
    void mouse_released(const MouseButton mb, const int modifier);
    void mouse_scroll(const float x, const float y);
    void wheel_event();

    float m_exposure;
    float m_fov; //fov in the x direction
    float m_near;
    float m_far;
    bool m_is_initialized; //the camera start in a somewhat default position. Initializing it means putting the camera in position in which you see the scene. This can be done with from_string or can be done by the viewer automatically when the first update is done. If you used from_string then the viewer doesnt need to do anything

    Trajectory m_traj;
    std::shared_ptr<Camera> clone();
    MeshSharedPtr create_frustum_mesh(const float scale_multiplier, const Eigen::Vector2f & viewport_size );

    Eigen::Affine3f m_model_matrix; //transforms from cam to world coordinates. So the same as tf_world_cam
private:

    Eigen::Vector3f m_up; //usually just (0,1,0)
    Eigen::Vector3f m_lookat;

    //mouse things
    bool m_is_mouse_down;
    Eigen::Vector2f m_current_mouse; //position of the mouse when it was clicked 
    Eigen::Vector2f m_prev_mouse; //position of the mouse at the previous timestep
    Eigen::Vector3f m_prev_translation; //translation of the camera at the previous timestmap
    Eigen::Quaternionf m_prev_rotation; // rotation fo the camera at the previous timestep
    bool m_prev_mouse_pos_valid;
    std::shared_ptr<radu::utils::RandGenerator> m_rand_gen;


    void recalculate_orientation();
    Eigen::Matrix4f compute_projection_matrix(const float fov_x, const float aspect, const float znear, const float zfar);
    Eigen::Quaternionf two_axis_rotation(const Eigen::Vector2f viewport_size, const float speed, const Eigen::Vector2f prev_mouse, const Eigen::Vector2f current_mouse);
};


} //namespace easy_pbr
