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
    m_is_mouse_down(false),
    m_prev_mouse_pos_valid(false),
    m_is_initialized(false)
{

    m_model_matrix.setIdentity();
    // m_position<< 0,0,1;
    // m_orientation.setIdentity();
    // m_up=Eigen::Vector3f::UnitY();
    // m_lookat.setZero();

}


Eigen::Matrix4f Camera::model_matrix(){
    // return m_model_matrix.matrix();

    // Eigen::Matrix3f cam_axes=this->cam_axes();

    // Eigen::Vector3f direction= this->direction();
    // Eigen::Vector3f right= (up().cross(direction)).normalized(); //the order is imporant 
    // Eigen::Vector3f up_recalc =  (direction.cross(right)).normalized(); //recalculate the up vector so that we ensure that it is perpendicular to direction and right

    // cam_axes.col(0)=right;
    // cam_axes.col(1)=up_recalc;
    // cam_axes.col(2)=-direction;

    //reorthogonalize  https://stackoverflow.com/a/23086492
    // Eigen::AngleAxisf aa(cam_axes);    // RotationMatrix to AxisAngle
    // cam_axes = aa.toRotationMatrix();  // AxisAngle      to RotationMatrix

    // // re-orthogonalize https://stackoverflow.com/a/23082112
    // Eigen::Vector3f x,y,z;
    // x=cam_axes.row(0);
    // y=cam_axes.row(1);
    // z=cam_axes.row(2);
    // float error=x.dot(y);
    // Eigen::Vector3f x_ort,y_ort,z_ort;
    // x_ort=x-(error/2)*y;
    // y_ort=y-(error/2)*x;
    // z_ort=x_ort.cross(y_ort);
    // Eigen::Vector3f x_new,y_new,z_new;
    // x_new = 1.0/2.0*(3-x_ort.dot(x_ort))*x_ort;
    // y_new = 1.0/2.0*(3-y_ort.dot(y_ort))*y_ort;
    // z_new = 1.0/2.0*(3-z_ort.dot(z_ort))*z_ort;
    // cam_axes.row(0)=x_new;
    // cam_axes.row(1)=y_new;
    // cam_axes.row(2)=z_new;


    // VLOG(1) << "direction is " << direction.transpose();
    // VLOG(1) << "cam_axes is \n" << cam_axes;
    // VLOG(1) << "2quaternionAndBack \n" << Eigen::Quaternionf(cam_axes).toRotationMatrix();

    // m_model_matrix.linear()=Eigen::Quaternionf(cam_axes).toRotationMatrix();
    // Eigen::Affine3f model_matrix;
    // model_matrix.linear()=m_orientation.toRotationMatrix();
    // model_matrix.translation()=m_position;

    return m_model_matrix.matrix();
}
Eigen::Matrix4f Camera::view_matrix(){
    return model_matrix().inverse().matrix();
}
Eigen::Vector3f Camera::position(){
    return m_model_matrix.translation();
    // return m_position;
}
Eigen::Vector3f Camera::lookat(){
    return Eigen::Affine3f(model_matrix()) * (Eigen::Vector3f(0,0,-1)*m_lookat_dist);
    // return m_lookat;
}
Eigen::Vector3f Camera::up(){
    // return m_up;
}
Eigen::Vector3f Camera::direction(){
    // Eigen::Vector3f direction= (lookat() - position()).normalized();
    // return direction;
    return - Eigen::Affine3f(model_matrix()).linear().col(2).normalized();
}
float Camera::dist_to_lookat(){
    return (position()-lookat()).norm();
}
// Eigen::Matrix3f Camera::cam_axes(){
//     Eigen::Matrix3f cam_axes;

//     Eigen::Vector3f direction= this->direction();
//     Eigen::Vector3f right= (up().cross(direction)).normalized(); //the order is imporant 
//     Eigen::Vector3f up_recalc =  (direction.cross(right)).normalized(); //recalculate the up vector so that we ensure that it is perpendicular to direction and right

//     cam_axes.col(0)=right;
//     cam_axes.col(1)=up_recalc;
//     cam_axes.col(2)=-direction;

//     return cam_axes;
// }



