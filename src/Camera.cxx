#include "easy_pbr/Camera.h"
#include "easy_pbr/Mesh.h"


//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

#include <math.h>

// Implementation
#include <cmath>
#include <iostream>

#include "string_utils.h"
#include "numerical_utils.h"
#include "RandGenerator.h"
#include "UtilsGL.h"

using namespace radu::utils;

namespace easy_pbr {

Camera::Camera():
    m_exposure(1.0),
    m_fov(30.0),
    m_near(0.01),
    m_far(5000),
    m_is_mouse_down(false),
    m_prev_mouse_pos_valid(false),
    m_is_initialized(false),
    m_position_initialized(false),
    m_lookat_initialized(false),
    m_use_fixed_proj_matrix(false),
    m_rand_gen(new RandGenerator())
{

    m_model_matrix.setIdentity();
    // m_model_matrix.translation() << 0,0,1;
    m_lookat.setZero();
    m_up=Eigen::Vector3f::UnitY();

}


Eigen::Matrix4f Camera::model_matrix(){
    return m_model_matrix.matrix();
}
Eigen::Affine3f Camera::model_matrix_affine(){
    return m_model_matrix;
}
Eigen::Matrix4f Camera::view_matrix(){
    return model_matrix().inverse().matrix();
}
Eigen::Affine3f Camera::view_matrix_affine(){
    return Eigen::Affine3f(model_matrix().inverse());
}
Eigen::Matrix4f Camera::proj_matrix(const Eigen::Vector2f viewport_size){
    float aspect=viewport_size.x()/viewport_size.y();

    VLOG(1)<<"m_use_fixed_proj_matrix" << m_use_fixed_proj_matrix;
    if (m_use_fixed_proj_matrix){
        return m_fixed_proj_matrix;
    }else{
        return compute_projection_matrix(m_fov, aspect, m_near, m_far);
    }
}
Eigen::Matrix4f Camera::proj_matrix(const float viewport_width, const float viewport_height){
    Eigen::Vector2f viewport_size;
    viewport_size << viewport_width, viewport_height;
    return proj_matrix(viewport_size);
}
Eigen::Matrix3f Camera::intrinsics(const float viewport_width, const float viewport_height){
    Eigen::Matrix4f P = proj_matrix(viewport_width, viewport_height);
    Eigen::Matrix3f K = opengl_proj_to_intrinsics(P, viewport_width, viewport_height);
    return K;
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

// float Camera::fov_y(){
//     // return m_fov;

//     float fov_y=fov_x/aspect;
//     VLOG(1) << "fov_x is " << fov_x << " fov_y is " << fov_y << " aspect is " << aspect;


//     double degree2radians = M_PI / 180.0; //switching to radians because std::tan takes radians
//     double f = 1.0 / tan(degree2radians * fov_y / 2.0);
//     double znear_minus_zfar = znear - zfar;

//     P(0,0)=f/aspect;
//     P(1,1)=f


//     proj_matrix=
//     Eigen::Matrix4f
// }
// float Camera::fov_x(){
//     return m_fov;
// }
// float Camera::aspect_ratio(){

// }




//seters
void Camera::set_model_matrix(const Eigen::Affine3f & model_matrix){
    m_model_matrix.setIdentity();
    transform_model_matrix(model_matrix);
}
void Camera::set_lookat(const Eigen::Vector3f& lookat){
    m_lookat=lookat;

    //setting a new lookat means rotating the cam in said direction
    recalculate_orientation();

    m_lookat_initialized=true;
}
void Camera::set_position(const Eigen::Vector3f& pos){
    m_model_matrix.translation()=pos;

    recalculate_orientation();

    m_position_initialized=true;
}
void Camera::set_quat(const Eigen::Vector4f& quat){
    float dist=dist_to_lookat();

    Eigen::Quaternion<float> q;
    q.coeffs()=quat;
    m_model_matrix.linear()=q.toRotationMatrix();

    m_lookat= Eigen::Affine3f(model_matrix()) * (-Eigen::Vector3f::UnitZ() * dist);

    m_is_initialized=true;
    m_lookat_initialized=true;
    m_position_initialized=true;
}
void Camera::set_up(const Eigen::Vector3f& up){
    m_up=up;

    recalculate_orientation();
}
void Camera::set_dist_to_lookat(const float dist){
    m_lookat = position() + cam_axes().col(2) * dist;
    m_lookat_initialized=true;
}


//convenicence
void Camera::move_cam_and_lookat(const Eigen::Vector3f& pos){
    Eigen::Vector3f displacement=pos-m_model_matrix.translation();
    m_model_matrix.translation()+=displacement;;
    m_lookat+=displacement;

    m_lookat_initialized=true;
    m_position_initialized=true;
}
void Camera::dolly(const Eigen::Vector3f & dv){
    m_model_matrix.translation() += dv;

    m_position_initialized=true;
}

void Camera::push_away(const float s){
    float old_at_dist = (lookat() - position()).norm();
    float new_lookat_dist = old_at_dist * s;
    dolly(-direction()*(new_lookat_dist - old_at_dist));

    m_position_initialized=true;
}

void Camera::push_away_by_dist(const float new_dist){
    dolly(-direction()*(new_dist));

    m_position_initialized=true;
}

void Camera::orbit(const Eigen::Quaternionf& q){

    Eigen::Vector3f look = lookat();


    //we apply rotations around the lookat point so we have to substract, apply rotation and then add back the lookat point
    Eigen::Affine3f model_matrix_rotated = Eigen::Translation3f(look) * q * Eigen::Translation3f(-look) * Eigen::Affine3f(model_matrix()) ;

    m_model_matrix=model_matrix_rotated;

    m_position_initialized=true;
}
void Camera::orbit_y(const float angle_degrees){

    Eigen::Vector3f axis_y;
    axis_y << 0,1,0;
    Eigen::Quaternionf q_y = Eigen::Quaternionf( Eigen::AngleAxis<float>( angle_degrees*M_PI/180.0 ,  axis_y.normalized() ) );

    orbit(q_y);

    m_position_initialized=true;
}
void Camera::orbit_axis_angle(const Eigen::Vector3f& axis, const float angle_degrees){

    Eigen::Quaternionf q = Eigen::Quaternionf( Eigen::AngleAxis<float>( angle_degrees*M_PI/180.0 ,  axis.normalized() ) );

    orbit(q);

    m_position_initialized=true;
}
void Camera::rotate(const Eigen::Quaternionf& q){
    // Eigen::Vector3f lookat_in_cam_coords=view_matrix() * lookat(); //gets the lookat position which is in world and puts it in cam coords
    // lookat_in_cam_coords=q.toRotationMatrix()*lookat_in_cam_coords;
    //get back to world coords
    // m_lookat=model_matrix()*lookat_in_cam_coords;

    //we get here the distance to the lookat point and after rotating we should have a lookat position that is at the same distance
    float dist=dist_to_lookat();

    m_model_matrix.linear()=q.toRotationMatrix()*Eigen::Affine3f(model_matrix()).linear();

    m_lookat= Eigen::Affine3f(model_matrix()) * (-Eigen::Vector3f::UnitZ() * dist); //goes in the negative z direction for an amount equal to the distance to lookat so we get a point in cam coords. Afterwards we multiply with the model matrix to get it in world coords

    CHECK( std::fabs(dist_to_lookat() - dist)<0.0001 ) <<"The distance to lookat point changed to much. Something went wrong. Previous dist was " << dist << " now distance is " << dist_to_lookat();

    m_lookat_initialized=true;
    m_position_initialized=true;
}

void Camera::rotate_axis_angle(const Eigen::Vector3f& axis, const float angle_degrees){
    Eigen::Quaternionf q = Eigen::Quaternionf( Eigen::AngleAxis<float>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );
    rotate(q);
}

void Camera::transform_model_matrix(const Eigen::Affine3f & delta)
{
    //we get here the distance to the lookat point and after rotating we should have a lookat position that is at the same distance
    float dist=dist_to_lookat();

    m_model_matrix = delta * m_model_matrix;
    //update the lookat point
    m_lookat= Eigen::Affine3f(model_matrix()) * (-Eigen::Vector3f::UnitZ() * dist); //goes in the negative z direction for an amount equal to the distance to lookat so we get a point in cam coords. Afterwards we multiply with the model matrix to get it in world coords

    // CHECK( std::fabs(dist_to_lookat() - dist)<0.0001 ) <<"The distance to lookat point changed to much. Something went wrong. Previous dist was " << dist << " now distance is " << dist_to_lookat();

    // m_up=cam_axes().col(1);

    m_is_initialized=true;
    m_lookat_initialized=true;
    m_position_initialized=true;
}

void Camera::flip_around_x(){
    m_model_matrix.linear().col(1)=-m_model_matrix.linear().col(1);
    m_model_matrix.linear().col(2)=-m_model_matrix.linear().col(2);
}

void Camera::from_frame(const Frame& frame, const bool flip_z_axis){

    m_model_matrix=frame.tf_cam_world.inverse();

    if (flip_z_axis){
        Eigen::Matrix3f cam_axes;
        cam_axes=m_model_matrix.linear();
        cam_axes.col(2)=-cam_axes.col(2);
        m_model_matrix.linear()= cam_axes;
    }

    //set fov in x direction THis is shit because it assumes a perfect principal point
    float fx=frame.K(0,0);
    m_fov= radians2degrees(2.0*std::atan( frame.width/(2.0*fx)  )); //https://stackoverflow.com/a/41137160
    VLOG(1) << "proj matrix using only the fov would be \n" <<  proj_matrix(frame.width, frame.height);

    m_fixed_proj_matrix=intrinsics_to_opengl_proj(frame.K, frame.width, frame.height);
    m_fixed_proj_matrix.col(2)=-m_fixed_proj_matrix.col(2);
    m_use_fixed_proj_matrix=true;
    VLOG(1) << "fixed proj matrix would be \n" <<  m_fixed_proj_matrix;

    // m_is_initialized=true; //we don't set the initialized because the m_near and m_far are still not valid and the viewer loop should initialize them to a reasonable value
    // m_lookat_initialized=true; //lookat is also not initialized when we set the camera from a frame
    m_position_initialized=true;
}


//computations
Eigen::Matrix4f Camera::compute_projection_matrix(const float fov_x, const float aspect, const float znear, const float zfar){
    //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    Eigen::Matrix4f P;
    P.setConstant(0.0);

    //glu perspective needs the vertical fov and not the usual horizontal one
    float fov_y=fov_x/aspect;
    // VLOG(1) << "fov_x is " << fov_x << " fov_y is " << fov_y << " aspect is " << aspect;


    double degree2radians = M_PI / 180.0; //switching to radians because std::tan takes radians
    double f = 1.0 / tan(degree2radians * fov_y / 2.0);
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

void Camera::mouse_move(const float x, const float y, const Eigen::Vector2f viewport_size, const float speed_multiplier){

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
            diff.array()*=speed_multiplier;
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

Eigen::Vector3f Camera::random_direction_in_frustum(const Eigen::Vector2f viewport_size, const float restrict_x, const float restrict_y){
    //the viewport grid of pixels from which we can sample from

    // Eigen::Matrix4f P=proj_matrix(viewport_size);
    int rand_x=m_rand_gen->rand_int(restrict_x, viewport_size.x()-restrict_x);
    int rand_y=m_rand_gen->rand_int(restrict_y, viewport_size.y()-restrict_y);
    // VLOG(1) << "viewport size is " << viewport_size << "x and y random is " << rand_x << " " <<rand_y;

    //unproject that pixel
    Eigen::Vector3f pixel;
    pixel << rand_x, rand_y, 1.0;

    Eigen::Vector3f point;
    Eigen::Matrix4f V=view_matrix();
    Eigen::Matrix4f P=proj_matrix(viewport_size);
    point=unproject(pixel, V, P, viewport_size);
    // VLOG(1) << "point_ is " << point;

    //get a normalized direction which is negated because the camera is looking in the negative z
    Eigen::Vector3f dir=point.normalized();

    // VLOG(1) << "dir is " << dir;

    return dir;
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
    m_lookat_initialized=true;
    m_position_initialized=true;
}

MeshSharedPtr Camera::create_frustum_mesh( const float scale_multiplier, const Eigen::Vector2f & viewport_size )
{
    MeshSharedPtr frustum_mesh = Mesh::create();
    // https://gamedev.stackexchange.com/questions/29999/how-do-i-create-a-bounding-frustum-from-a-view-projection-matrix
    Eigen::Matrix4f proj=compute_projection_matrix(m_fov, viewport_size.x()/viewport_size.y(), m_near, scale_multiplier * m_far);
    Eigen::Matrix4f view=view_matrix();
    Eigen::Matrix4f view_projection=proj*view;
    Eigen::Matrix4f view_projection_inv=view_projection.inverse();
    Eigen::MatrixXf frustum_V_in_NDC(8,3); //cube in range [-1,1]
    frustum_V_in_NDC <<
                        // near face
                        1,  1, -1,
                       -1,  1, -1,
                       -1, -1, -1,
                        1, -1, -1,
                        //far face
                        1,  1,  1,
                       -1,  1,  1,
                       -1, -1,  1,
                        1, -1,  1;

    //edges
    Eigen::MatrixXi E(12,2);
    E <<
         //near face
         0,1,
         1,2,
         2,3,
         3,0,
         //far face
         4,5,
         5,6,
         6,7,
         7,4,
         //in between
         0,4,
         5,1,
         6,2,
         7,3;

    // Eigen::MatrixXf frustum_in_world=frustum_V_in_NDC*view_projection_inv;
    Eigen::MatrixXf frustum_in_world=(view_projection_inv*frustum_V_in_NDC.transpose().colwise().homogeneous()).transpose();
    Eigen::MatrixXf frustum_in_world_postw;
    Eigen::MatrixXf frustum_vertex_color;
    // frustrum_in_world_postw=frustum_in_world.leftCols(3).array().rowwise()/frustum_in_world.col(3).transpose().array();
    frustum_in_world_postw.resize(8,3);
    frustum_vertex_color.resize(8,3);
    for (int i = 0; i < frustum_in_world.rows(); ++i) {
        float w=frustum_in_world(i,3);
        for (size_t j = 0; j < 3; j++) {
            frustum_in_world_postw(i,j)=frustum_in_world(i,j)/w;
            frustum_vertex_color(i,j) = 1.f;
        }
    }
    // std::cout << "frustrum_in_world_postw is " << frustrum_in_world_postw.rows() << " " << frustrum_in_world_postw.cols() << '\n';

    frustum_mesh->V=frustum_in_world_postw.cast<double>();
    frustum_mesh->C=frustum_vertex_color.cast<double>();
    frustum_mesh->E=E;
    frustum_mesh->m_vis.m_show_mesh=false;
    frustum_mesh->m_vis.m_show_lines=true;

    //since we want somtimes to see the axes of the camera, we actually make the vertices be at the origin an then use the model matrix to trasnform them while rendering. And then through the imguimzo i can check the axes
    //transform from world to camera
    Eigen::Affine3d tf_cam_world=Eigen::Affine3d(view.cast<double>());
    frustum_mesh->transform_vertices_cpu(tf_cam_world.cast<double>(), true);
    frustum_mesh->set_model_matrix( tf_cam_world.cast<double>().inverse() );

    return frustum_mesh;
}


std::shared_ptr<Camera> Camera::clone()
{
    //std::shared_ptr<Camera> newCam = std::make_shared<Camera>();
    //return newCam;
    return std::make_shared<Camera>(*this);
}

} //namespace easy_pbr
