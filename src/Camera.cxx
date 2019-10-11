#include "easy_pbr/Camera.h"


//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

#include <math.h>

// Implementation
#include <cmath>
#include <iostream>

// #include "MiscUtils.h"
#include "string_utils.h"

using namespace easy_pbr::utils;


Camera::Camera():
    m_fov(45.0),
    m_near(0.01),
    m_far(5000),
    m_lookat_dist(1),
    m_rotation_conj(1,0,0,0),
    m_translation(0,0,5),
    m_is_mouse_down(false),
    m_prev_mouse_pos_valid(false),
    m_is_initialized(false)
{

}

void Camera::set_eye(Eigen::Vector3f eye){
    m_translation=eye;
}

Eigen::Affine3f Camera::tf_world_cam_affine(){
    Eigen::Affine3f tf = Eigen::Affine3f::Identity();
    tf.rotate(m_rotation_conj.conjugate());
    tf.translate(m_translation);
    return tf;
}

Eigen::Affine3f Camera::tf_cam_world_affine(){
    Eigen::Affine3f tf = Eigen::Affine3f::Identity();
    tf.translate(-m_translation);
    tf.rotate(m_rotation_conj);
    return tf;
}

Eigen::Vector3f Camera::eye(){
    // the eye is at origin in the cam coordinates. Transforimg from cam to world gives us the eye position
    return tf_world_cam_affine() * Eigen::Vector3f(0,0,0); 
}

Eigen::Vector3f Camera::lookat(){
    return tf_world_cam_affine() * (Eigen::Vector3f(0,0,-1)*m_lookat_dist);
}

void Camera::dolly(const Eigen::Vector3f & dv){
    m_translation += dv;
}

void Camera::push_away(const float s){
    float old_at_dist = m_lookat_dist;
    m_lookat_dist = old_at_dist * s;
    dolly(Eigen::Vector3f(0,0,1)*(m_lookat_dist - old_at_dist));
}

void Camera::push_away_by_dist(const float new_dist){
    m_lookat_dist = new_dist;
    dolly(Eigen::Vector3f(0,0,1)*(new_dist));
}

void Camera::orbit(const Eigen::Quaternionf & q){
  Eigen::Vector3f old_at = lookat();
  m_rotation_conj = q.conjugate();
  m_translation = 
    m_rotation_conj * 
      (old_at + 
         m_rotation_conj.conjugate() * Eigen::Vector3f(0,0,1) * m_lookat_dist);
}

Eigen::Matrix4f Camera::view_matrix(){
    return tf_cam_world_affine().matrix().cast<float>();
}

Eigen::Matrix4f Camera::proj_matrix(const Eigen::Vector2f viewport_size){
    float aspect=viewport_size.x()/viewport_size.y();
    return compute_projection_matrix(m_fov, aspect, m_near, m_far);
}

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
            q=two_axis_rotation( viewport_size, 2.0, m_prev_rotation, m_prev_mouse, m_current_mouse );
            orbit(q.conjugate());

      }else if(mouse_mode==MouseMode::Translation &&  m_prev_mouse_pos_valid){

          Eigen::Matrix4f view=view_matrix();
          Eigen::Matrix4f proj=proj_matrix(viewport_size);

           Eigen::Vector3f coord =
          project(
            Eigen::Vector3f(0,0,0),
            view,
            proj,
            viewport_size);
          float down_mouse_z = coord[2];


          Eigen::Vector3f pos1 = unproject(Eigen::Vector3f(x, viewport_size.y() - y, down_mouse_z), view, proj, viewport_size);
          Eigen::Vector3f pos0 = unproject(Eigen::Vector3f(m_prev_mouse.x(), viewport_size.y() - m_prev_mouse.y(), down_mouse_z), view, proj, viewport_size);

          pos1=m_rotation_conj*pos1;
          pos0=m_rotation_conj*pos0;


          Eigen::Vector3f diff = pos1 - pos0;
          m_translation = m_prev_translation - Eigen::Vector3f(diff[0],diff[1],diff[2]);

     }
      m_prev_mouse=m_current_mouse;
      m_prev_rotation = m_rotation_conj;
      m_prev_translation = m_translation;
      m_prev_mouse_pos_valid=true;

    }


}


Eigen::Quaternionf Camera::two_axis_rotation(const Eigen::Vector2f viewport_size, const float speed, const Eigen::Quaternionf prev_rotation, const Eigen::Vector2f prev_mouse, const Eigen::Vector2f current_mouse){
  
    Eigen::Quaternionf rot_output;

    //rotate around Y
    Eigen::Vector3f axis_y;
    axis_y << 0,1,0; 
    rot_output = prev_rotation * Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(current_mouse.x()-prev_mouse.x())/viewport_size.x()*speed,  axis_y.normalized() ) );
    rot_output.normalize(); 

    //rotate around x
    Eigen::Vector3f axis_x;
    axis_x << 1,0,0; 
    rot_output = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(current_mouse.y()-prev_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) * rot_output;
    rot_output.normalize();

    return rot_output;

}

void Camera::mouse_released(const MouseButton mb, const int modifier){
    m_is_mouse_down=false;
    m_prev_mouse_pos_valid=false;
}

void Camera::mouse_scroll(const float x, const float y){
    float val;
    if(y<0){
      val=0.9;
    }else{
      val=1.1;
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
   VLOG(1) << "Camera m_translation and m_rotation_conj: " << to_string(); 
}

std::string Camera::to_string(){
    std::stringstream buffer;
    buffer << m_translation.transpose() << " " << m_rotation_conj.vec().transpose() << " " << m_rotation_conj.w() << " " << m_lookat_dist << " " << m_fov << " " << m_near << " " << m_far;
    return buffer.str();
}

void Camera::from_string(const std::string pose){
    std::vector<std::string> tokens = split(pose, " "); 

    CHECK(tokens.size()==11) << "We should have 7 components to the pose. The components  are tx ty tz qx xy qz qw lookat_dist fov znear zfar. We have nr of components: " << tokens.size();

    //tx ty tz
    m_translation.x() = stof(tokens[0]);
    m_translation.y() = stof(tokens[1]);
    m_translation.z() = stof(tokens[2]);
    //qx qy qz qw
    m_rotation_conj.vec().x() = stof(tokens[3]);
    m_rotation_conj.vec().y() = stof(tokens[4]);
    m_rotation_conj.vec().z() = stof(tokens[5]);
    m_rotation_conj.w() = stof(tokens[6]);
    //lookat_dist fov znear zfar
    m_lookat_dist=stof(tokens[7]);
    m_fov=stof(tokens[8]);
    m_near=stof(tokens[9]);
    m_far=stof(tokens[10]);

    m_is_initialized=true;
}



