#pragma once 

#include <memory>

#include <Eigen/Geometry>


class Camera : public std::enable_shared_from_this<Camera>
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    // UI Enumerations 
    enum class MouseButton {Left, Middle, Right};
    enum class MouseMode { None, Rotation, Zoom, Pan, Translation} mouse_mode;

    Camera();
    //stores internally something aking to a model_matrix, that puts the camera from the local camera coordinates into the world coordinates. we call this the model matrix and the inverse is the view_matrix

    //getters
    Eigen::Matrix4f model_matrix();
    Eigen::Matrix4f view_matrix();
    Eigen::Matrix4f proj_matrix(const Eigen::Vector2f viewport_size);
    Eigen::Vector3f position();
    Eigen::Vector3f lookat();
    Eigen::Vector3f up();
    Eigen::Vector3f direction();
    float dist_to_lookat();


    //setters
    void set_lookat(const Eigen::Vector3f& lookat);
    void set_position(const Eigen::Vector3f& pos);
    void set_up(const Eigen::Vector3f& up);


    //convenience functions
    void dolly(const Eigen::Vector3f& dv); //moves the camera along a certain displacement vector dv expressed in world coordinates
    void push_away(const float s); //moves the camera closer or further from the lookup point. A 's' values of 1 means no movement s>1 means going further and s<1 means closer
    void push_away_by_dist(const float new_dist); //pueshes the camera backwards or forwards until the distance to lookat point matches the new_dist 
    void orbit(const Eigen::Quaternionf& q_x, const Eigen::Quaternionf& q_y ); //Orbit around the m_lookat so that rotation is now q










    // void set_view_matrix();
    // void set_eye(Eigen::Vector3f);
    // void set_lookat(const Eigen::Vector3f& eye, const Eigen::Vector3f&center, const Eigen::Vector3f& up);
    // Eigen::Matrix4f view_matrix(); //gets the transform from world coordinates to the camera frame, so tf_cam_world which maps points from the world into the frame of the camera
    // Eigen::Affine3f tf_world_cam_affine(); //transform the from the camera frame into the world frame
    // Eigen::Affine3f tf_cam_world_affine(); //transforms the scene from the world coordinates and puts it infront of the camera, this is the view_matrix but expressed as an affine (actually rigid) matrix
    // Eigen::Matrix4f proj_matrix(const Eigen::Vector2f viewport_size);
    // Eigen::Vector3f eye(); //returns position of the eye of the camera in world coordinates
    // Eigen::Vector3f lookat(); //returns the lookat points in world coordinates

    // //convenience movements for camera
    // void dolly(const Eigen::Vector3f& dv); //moves the camera along a certain displacement vector dv expressed in world coordinates
    // void push_away(const float s); //moves the camera closer or further from the lookup point. A 's' values of 1 means no movement s>1 means going further and s<1 means closer
    // void push_away_by_dist(const float new_dist); //pueshes the camera backwards or forwards until the distance to lookat point matches the new_dist 
    // void orbit(const Eigen::Quaternionf& q); //Orbit around the m_lookat so that rotation is now q

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

    float m_fov;
    float m_near;
    float m_far;
    // float m_lookat_dist;
    bool m_is_initialized; //the camera start in a somewhat default position. Initializing it means putting the camera in position in which you see the scene. This can be done with from_string or can be done by the viewer automatically when the first update is done. If you used from_string then the viewer doesnt need to do anything

private:

    // Eigen::Vector3f m_lookat; //position at which the camera is looking at in world coordinates. So to speak, the focus of the camera, and the center around which it will orbit 
    // Eigen::Quaternionf m_rotation_conj;
    // Eigen::Vector3f m_translation;
    // Eigen::Vector3f m_position;
    // Eigen::Quaternionf m_orientation;
    // Eigen::Affine3f m_model_matrix; //transforms from cam to world coordinates. So the same as tf_world_cam
    Eigen::Vector3f m_position;
    // Eigen::Matrix3f m_orientation; //the columns represent the x,y,z axes of the camera frame. Take into account that the z is negated here
    Eigen::Vector3f m_up; //usually just (0,1,0)
    Eigen::Vector3f m_lookat;

    //mouse things
    bool m_is_mouse_down;
    Eigen::Vector2f m_current_mouse; //position of the mouse when it was clicked 
    Eigen::Vector2f m_prev_mouse; //position of the mouse at the previous timestep
    Eigen::Vector3f m_prev_translation; //translation of the camera at the previous timestmap
    Eigen::Quaternionf m_prev_rotation; // rotation fo the camera at the previous timestep
    bool m_prev_mouse_pos_valid;


    Eigen::Matrix4f compute_view_matrix(const Eigen::Vector3f& eye, const Eigen::Vector3f&center, const Eigen::Vector3f& up); //computes a view matrix that places the camera at eye, looking at a the point "center" and orientated with the up vector which is usually (0,1,0)
    Eigen::Matrix4f compute_projection_matrix(const float fov, const float aspect, const float znear, const float zfar);
    Eigen::Vector3f project(const Eigen::Vector3f point_world, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport); 
    Eigen::Vector3f unproject(const Eigen::Vector3f point_screen, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport); 
    void two_axis_rotation(Eigen::Quaternionf& q_x, Eigen::Quaternionf& q_y, const Eigen::Vector2f viewport_size, const float speed, const Eigen::Quaternionf prev_rotation, const Eigen::Vector2f prev_mouse, const Eigen::Vector2f current_mouse);
};