//seters
void Camera::set_lookat(const Eigen::Vector3f& lookat){
    //setting a new lookat assumes we keep the position and the up vector the same

    // Eigen::Matrix3f cam_axes;

    // Eigen::Vector3f direction= (lookat - position()).normalized();
    // Eigen::Vector3f up=this->up();
    // Eigen::Vector3f right= up.cross(direction); //the order is imporant 
    // up =  (direction.cross(right)).normalized(); //recalculate the up vector so that we ensure that it is perpendicular to direction and right

    // cam_axes.col(0)=right;
    // cam_axes.col(1)=up;
    // cam_axes.col(2)=direction;

    // m_model_matrix.linear()=Eigen::Quaternionf(cam_axes).toRotationMatrix();

    // //update lookat dist 
    // m_lookat_dist= (lookat - position()).norm();

    // m_lookat=lookat;

    m_lookat_dist=(position()-lookat).norm();

}
void Camera::set_position(const Eigen::Vector3f& pos){
    Eigen::Vector3f look=lookat();
    m_model_matrix.translation()=pos;
    set_lookat(look); //updates the distance at which we have the lookat because we now moved the position
    // m_position=pos;
    // m_lookat_dist= (lookat() - pos).norm();
}
// void Camera::set_up(const Eigen::Vector3f& up){
//     // m_up=up;

//     // Eigen::Matrix3f cam_axes;

//     // Eigen::Vector3f direction= this->direction();
//     // Eigen::Vector3f right= up.cross(direction); //the order is imporant 
//     // Eigen::Vector3f up_recalc =  (direction.cross(right)).normalized(); //recalculate the up vector so that we ensure that it is perpendicular to direction and right

//     // cam_axes.col(0)=right;
//     // cam_axes.col(1)=up_recalc;
//     // cam_axes.col(2)=direction;

//     // m_model_matrix.linear()=Eigen::Quaternionf(cam_axes).toRotationMatrix();
// }


//convenicence
void Camera::dolly(const Eigen::Vector3f & dv){
    m_model_matrix.translation() += dv;
}

void Camera::push_away(const float s){
    float old_at_dist = (lookat() - position()).norm();
    float new_lookat_dist = old_at_dist * s;
    dolly(-direction()*(new_lookat_dist - old_at_dist));
}

void Camera::push_away_by_dist(const float new_dist){
    // m_lookat_dist = new_dist;
    dolly(-direction()*(new_dist));
}

void Camera::orbit(const Eigen::Quaternionf& q_x, const Eigen::Quaternionf& q_y){
//   Eigen::Vector3f old_at = lookat();
//   m_rotation_conj = q.conjugate();
//   m_translation = 
//     m_rotation_conj * 
//       (old_at + 
//          m_rotation_conj.conjugate() * Eigen::Vector3f(0,0,1) * m_lookat_dist);

    //model matrix, translate it to the origin, rotate, translate back
    // Eigen::Vector3f t = lookat()-position();
    Eigen::Vector3f look = lookat();
    Eigen::Vector3f pos = position();

    // float dist=dist_to_lookat();
    float dist=m_lookat_dist;
    VLOG(1) << "dist_to_lookat before is " << dist;

    // VLOG(1) << "at origin mat should be " << (Eigen::Translation3f(-t) * Eigen::Affine3f(model_matrix()) ).matrix();

    Eigen::Affine3f original_mat=Eigen::Affine3f(model_matrix()); //
    // original_mat.translation()-=lookat();


    Eigen::Affine3f model_matrix_rotated = Eigen::Translation3f(look) * q_x * Eigen::Translation3f(-look) * original_mat ;
    model_matrix_rotated = Eigen::Translation3f(look) * q_y * Eigen::Translation3f(-look) * model_matrix_rotated ;
    // Eigen::Affine3f model_matrix_rotated =  q_x * Eigen::Translation3f(-look) * original_mat ;

    // m_model_matrix=rotated;dist_to_lookat


    

    // model_matrix_rotated.translation()=-direction()*dist_to_lookat;

    //set our new position 
    // m_position=model_matrix_rotated.translation();
    // m_orientation=model_matrix_rotated.linear();
    m_model_matrix=model_matrix_rotated;

    // //if the up vector is no longer pointing up, change it to a down vector
    // Eigen::Vector3f new_up=(model_matrix_rotated.linear().col(1)).normalized();
    // float pointing_up=new_up.dot(Eigen::Vector3f::UnitY()); //if it's still pointing up then the dot product is positive, if not it is negative
    // if(pointing_up>0.0){
    //     m_up=Eigen::Vector3f::UnitY();
    // }else{
    //     m_up=-Eigen::Vector3f::UnitY();
    // }
    // VLOG(1) << pointing_up;


    //the position may have diverged because of innacuracies so we set the distance to the lookat point to be the same as before
    // m_model_matrix.translation()=lookat()-direction()*dist; //starting from lookat we move in the negative view direction
    m_model_matrix.translation()=-direction()*dist; //starting from lookat we move in the negative view direction


    VLOG(1) << "dist_to_lookat after is " << dist_to_lookat();
}























