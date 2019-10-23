#include "easy_pbr/Camera.h"


//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

#include <math.h>

// Implementation
#include <cmath>
#include <iostream>

#include "string_utils.h"

using namespace easy_pbr::utils;


Camera::Camera():
    m_exposure(1.0),
    m_fov(30.0),
    m_near(0.01),
    m_far(5000),
    m_is_mouse_down(false),
    m_prev_mouse_pos_valid(false),
    m_is_initialized(false)
{

    m_model_matrix.setIdentity();
    m_model_matrix.translation() << 0,0,1;
    m_lookat.setZero();
    m_up=Eigen::Vector3f::UnitY();

}


Eigen::Matrix4f Camera::model_matrix(){
    return m_model_matrix.matrix();
}
Eigen::Matrix4f Camera::view_matrix(){
    return model_matrix().inverse().matrix();
}
Eigen::Matrix4f Camera::proj_matrix(const Eigen::Vector2f viewport_size){
    float aspect=viewport_size.x()/viewport_size.y();
    return compute_projection_matrix(m_fov, aspect, m_near, m_far);
}
Eigen::Vector3f Camera::position(){
    return m_model_matrix.translation();
}
Eigen::Vector3f Camera::lookat(){
    return m_lookat;
}
Eigen::Vector3f Camera::direction(){
    // return  -cam_axes().col(2);
    return (lookat()-position()).normalized(); //better to return this than the cam_axes() because the recalculate orientation needs the direction before actually updating the cam_axes
}
Eigen::Vector3f Camera::up(){
    return m_up;
}
float Camera::dist_to_lookat(){
    return (position()-lookat()).norm();
}
Eigen::Matrix3f Camera::cam_axes(){
    Eigen::Matrix3f cam_axes;

    cam_axes=m_model_matrix.linear();

    return cam_axes;
}



//seters
void Camera::set_lookat(const Eigen::Vector3f& lookat){
    m_lookat=lookat;

    //setting a new lookat means rotating the cam in said direction
    recalculate_orientation();
}
void Camera::set_position(const Eigen::Vector3f& pos){
    m_model_matrix.translation()=pos;

    //setting a new position means rotating the cam so that it still looks at the lookat point
    recalculate_orientation();
}
void Camera::set_up(const Eigen::Vector3f& up){
    m_up=up;

    recalculate_orientation();
}


//convenicence
void Camera::move_cam_and_lookat(const Eigen::Vector3f& pos){
    Eigen::Vector3f displacement=pos-m_model_matrix.translation();
    m_model_matrix.translation()+=displacement;;
    m_lookat+=displacement;
}
void Camera::dolly(const Eigen::Vector3f & dv){
    m_model_matrix.translation() += dv;
}

void Camera::push_away(const float s){
    float old_at_dist = (lookat() - position()).norm();
    float new_lookat_dist = old_at_dist * s;
    dolly(-direction()*(new_lookat_dist - old_at_dist));
}

void Camera::push_away_by_dist(const float new_dist){
    dolly(-direction()*(new_dist));
}

void Camera::orbit(const Eigen::Quaternionf& q){

    Eigen::Vector3f look = lookat();


    //we apply rotations around the lookat point so we have to substract, apply rotation and then add back the lookat point
    Eigen::Affine3f model_matrix_rotated = Eigen::Translation3f(look) * q * Eigen::Translation3f(-look) * Eigen::Affine3f(model_matrix()) ;

    m_model_matrix=model_matrix_rotated;

}





//computations
Eigen::Matrix4f Camera::compute_projection_matrix(const float fov, const float aspect, const float znear, const float zfar){
    //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    Eigen::Matrix4f P;
    P.setConstant(0.0);

    double degree2radians = M_PI / 180.0; //switching to radians because std::tan takes radians
    double f = 1.0 / tan(degree2radians * fov / 2.0);
    double znear_minus_zfar = znear - zfar;

    P(0,0)=f/aspect;
    P(1,1)=f;
    P(2,2)=(zfar+znear)/znear_minus_zfar;
    P(2,3)=(2*zfar*znear)/znear_minus_zfar;
    P(3,2)=-1.0;

    return P;
}

void Camera::recalculate_orientation(){
    Eigen::Matrix3f cam_axes;

    Eigen::Vector3f dir= this->direction();
    Eigen::Vector3f right= (up().cross(-dir)).normalized(); //the order is imporant. We assume a right handed system, up is y axis and (-dir) is the z axis. Dir points towards the negative z
    Eigen::Vector3f up_recalc =  (-dir.cross(right)).normalized(); //recalculate the up vector so that we ensure that it is perpendicular to direction and right

    cam_axes.col(0)=right;
    cam_axes.col(1)=up_recalc;
    cam_axes.col(2)=-dir;
    // m_model_matrix.linear()=Eigen::Quaternionf(cam_axes).toRotationMatrix();
    m_model_matrix.linear()=cam_axes;
}


void Camera::mouse_pressed(const MouseButton mb, const int modifier){

    m_is_mouse_down=true;

    switch (mb){
      case MouseButton::Left:
        mouse_mode = MouseMode::Rotation;
        break;
      case MouseButton::Right:
        mouse_mode = MouseMode::Translation;
        break;
      case MouseButton::Middle:
        break;
    }

}

