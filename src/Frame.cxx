#include "easy_pbr/Frame.h"

#include "UtilsGL.h"

//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

namespace easy_pbr {

Frame::Frame()
        {

}


Mesh Frame::create_frustum_mesh(float scale_multiplier){

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";

    // https://gamedev.stackexchange.com/questions/29999/how-do-i-create-a-bounding-frustum-from-a-view-projection-matrix
    Mesh frustum_mesh;

    Eigen::Matrix4f proj=intrinsics_to_opengl_proj(K, width, height, 0.5*scale_multiplier, 2.5*scale_multiplier);
    Eigen::Matrix4f view= tf_cam_world.matrix();
    Eigen::Matrix4f view_projection= proj*view;
    Eigen::Matrix4f view_projection_inv=view_projection.inverse();
    Eigen::MatrixXf frustum_V_in_NDC(8,3); //cube in range [-1,1]
    frustum_V_in_NDC <<
        // near face
        1, 1, -1,
        -1, 1, -1,
        -1, -1, -1,
        1, -1, -1,
        //far face
        1, 1, 1,
        -1, 1, 1,
        -1, -1, 1,
        1, -1, 1;


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
    Eigen::MatrixXf frustrum_in_world_postw;
    // frustrum_in_world_postw=frustum_in_world.leftCols(3).array().rowwise()/frustum_in_world.col(3).transpose().array();
    frustrum_in_world_postw.resize(8,3);
    for (int i = 0; i < frustum_in_world.rows(); i++) {
        float w=frustum_in_world(i,3);
        for (size_t j = 0; j < 3; j++) {
            frustrum_in_world_postw(i,j)=frustum_in_world(i,j)/w;
        }
    }
    // std::cout << "frustrum_in_world_postw is " << frustrum_in_world_postw.rows() << " " << frustrum_in_world_postw.cols() << '\n';

    frustum_mesh.V=frustrum_in_world_postw.cast<double>();
    frustum_mesh.E=E;
    frustum_mesh.m_vis.m_show_mesh=false;
    frustum_mesh.m_vis.m_show_lines=true;

    return frustum_mesh;

}

void Frame::rotate_y_axis(const float rads ){
    Eigen::Affine3f tf;
    tf.setIdentity();
    Eigen::Matrix3f tf_rot;
    tf_rot = Eigen::AngleAxisf(rads*M_PI, Eigen::Vector3f::UnitY());
    tf.matrix().block<3,3>(0,0)=tf_rot;

    //transform 
    Eigen::Affine3f tf_world_cam=tf_cam_world.inverse();
    Eigen::Affine3f tf_world_cam_rotated= tf*tf_world_cam; //cam goes to world, and then cam rotates
    tf_cam_world=tf_world_cam_rotated.inverse();

}

Mesh Frame::backproject_depth() const{

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(depth.data) << "There is no data for the depth image. Are you sure this frame contains depth image?";

    //Make a series of point in sensor frame 
    Eigen::MatrixXd V;
    Eigen::MatrixXd D;
    V.resize(depth.rows*depth.cols,3);
    D.resize(depth.rows*depth.cols,1);
    V.setZero();
    for(int y=0; y<depth.rows; y++){
        for(int x=0; x<depth.cols; x++){
            int idx_insert= y*depth.cols + x;
            V.row(idx_insert) << x, y, 1.0; 
            D.row(idx_insert) << depth.at<float>(y,x);
        }
    }

    //puts it in camera frame 
    // VLOG(1) << " K is \n" << K;
    // VLOG(1) << " K inverse is \n" << K.cast<double>().inverse();
    // V=(K.cast<double>().inverse()*V.transpose()).transpose();
    V=V*K.cast<double>().inverse().transpose(); 
    // V=V.rowwise()*D.array();
    for(int i=0; i<V.rows(); i++){
        V(i,0)=V(i,0)*D(i,0); 
        V(i,1)=V(i,1)*D(i,0); 
        V(i,2)=V(i,2)*D(i,0); 
    }

    //flip y and x
    // V.col(1) = -V.col(1);
    // V.col(0) = -V.col(0);

    // V.row(i)=trans.linear()*V.row(i).transpose() + trans.translation(); 

    Mesh cloud;
    cloud.V=V;
    cloud.D=D;
    cloud.m_width=depth.cols;
    cloud.m_height=depth.rows;
    Eigen::Affine3d tf_world_cam=tf_cam_world.inverse().cast<double>();
    cloud.transform_vertices_cpu(tf_world_cam);
    cloud.m_vis.m_show_points=true;

    return cloud;

}

Mesh Frame::assign_color(Mesh& cloud){
    Eigen::MatrixXd V_transformed;
    V_transformed.resize(cloud.V.rows(), cloud.V.cols());
    cloud.C.resize(cloud.V.rows(), 3);
    cloud.C.setZero();
    cloud.UV.resize(cloud.V.rows(),2);
    cloud.UV.setZero();

    // VLOG(1) << "V is " << cloud.V;

    // transform from world into this frame coords
    // V_transformed=tf_cam_world*cloud.V;
    Eigen::Affine3d trans=tf_cam_world.cast<double>();
    int nr_valid=0;
    for (int i = 0; i < cloud.V.rows(); i++) {
        if(!cloud.V.row(i).isZero()){
            V_transformed.row(i)=trans.linear()*cloud.V.row(i).transpose() + trans.translation();
            nr_valid++;
        }
    }

    // VLOG(1) << "nr valid is " << nr_valid;
    // VLOG(1) << "V_transformed" << V_transformed;
    // VLOG(1) << "V_transformed" << V_transformed;


    //project
    V_transformed=V_transformed*K.cast<double>().transpose(); 
    // V_transformed=K.cast<double>()*V_transformed; 
    // for (int i = 0; i < V_transformed.rows(); i++) {
        // V_transformed.row(i)=trans.linear()*cloud.V.row(i).transpose() + trans.translation();
        // nr_valid++;
    // }
    for (int i = 0; i < cloud.V.rows(); i++) {
        V_transformed(i,0)/=V_transformed(i,2);
        V_transformed(i,1)/=V_transformed(i,2);
        // V_transformed(i,2)=1.0;

        int x=V_transformed(i,0);
        int y=V_transformed(i,1);
        if(y<0 || y>=rgb_32f.rows || x<0 || x>rgb_32f.cols){
            continue;
        }

        // VLOG(1) << "accessing at y,x" << y << " " << x;
        cloud.C(i,0)=rgb_32f.at<cv::Vec3f>(y, x) [ 2 ];
        cloud.C(i,1)=rgb_32f.at<cv::Vec3f>(y, x) [ 1 ];
        cloud.C(i,2)=rgb_32f.at<cv::Vec3f>(y, x) [ 0 ];

        cloud.UV(i,0) = V_transformed(i,0)/rgb_32f.cols;
        cloud.UV(i,1) = V_transformed(i,1)/rgb_32f.rows;
    }

    cloud.m_vis.set_color_pervertcolor();

    return cloud;

}

std::shared_ptr<Mesh> Frame::pixel_world_direction(){
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";

    MeshSharedPtr directions_mesh=Mesh::create();
    directions_mesh->V.resize(width*height, 3);

    int idx_insert=0;
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            Eigen::Vector3f point_screen;
            point_screen << x,y,1.0;

            Eigen::Vector3f point_cam_coords;
            point_cam_coords=K.inverse()*point_screen;

            Eigen::Vector3f point_world_coords;
            point_world_coords=tf_cam_world.inverse()*point_cam_coords;

            Eigen::Vector3f dir=(point_world_coords-tf_cam_world.inverse().translation()).normalized();

            // directions_mesh->V.row(idx_insert)= point_world_coords.cast<double>();
            directions_mesh->V.row(idx_insert)= dir.cast<double>();

            idx_insert++;
        
        }
    }

    directions_mesh->m_vis.m_show_points=true;
    directions_mesh->m_width=width;
    directions_mesh->m_height=height;

    return directions_mesh;

}