//----------------------------------------------

// void Camera::set_eye(Eigen::Vector3f eye){
//     m_translation=eye;
// }

// Eigen::Affine3f Camera::tf_world_cam_affine(){
//     Eigen::Affine3f tf = Eigen::Affine3f::Identity();
//     tf.rotate(m_rotation_conj.conjugate());
//     tf.translate(m_translation);
//     return tf;
// }

// Eigen::Affine3f Camera::tf_cam_world_affine(){
//     Eigen::Affine3f tf = Eigen::Affine3f::Identity();
//     tf.translate(-m_translation);
//     tf.rotate(m_rotation_conj);
//     return tf;
// }

// Eigen::Vector3f Camera::eye(){
//     // the eye is at origin in the cam coordinates. Transforimg from cam to world gives us the eye position
//     return tf_world_cam_affine() * Eigen::Vector3f(0,0,0); 
// }

// Eigen::Vector3f Camera::lookat(){
//     return tf_world_cam_affine() * (Eigen::Vector3f(0,0,-1)*m_lookat_dist);
// }

// // https://github.com/OpenGP/htrack/blob/master/util/eigen_opengl_helpers.h
// void Camera::set_lookat(const Eigen::Vector3f& eye, const Eigen::Vector3f&center, const Eigen::Vector3f& up){
//     Eigen::Matrix4f view=compute_view_matrix(eye, center, up);

//     //set the m_lookat_dist, m_rotation_conj and m_translation so that we can get the same view matrix by calling view_matrix()
//     m_translation=eye;


// // Vector3 f = (center - eye).normalized();
// //   Vector3 u = up.normalized();
// //   Vector3 s = f.cross(u).normalized();
// //   u = s.cross(f);
// //   Matrix4 mat = Matrix4::Zero();
// //   mat(0,0) = s.x();
// //   mat(0,1) = s.y();
// //   mat(0,2) = s.z();
// //   mat(0,3) = -s.dot(eye);
// //   mat(1,0) = u.x();
// //   mat(1,1) = u.y();
// //   mat(1,2) = u.z();
// //   mat(1,3) = -u.dot(eye);
// //   mat(2,0) = -f.x();
// //   mat(2,1) = -f.y();
// //   mat(2,2) = -f.z();
// //   mat(2,3) = f.dot(eye);
// //   mat.row(3) << 0,0,0,1; 
// // return mat;
// }

// void Camera::dolly(const Eigen::Vector3f & dv){
//     m_translation += dv;
// }

// void Camera::push_away(const float s){
//     float old_at_dist = m_lookat_dist;
//     m_lookat_dist = old_at_dist * s;
//     dolly(Eigen::Vector3f(0,0,1)*(m_lookat_dist - old_at_dist));
// }

// void Camera::push_away_by_dist(const float new_dist){
//     m_lookat_dist = new_dist;
//     dolly(Eigen::Vector3f(0,0,1)*(new_dist));
// }

// void Camera::orbit(const Eigen::Quaternionf & q){
//   Eigen::Vector3f old_at = lookat();
//   m_rotation_conj = q.conjugate();
//   m_translation = 
//     m_rotation_conj * 
//       (old_at + 
//          m_rotation_conj.conjugate() * Eigen::Vector3f(0,0,1) * m_lookat_dist);
// }

// Eigen::Matrix4f Camera::view_matrix(){
//     return tf_cam_world_affine().matrix().cast<float>();
// }

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

