#include "easy_pbr/Frame.h"
#include "easy_pbr/Scene.h"

#include "UtilsGL.h"
#include "opencv_utils.h"

//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

namespace easy_pbr {

Frame::Frame()
        {

}


std::shared_ptr<Mesh> Frame::create_frustum_mesh(float scale_multiplier, bool show_texture) const{

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";

    // https://gamedev.stackexchange.com/questions/29999/how-do-i-create-a-bounding-frustum-from-a-view-projection-matrix
    MeshSharedPtr frustum_mesh=Mesh::create();

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
        1, 1, 1, //top right
        -1, 1, 1, //top left
        -1, -1, 1, //bottom left
        1, -1, 1; //bottom right


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

    frustum_mesh->V=frustrum_in_world_postw.cast<double>();
    frustum_mesh->E=E;
    frustum_mesh->m_vis.m_show_mesh=false;
    frustum_mesh->m_vis.m_show_lines=true;

    //since we want somtimes to see the axes of the camera, we actually make the vertices be at the origin an then use the model matrix to trasnform them while rendering. And then through the imguimzo i can check the axes
    //transform from world to camera 
    frustum_mesh->transform_vertices_cpu(tf_cam_world.cast<double>(), true);
    frustum_mesh->set_model_matrix( tf_cam_world.cast<double>().inverse() );


    //make also a mesh from the far face so that we can display a texture
    if(show_texture){
        
        //try to get a mat that has texture
        cv::Mat tex;
        if(tex.empty() && !rgb_8u.empty()){  tex=rgb_8u;  }
        if(tex.empty() && !gray_8u.empty()){ tex=gray_8u;  }
        if(tex.empty() && !rgb_32f.empty()){  tex=rgb_32f;  }
        if(tex.empty() && !gray_32f.empty()){  tex=gray_32f;  }

        //if we found a texture 
        if(!tex.empty()){

            Eigen::MatrixXi F(2,3);
            F << 4,5,6,
                4,6,7;
            Eigen::MatrixXd NV(8,3);
            NV << 
                // near face doesnt need any normals because we don't make any faces there but we just put summy dummy normals
                0, 0, 1,
                0, 0, 1,
                0, 0, 1,
                0, 0, 1,
                //far face (here we have defined the faces for the texturing and we set also the normals to be pointing towards the camera)
                0, 0, 1,
                0, 0, 1,
                0, 0, 1,
                0, 0, 1;

            //the origin of uv has 0,0 at the bottom left so the ndc vertex with position -1,-1,1
            Eigen::MatrixXd UV(8,2);
            UV << 
                // near face (doesnt actually need uvs)
                1, 1, //top right
                0, 1, // top left
                0, 0, //bottom left
                1, 0, //bottom right
                //far face
                1, 1, //top right 
                0, 1, // top left corner of ndc
                0, 0, //bottom left
                1, 0; //bottom right
            frustum_mesh->F=F;
            frustum_mesh->NV=NV;
            frustum_mesh->UV=UV;
        
            //resize 
            int max_size=256.0;
            if(tex.cols>max_size || tex.rows> max_size){
                int subsample_factor= std::ceil( std::max(tex.cols, tex.rows)/max_size );
                cv::Mat resized;
                cv::resize(tex, resized, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA );
                tex=resized;
            }

            if( !tex.empty() ){
                frustum_mesh->set_diffuse_tex(tex);
            }
            frustum_mesh->m_vis.m_show_mesh=true;
            frustum_mesh->m_vis.set_color_texture();

        }
    }


    return frustum_mesh;

}

cv::Mat Frame::depth2world_xyz_mat() const{

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(depth.channels()==1) << "We assume that the depth should have only one channel but it has " << depth.channels();
    CHECK(depth.type()==CV_32FC1) << "We assume that the depth should be of type CV_32FC1 but it is " << radu::utils::type2string(depth.type() );
    CHECK(depth.data) << "There is no data for the depth image. Are you sure this frame contains depth image?";

    cv::Mat mat_xyz = cv::Mat(depth.rows, depth.cols, CV_32FC3);
    mat_xyz=0.0;
    Eigen::Affine3d tf_world_cam=tf_cam_world.inverse().cast<double>();

    //backproject the depth into the 3D world
    for(int y=0; y<depth.rows; y++){
        for(int x=0; x<depth.cols; x++){
            // int idx_insert= y*depth.cols + x;

            float depth_val = depth.at<float>(y,x); 
            if(depth_val!=0.0){

                Eigen::Vector3d point_2D;
                //the point is not at x,y but at x, heght-y. That's because we got the depth from the depth map at x,y and we have to take into account that opencv mat has origin at the top left. However the camera coordinate system actually has origin at the bottom left (same as the origin of the uv space in opengl) So the point in screen coordinates will be at x, height-y
                point_2D << x, depth.rows-y, 1.0;  
                Eigen::Vector3d point_3D_camera_coord= K.cast<double>().inverse() * point_2D; 
                point_3D_camera_coord*=depth_val;
                Eigen::Vector3d point_3D_world_coord=tf_world_cam*point_3D_camera_coord;

                mat_xyz.at<cv::Vec3f>(y, x) [0] = point_3D_world_coord.x();
                mat_xyz.at<cv::Vec3f>(y, x) [1] = point_3D_world_coord.y();
                mat_xyz.at<cv::Vec3f>(y, x) [2] = point_3D_world_coord.z();


                // //attempt 2 doing it more like https://github.com/IntelRealSense/librealsense/blob/8594d09f092347a8b3d832d14e4fb631140620c5/src/gl/pointcloud-gl.cpp
                // float focal1=K(0,0);
                // float focal2=K(1,1);
                // float principal_x=K(0,2);
                // float principal_y=K(1,2);

                // float x_val= (x-principal_x)/focal1;
                // float y_val= (y-principal_y)/focal2;

                // float coeffs1[5];
                // // 0.15755560994148254, -0.4658984839916229, -0.0013873715652152896, -0.00021726528939325362, 0.41800209879875183
                // coeffs1[0]=0.15755560994148254;
                // coeffs1[1]=-0.4658984839916229;
                // coeffs1[2]=-0.0013873715652152896;
                // coeffs1[3]=-0.00021726528939325362;
                // coeffs1[4]=0.41800209879875183;

                // float r2  = x_val*x_val + y_val*y_val;
                // float f = 1.0 + coeffs1[0]*r2 + coeffs1[1]*r2*r2 + coeffs1[4]*r2*r2*r2;
                // float ux = x_val*f + 2.0*coeffs1[2]*x_val*y_val + coeffs1[3]*(r2 + 2.0*x_val*x_val);
                // float uy = y_val*f + 2.0*coeffs1[3]*x_val*y_val + coeffs1[2]*(r2 + 2.0*y_val*y_val);
                // x_val = ux;
                // y_val = uy;

                // Eigen::Vector3d point_3D_camera_coord;
                // point_3D_camera_coord << x_val*depth_val, y_val*depth_val, depth_val;
                // Eigen::Vector3d point_3D_world_coord=tf_world_cam*point_3D_camera_coord;
                // mat_xyz.at<cv::Vec3f>(y, x) [0] = point_3D_world_coord.x();
                // mat_xyz.at<cv::Vec3f>(y, x) [1] = point_3D_world_coord.y();
                // mat_xyz.at<cv::Vec3f>(y, x) [2] = point_3D_world_coord.z();


            }

        }
    }

    return mat_xyz;

}

std::shared_ptr<Mesh> Frame::depth2world_xyz_mesh() const{


    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(depth.channels()==1) << "We assume that the depth should have only one channel but it has " << depth.channels();
    CHECK(depth.type()==CV_32FC1) << "We assume that the depth should be of type CV_32FC1 but it is " << radu::utils::type2string(depth.type() );
    CHECK(depth.data) << "There is no data for the depth image. Are you sure this frame contains depth image?";

    cv::Mat depth_xyz=depth2world_xyz_mat();

    //Make a series of point in sensor frame 
    Eigen::MatrixXd V;
    // Eigen::MatrixXd D;
    V.resize(depth_xyz.rows*depth_xyz.cols,3);
    // D.resize(depth_xyz.rows*depth_xyz.cols,1);
    V.setZero();
    for(int y=0; y<depth_xyz.rows; y++){
        for(int x=0; x<depth_xyz.cols; x++){
            int idx_insert= y*depth_xyz.cols + x;
            V.row(idx_insert) << depth_xyz.at<cv::Vec3f>(y, x) [0], depth_xyz.at<cv::Vec3f>(y, x) [1], depth_xyz.at<cv::Vec3f>(y, x) [2]; 
        }
    }

    // //puts it in camera frame 
    // V=V*K.cast<double>().inverse().transpose(); 
    // // V=V.rowwise()*D.array();
    // for(int i=0; i<V.rows(); i++){
    //     V(i,0)=V(i,0)*D(i,0); 
    //     V(i,1)=V(i,1)*D(i,0); 
    //     V(i,2)=V(i,2)*D(i,0); 
    // }

 

    MeshSharedPtr cloud=Mesh::create();
    cloud->V=V;
    // cloud->D=D;
    cloud->m_width=depth_xyz.cols;
    cloud->m_height=depth_xyz.rows;
    cloud->m_vis.m_show_points=true;


    return cloud;

}

std::shared_ptr<Mesh> Frame::pixels2dirs_mesh() const{
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";

    MeshSharedPtr directions_mesh=Mesh::create();
    directions_mesh->V.resize(width*height, 3);

    int idx_insert=0;
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            Eigen::Vector3f point_screen;
            //the point is not at x,y but at x, heght-y. That's because we got the depth from the depth map at x,y and we have to take into account that opencv mat has origin at the top left. However the camera coordinate system actually has origin at the bottom left (same as the origin of the uv space in opengl) So the point in screen coordinates will be at x, height-y
            point_screen << x,height-y,1.0;

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

std::shared_ptr<Mesh> Frame::pixels2_euler_angles_mesh() const{
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";

    std::shared_ptr<Mesh> dirs_mesh = pixels2dirs_mesh();


    for(int i=0; i<dirs_mesh->V.rows(); i++){
        Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors( dirs_mesh->V.row(i), -Eigen::Vector3d::UnitZ() );
        Eigen::Matrix3d rot = q.toRotationMatrix();
        Eigen::Vector3d euler_angles= rot.eulerAngles(0,1,2);
        dirs_mesh->V.row(i) = euler_angles;
    }

    dirs_mesh->m_vis.m_show_points=true;
    dirs_mesh->m_width=width;
    dirs_mesh->m_height=height;

    return dirs_mesh;

}

std::shared_ptr<Mesh> Frame::assign_color(std::shared_ptr<Mesh>& cloud) const{

    //check that we can get rgb color
    CHECK(!rgb_8u.empty() || !rgb_32f.empty() ) << "There is no rgb data";
    // bool rgb_8u_has_data;
    // VLOG(1) << "rgb_8u" << rgb_8u.empty();
    // VLOG(1) << "rgb_32f" << rgb_32f.empty();
    cv::Mat color_mat;
    if (rgb_32f.empty()){
        // VLOG(1) << "rgb32f is empty";
        rgb_8u.convertTo(color_mat, CV_32FC3, 1.0/255.0);
    }else{
        color_mat=rgb_32f;
    }


    Eigen::MatrixXd V_transformed;
    V_transformed.resize(cloud->V.rows(), cloud->V.cols());
    V_transformed.setZero();
    cloud->C.resize(cloud->V.rows(), 3);
    cloud->C.setZero();
    cloud->UV.resize(cloud->V.rows(),2);
    cloud->UV.setZero();



    // transform from world into this frame coords
    // V_transformed=tf_cam_world*cloud.V;
    Eigen::Affine3d trans=tf_cam_world.cast<double>();
    int nr_valid=0;
    for (int i = 0; i < cloud->V.rows(); i++) {
        if(!cloud->V.row(i).isZero()){
            V_transformed.row(i)=trans.linear()*cloud->V.row(i).transpose() + trans.translation();
            nr_valid++;
        }
    }


    //project
    V_transformed=V_transformed*K.cast<double>().transpose(); 
    int nr_points_projected=0;
    for (int i = 0; i < cloud->V.rows(); i++) {
        V_transformed(i,0)/=V_transformed(i,2);
        V_transformed(i,1)/=V_transformed(i,2);
        // V_transformed(i,2)=1.0;

        //V_transformed is now int he screen coordinates which has origin at the lower left as per normal UV coordinates of opengl. However Opencv considers the origin to be at the top left so we need the y to be height-V_transformed(i,1)
        int x=V_transformed(i,0);
        int y=height-V_transformed(i,1);
        if(y<0 || y>=color_mat.rows || x<0 || x>color_mat.cols){
            continue;
        }
        nr_points_projected++;

        // // VLOG(1) << "accessing at y,x" << y << " " << x;
        // // store Color in C as RGB
        cloud->C(i,0)=color_mat.at<cv::Vec3f>(y, x) [ 2 ]; 
        cloud->C(i,1)=color_mat.at<cv::Vec3f>(y, x) [ 1 ];
        cloud->C(i,2)=color_mat.at<cv::Vec3f>(y, x) [ 0 ];

        cloud->UV(i,0) = V_transformed(i,0)/color_mat.cols;
        cloud->UV(i,1) = V_transformed(i,1)/color_mat.rows;
    }
    // VLOG(1) << "nr_points_projected" << nr_points_projected;

    cloud->m_vis.set_color_pervertcolor();


    return cloud;

}

cv::Mat Frame::rgb_with_valid_depth(const Frame& frame_depth) const{
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";
    CHECK(frame_depth.depth.data) << "frame depth does not have depth data assigned";
    CHECK(rgb_32f.data) << "current does not have rgb_32f data assigned";


    MeshSharedPtr cloud= frame_depth.depth2world_xyz_mesh();
    cloud=assign_color(cloud);

    cv::Mat rgb_valid = cv::Mat(cloud->m_height, cloud->m_width, CV_32FC3);  //rgb valid has to be in the size of whatever depth image created the cloud

    for(int y=0; y<cloud->m_height; y++){
        for(int x=0; x<cloud->m_width; x++){

            int idx= y*cloud->m_width + x;

            if(cloud->C.row(idx).isZero()){
                rgb_valid.at<cv::Vec3f>(y, x) [0]=0;
                rgb_valid.at<cv::Vec3f>(y, x) [1]=0;
                rgb_valid.at<cv::Vec3f>(y, x) [2]=0;
            }else{
                //get the RGB data from C and store it as BGR in the mat
                rgb_valid.at<cv::Vec3f>(y, x) [0]=cloud->C(idx,2);
                rgb_valid.at<cv::Vec3f>(y, x) [1]=cloud->C(idx,1);
                rgb_valid.at<cv::Vec3f>(y, x) [2]=cloud->C(idx,0);
            }
        }
    }

    // directions_mesh->m_vis.m_show_points=true;

    return rgb_valid;

}

// void Frame::rotate_y_axis(const float rads ){
//     Eigen::Affine3f tf;
//     tf.setIdentity();
//     Eigen::Matrix3f tf_rot;
//     tf_rot = Eigen::AngleAxisf(rads*M_PI, Eigen::Vector3f::UnitY());
//     tf.matrix().block<3,3>(0,0)=tf_rot;

//     //transform 
//     Eigen::Affine3f tf_world_cam=tf_cam_world.inverse();
//     Eigen::Affine3f tf_world_cam_rotated= tf*tf_world_cam; //cam goes to world, and then cam rotates
//     tf_cam_world=tf_world_cam_rotated.inverse();

// }





Eigen::Vector3f Frame::pos_in_world() const{
    return tf_cam_world.inverse().translation();
}

Eigen::Vector3f Frame::look_dir() const{
    // return tf_cam_world.inverse().translation();
    Eigen::Matrix3f rot= tf_cam_world.inverse().linear();
    Eigen::Vector3f dir= rot.col(2); 
    dir=dir.normalized();
    return dir;
}




} //namespace easy_pbr