cv::Mat Frame::rgb_with_valid_depth(const Frame& frame_depth){
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";
    CHECK(frame_depth.depth.data) << "frame depth does not have depth data assigned";
    CHECK(rgb_32f.data) << "current does not have rgb_32f data assigned";


    Mesh cloud= frame_depth.backproject_depth();
    cloud=assign_color(cloud);

    cv::Mat rgb_valid = cv::Mat(cloud.m_height, cloud.m_width, CV_32FC3);  //rgb valid has to be in the size of whatever depth image created the cloud

    for(int y=0; y<cloud.m_height; y++){
        for(int x=0; x<cloud.m_width; x++){

            int idx= y*cloud.m_width + x;

            if(cloud.C.row(idx).isZero()){
                rgb_valid.at<cv::Vec3f>(y, x) [0]=0;
                rgb_valid.at<cv::Vec3f>(y, x) [1]=0;
                rgb_valid.at<cv::Vec3f>(y, x) [2]=0;
            }else{
                rgb_valid.at<cv::Vec3f>(y, x) [0]=cloud.C(idx,2);
                rgb_valid.at<cv::Vec3f>(y, x) [1]=cloud.C(idx,1);
                rgb_valid.at<cv::Vec3f>(y, x) [2]=cloud.C(idx,0);
            }
        }
    }

    // directions_mesh->m_vis.m_show_points=true;

    return rgb_valid;

}