// https://github.com/OpenGP/htrack/blob/master/util/eigen_opengl_helpers.h
Eigen::Matrix4f Camera::compute_view_matrix(const Eigen::Vector3f& eye, const Eigen::Vector3f&center, const Eigen::Vector3f& up){
    Eigen::Vector3f f = (center - eye).normalized();
    Eigen::Vector3f u = up.normalized();
    Eigen::Vector3f s = f.cross(u).normalized();
    u = s.cross(f);
    Eigen::Matrix4f mat = Eigen::Matrix4f::Zero();
    mat(0,0) = s.x();
    mat(0,1) = s.y();
    mat(0,2) = s.z();
    mat(0,3) = -s.dot(eye);
    mat(1,0) = u.x();
    mat(1,1) = u.y();
    mat(1,2) = u.z();
    mat(1,3) = -u.dot(eye);
    mat(2,0) = -f.x();
    mat(2,1) = -f.y();
    mat(2,2) = -f.z();
    mat(2,3) = f.dot(eye);
    mat.row(3) << 0,0,0,1; 
    return mat;
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

            Eigen::Quaternionf q_x, q_y;
            two_axis_rotation(q_x, q_y, viewport_size, 2.0, m_prev_rotation, m_prev_mouse, m_current_mouse );
            orbit(q_x,q_y);

      }else if(mouse_mode==MouseMode::Translation &&  m_prev_mouse_pos_valid){

        //   Eigen::Matrix4f view=view_matrix();
        //   Eigen::Matrix4f proj=proj_matrix(viewport_size);

        //    Eigen::Vector3f coord =
        //   project(
        //     Eigen::Vector3f(0,0,0),
        //     view,
        //     proj,
        //     viewport_size);
        //   float down_mouse_z = coord[2];


        //   Eigen::Vector3f pos1 = unproject(Eigen::Vector3f(x, viewport_size.y() - y, down_mouse_z), view, proj, viewport_size);
        //   Eigen::Vector3f pos0 = unproject(Eigen::Vector3f(m_prev_mouse.x(), viewport_size.y() - m_prev_mouse.y(), down_mouse_z), view, proj, viewport_size);

        //   pos1=m_rotation_conj*pos1;
        //   pos0=m_rotation_conj*pos0;


        //   Eigen::Vector3f diff = pos1 - pos0;
        //   m_translation = m_prev_translation - Eigen::Vector3f(diff[0],diff[1],diff[2]);

     }
      m_prev_mouse=m_current_mouse;
      m_prev_rotation = Eigen::Affine3f(model_matrix()).linear();
    //   m_prev_translation = m_translation;
      m_prev_mouse_pos_valid=true;

    }


}


void Camera::two_axis_rotation(Eigen::Quaternionf& q_x, Eigen::Quaternionf& q_y, const Eigen::Vector2f viewport_size, const float speed, const Eigen::Quaternionf prev_rotation, const Eigen::Vector2f prev_mouse, const Eigen::Vector2f current_mouse){
  
    // Eigen::Quaternionf rot_output;

    // rotate around Y
    Eigen::Vector3f axis_y;
    axis_y << 0,1,0; 
    // axis_y = Eigen::Affine3f(model_matrix()).linear().col(1);
    q_y = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.x()-current_mouse.x())/viewport_size.x()*speed,  axis_y.normalized() ) );
    q_y.normalize(); 

    // //rotate around x
    Eigen::Vector3f axis_x;
    axis_x << 1,0,0; 
    axis_x = Eigen::Affine3f(model_matrix()).linear().col(0);
    // // rot_output = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(current_mouse.y()-prev_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) * rot_output.conjugate();
    // // rot_output = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.y()-current_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) * rot_output.conjugate();
    // rot_output = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.y()-current_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) * rot_output.conjugate();
    // rot_output = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(current_mouse.y()-prev_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) * rot_output;
    q_x = Eigen::Quaternionf( Eigen::AngleAxis<float>( M_PI*(prev_mouse.y()-current_mouse.y())/viewport_size.y()*speed,  axis_x.normalized() ) ) ;
    q_x.normalize();

    // return rot_output;

    // q_x=q_y*q_x.conjugate();

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
//    VLOG(1) << "Camera m_translation and m_rotation_conj: " << to_string(); 
}

std::string Camera::to_string(){
    // std::stringstream buffer;
    // buffer << m_translation.transpose() << " " << m_rotation_conj.vec().transpose() << " " << m_rotation_conj.w() << " " << m_lookat_dist << " " << m_fov << " " << m_near << " " << m_far;
    // return buffer.str();
}

void Camera::from_string(const std::string pose){
    // std::vector<std::string> tokens = split(pose, " "); 

    // CHECK(tokens.size()==11) << "We should have 7 components to the pose. The components  are tx ty tz qx xy qz qw lookat_dist fov znear zfar. We have nr of components: " << tokens.size();

    // //tx ty tz
    // m_translation.x() = stof(tokens[0]);
    // m_translation.y() = stof(tokens[1]);
    // m_translation.z() = stof(tokens[2]);
    // //qx qy qz qw
    // m_rotation_conj.vec().x() = stof(tokens[3]);
    // m_rotation_conj.vec().y() = stof(tokens[4]);
    // m_rotation_conj.vec().z() = stof(tokens[5]);
    // m_rotation_conj.w() = stof(tokens[6]);
    // //lookat_dist fov znear zfar
    // m_lookat_dist=stof(tokens[7]);
    // m_fov=stof(tokens[8]);
    // m_near=stof(tokens[9]);
    // m_far=stof(tokens[10]);

    // m_is_initialized=true;
}