void Camera::mouse_move(const float x, const float y, const Eigen::Vector2f viewport_size){

    ///attemp2 using https://github.com/libigl/libigl-examples/blob/master/scene-rotation/example.cpp
    m_current_mouse << x,y;
    if(m_is_mouse_down){

        if(mouse_mode==MouseMode::Rotation && m_prev_mouse_pos_valid){

            Eigen::Quaternionf q;
            q=two_axis_rotation(viewport_size, 2.0, m_prev_mouse, m_current_mouse );
            orbit(q);

        }else if(mouse_mode==MouseMode::Translation &&  m_prev_mouse_pos_valid){

            Eigen::Matrix4f view=view_matrix();
            Eigen::Matrix4f proj=proj_matrix(viewport_size);

            Eigen::Vector3f coord = project( Eigen::Vector3f(0,0,0), view, proj, viewport_size);
            float down_mouse_z = coord[2];


            Eigen::Vector3f pos1 = unproject(Eigen::Vector3f(x, viewport_size.y() - y, down_mouse_z), view, proj, viewport_size);
            Eigen::Vector3f pos0 = unproject(Eigen::Vector3f(m_prev_mouse.x(), viewport_size.y() - m_prev_mouse.y(), down_mouse_z), view, proj, viewport_size);


            Eigen::Vector3f diff = pos1 - pos0;
            Eigen::Vector3f new_pos=m_model_matrix.translation() - Eigen::Vector3f(diff[0],diff[1],diff[2]);
            move_cam_and_lookat(new_pos);


        }
        m_prev_mouse=m_current_mouse;
        m_prev_mouse_pos_valid=true;
    }


}


Eigen::Quaternionf Camera::two_axis_rotation(const Eigen::Vector2f viewport_size, const float speed, const Eigen::Vector2f prev_mouse, const Eigen::Vector2f current_mouse){
  
    Eigen::Quaternionf rot_output;

    // rotate around Y axis of the world (the vector 0,1,0)
    Eigen::Vector3f axis_y;
    axis_y << 0,1,0; 
    Eigen::Quaternionf q_y = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.x()-current_mouse.x())/viewport_size.x()*speed,  axis_y.normalized() ) );
    q_y.normalize(); 

    // //rotate around x axis of the camera coordinate
    Eigen::Vector3f axis_x;
    axis_x = cam_axes().col(0);
    Eigen::Quaternionf q_x = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.y()-current_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) ;
    q_x.normalize();

    rot_output=q_y*q_x;

    return rot_output;

}

void Camera::mouse_released(const MouseButton mb, const int modifier){
    m_is_mouse_down=false;
    m_prev_mouse_pos_valid=false;
}

void Camera::mouse_scroll(const float x, const float y){
    float val;
    if(y<0){
      val=1.1;
    }else{
      val=0.9;
    }
    push_away(val);
}

Eigen::Vector3f Camera::project(const Eigen::Vector3f point_world, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport){
    Eigen::Vector4f tmp;
    tmp << point_world,1;

    tmp = view * tmp;

    tmp = proj * tmp;

    tmp = tmp.array() / tmp(3);
    tmp = tmp.array() * 0.5f + 0.5f;
    tmp(0) = tmp(0) * viewport(0);
    tmp(1) = tmp(1) * viewport(1);

    return tmp.head(3);
}
Eigen::Vector3f Camera::unproject(const Eigen::Vector3f win, const Eigen::Matrix4f view, const Eigen::Matrix4f proj, const Eigen::Vector2f viewport){
    Eigen::Vector3f scene;


    Eigen::Matrix4f Inverse = 
      (proj * view).inverse();

    Eigen::Vector4f tmp;
    tmp << win, 1;
    tmp(0) = tmp(0)  / viewport(0);
    tmp(1) = tmp(1)  / viewport(1);
    tmp = tmp.array() * 2.0f - 1.0f;

    Eigen::Vector4f obj = Inverse * tmp;
    obj /= obj(3);

    scene = obj.head(3);

    return scene;

}

void Camera::print(){
   VLOG(1) << "Camera m_translation and m_rotation of the model_matrix: " << to_string(); 
}

std::string Camera::to_string(){
    std::stringstream buffer;
    Eigen::Quaternionf q(m_model_matrix.linear());
    buffer << m_model_matrix.translation().transpose() << " " << q.vec().transpose() << " " << q.w() << " " << m_lookat.transpose() << " " << m_fov << " " << m_near << " " << m_far;
    return buffer.str();
}

void Camera::from_string(const std::string pose){
    std::vector<std::string> tokens = split(pose, " "); 

    CHECK(tokens.size()==13) << "We should have 13 components to the pose. The components  are: tx ty tz qx qy qz qw lx ly lz fov znear zfar. We have nr of components: " << tokens.size();

    //tx ty tz
    m_model_matrix.translation().x() = stof(tokens[0]);
    m_model_matrix.translation().y() = stof(tokens[1]);
    m_model_matrix.translation().z() = stof(tokens[2]);
    //qx qy qz qw
    Eigen::Quaternionf q;
    q.vec().x() = stof(tokens[3]);
    q.vec().y() = stof(tokens[4]);
    q.vec().z() = stof(tokens[5]);
    q.w() = stof(tokens[6]);
    m_model_matrix.linear()=q.toRotationMatrix();
    //lookat_dist fov znear zfar
    m_lookat.x()=stof(tokens[7]);
    m_lookat.y()=stof(tokens[8]);
    m_lookat.z()=stof(tokens[9]);
    m_fov=stof(tokens[10]);
    m_near=stof(tokens[11]);
    m_far=stof(tokens[12]);

    m_is_initialized=true;
}