Eigen::Vector3f Frame::pos_in_world(){
    return tf_cam_world.inverse().translation();
}

Eigen::Vector3f Frame::look_dir(){
    // return tf_cam_world.inverse().translation();
    Eigen::Matrix3f rot= tf_cam_world.inverse().linear();
    Eigen::Vector3f dir= rot.col(2); 
    dir=dir.normalized();
    return dir;
}


// #ifdef WITH_TORCH
//     torch::Tensor Frame::rgb2tensor(){
//         CHECK(rgb_32f.data) << "There is no data for the rgb_32f image. Are you sure this frame contains the image?";

//         torch::Tensor img_tensor=torch::from_blob(rgb_32f.data, {rgb_32f.rows, rgb_32f.cols, 3});
        
//         return img_tensor;
//     }
//     torch::Tensor Frame::depth2tensor(){
//         CHECK(depth.data) << "There is no data for the depth image. Are you sure this frame contains the image?";

//         torch::Tensor img_tensor=torch::from_blob(depth.data, {depth.rows, depth.cols, 1});
        
//         return img_tensor;
//     }
//     void Frame::tensor2rgb(const torch::Tensor& tensor){

//         CHECK(tensor.size(2)==3) << "We assuming that the tensor should have fromat H,W,C and the C should be 3. However the tensor has shape " << tensor.sizes();
        
//         torch::Tensor tensor_cpu=tensor.to("cpu");

//         rgb_32f=cv::Mat(tensor.size(0), tensor.size(1), CV_32FC3 );
//         std::memcpy( rgb_32f.data, tensor_cpu.data_ptr<float>(), tensor.size(0)*tensor.size(1)*3*sizeof(float) );
//     }
//     void Frame::tensor2gray(const torch::Tensor& tensor){

//         CHECK(tensor.size(2)==1) << "We assuming that the tensor should have fromat H,W,C and the C should be 1. However the tensor has shape " << tensor.sizes();
        
//         torch::Tensor tensor_cpu=tensor.to("cpu");

//         gray_32f=cv::Mat(tensor.size(0), tensor.size(1), CV_32FC1 );
//         std::memcpy( gray_32f.data, tensor_cpu.data_ptr<float>(), tensor.size(0)*tensor.size(1)*1*sizeof(float) );
//     }
//     void Frame::tensor2depth(const torch::Tensor& tensor){

//         CHECK(tensor.size(2)==1) << "We assuming that the tensor should have fromat H,W,C and the C should be 3. However the tensor has shape " << tensor.sizes();
        
//         torch::Tensor tensor_cpu=tensor.to("cpu");

//         depth=cv::Mat(tensor.size(0), tensor.size(1), CV_32FC1 );
//         std::memcpy( depth.data, tensor_cpu.data_ptr<float>(), tensor.size(0)*tensor.size(1)*1*sizeof(float) );
//     }
// #endif

} //namespace easy_pbr