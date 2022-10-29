#include "easy_pbr/Frame.h"
#include "easy_pbr/Scene.h"
#include "easy_pbr/Camera.h"

#include "easy_gl/UtilsGL.h"
#include "opencv_utils.h"
#include <opencv2/core/eigen.hpp>

#include "RandGenerator.h"

#include "numerical_utils.h"

//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>



namespace easy_pbr {

Frame::Frame():
    m_rand_gen(new radu::utils::RandGenerator( int(time(NULL))  )) //we seed the generator with a random number otherwise all the new frames we create will start with the same seed
        {

}


std::shared_ptr<Mesh> Frame::create_frustum_mesh(float scale_multiplier, bool show_texture, const int texture_max_size, const float near_multiplier, const float far_multiplier) const{

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";

    // https://gamedev.stackexchange.com/questions/29999/how-do-i-create-a-bounding-frustum-from-a-view-projection-matrix
    MeshSharedPtr frustum_mesh=Mesh::create();

    Eigen::Matrix4f proj=intrinsics_to_opengl_proj(K, width, height, near_multiplier*scale_multiplier, far_multiplier*scale_multiplier);
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


    Eigen::MatrixXi F(2,3);
    F << 4,6,5,
        4,7,6;
    Eigen::MatrixXd NV(8,3);
    NV <<
        // near face doesnt need any normals because we don't make any faces there but we just put summy dummy normals
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        //far face (here we have defined the faces for the texturing and we set also the normals to be pointing towards the camera)
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1;

    //the origin of uv has 0,0 at the bottom left so the ndc vertex with position -1,-1,1
    Eigen::MatrixXd UV(8,2);
    UV <<
        // near face (doesnt actually need uvs)
        // 1, 1, //top right
        // 0, 1, // top left
        // 0, 0, //bottom left
        // 1, 0, //bottom right
        // //far face
        // 1, 1, //top right
        // 0, 1, // top left corner of ndc
        // 0, 0, //bottom left
        // 1, 0; //bottom right

        //we use the opencv systme in which the origin of the uv is top left https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html
        //THIS WORKS because the tf_cam_world of the frame look like the one in the link. So the X to the right, y towards bottom and Z towards the frame itself
        1, 0, //top right
        0, 0, // top left
        0, 1, //bottom left
        1, 1, //bottom right
        //far face
        1, 0, //top right
        0, 0, // top left corner of ndc
        0, 1, //bottom left
        1, 1; //bottom right
    frustum_mesh->F=F;
    frustum_mesh->NV=NV;
    frustum_mesh->UV=UV;

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




            //resize
            // int max_size=texture_max_size;
            if(tex.cols>texture_max_size || tex.rows> texture_max_size){
                int subsample_factor= std::ceil( std::max(tex.cols, tex.rows)/texture_max_size );
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


Frame Frame::random_crop(const int crop_height, const int crop_width){
    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(crop_width<width) << "Crop width is larger than width of the image. Crop width is " << crop_width << " width of this frame is " << width;
    CHECK(crop_height<width) << "Crop height is larger than height of the image. Crop height is " << crop_height << " height of this frame is " << height;

    //I use this random nr generator from opencv because I don't want to have in this class a RandGenerator from my utils library because it doesnt have a global state so recreating the frame, will recreate a rng with the same seed
    int rand_x=m_rand_gen->rand_int(0,width-1-crop_width);
    int rand_y=m_rand_gen->rand_int(0,height-1-crop_height);

    return this->crop(rand_x, rand_y, crop_width, crop_height, false); //we set the clamping to false because we want to make sure we get exactly something of that specific height and width
   
}
Frame Frame::crop(int start_x, int start_y, int crop_width, int crop_height, bool clamp_to_borders){
    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(crop_width<width) << "Crop width is larger than width of the image. Crop width is " << crop_width << " width of this frame is " << width;
    CHECK(crop_height<width) << "Crop height is larger than height of the image. Crop height is " << crop_height << " height of this frame is " << height;
    CHECK(crop_width >=0) << "crop_width should be positive but it is " <<crop_width;
    CHECK(crop_height >=0) << "crop_height should be positive but it is " <<crop_height;


    //DO NOT DO any bounding becasue I want to make sure that whatever I give this function, it will crop to exactly that size, it is quite important when I do various crops to make sure that they are those exact size, and to fail if that size cannot be recovered
    if (clamp_to_borders){
        // //bound the crop in case we are outside of the image
        // int clamped_start_x=radu::utils::clamp(start_x,0,width);
        // int clamped_start_y=radu::utils::clamp(start_y,0,height);
        // //in case the start x or y got clamped we should also modify the width because now it should be shorter
        // crop_width=crop_width-std::abs(start_x-clamped_start_x);
        // crop_height=crop_height-std::abs(start_y-clamped_start_y);
        // //now we replace the start x and y with the clamped one
        // start_x=clamped_start_x;
        // start_y=clamped_start_y;
        // //clamp also the width and height
        // crop_width=radu::utils::clamp(crop_width,0,width-start_x);
        // crop_height=radu::utils::clamp(crop_height,0,height-start_y);
        auto valid_crop=get_valid_crop(start_x, start_y, crop_width, crop_height);
        start_x=std::get<0>(valid_crop);
        start_y=std::get<1>(valid_crop);
        crop_width=std::get<2>(valid_crop);
        crop_height=std::get<3>(valid_crop);
    }

    Frame new_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats

    cv::Rect rect_crop(start_x, start_y, crop_width, crop_height);

    if(!rgb_8u.empty())    new_frame.rgb_8u=rgb_8u(rect_crop).clone();
    if(!rgb_32f.empty())   new_frame.rgb_32f=rgb_32f(rect_crop).clone();
    if(!gray_8u.empty())   new_frame.gray_8u=gray_8u(rect_crop).clone();
    if(!gray_32f.empty())   new_frame.gray_32f=gray_32f(rect_crop).clone();
    if(!grad_x_32f.empty())  new_frame.grad_x_32f=grad_x_32f(rect_crop).clone();
    if(!grad_y_32f.empty())  new_frame.grad_y_32f=grad_y_32f(rect_crop).clone();
    if(!gray_with_gradients.empty())  new_frame.gray_with_gradients=gray_with_gradients(rect_crop).clone();
    if(!thermal_16u.empty())  new_frame.thermal_16u=thermal_16u(rect_crop).clone();
    if(!thermal_32f.empty()) new_frame.thermal_32f=thermal_32f(rect_crop).clone();
    if(!thermal_vis_32f.empty()) new_frame.thermal_vis_32f=thermal_vis_32f(rect_crop).clone();
    if(!mask.empty())   new_frame.mask=mask(rect_crop).clone();
    if(!depth.empty())  new_frame.depth=depth(rect_crop).clone();
    if(!depth_along_ray.empty())  new_frame.depth_along_ray=depth_along_ray(rect_crop).clone();
    if(!confidence.empty())  new_frame.confidence=confidence(rect_crop).clone();

    //ajust principal point
    new_frame.K(0,2) = K(0,2) - start_x;
    new_frame.K(1,2) = K(1,2) - start_y;

    //set the new width and height
    new_frame.width=crop_width;
    new_frame.height=crop_height;

    //set the crop of this.
    new_frame.add_extra_field("crop_x", start_x);
    new_frame.add_extra_field("crop_y", start_y);
    new_frame.add_extra_field("crop_width", crop_width);
    new_frame.add_extra_field("crop_height", crop_height);
    new_frame.add_extra_field("width_before_crop", width);
    new_frame.add_extra_field("height_before_crop", height);

    return new_frame;
}

std::tuple<int,int,int,int> Frame::get_valid_crop(int start_x, int start_y, int crop_width, int crop_height){
    //bound the crop in case we are outside of the image
    int clamped_start_x=radu::utils::clamp(start_x,0,width);
    int clamped_start_y=radu::utils::clamp(start_y,0,height);
    //in case the start x or y got clamped we should also modify the width because now it should be shorter
    crop_width=crop_width-std::abs(start_x-clamped_start_x);
    crop_height=crop_height-std::abs(start_y-clamped_start_y);
    //now we replace the start x and y with the clamped one
    start_x=clamped_start_x;
    start_y=clamped_start_y;
    //clamp also the width and height
    crop_width=radu::utils::clamp(crop_width,0,width-start_x);
    crop_height=radu::utils::clamp(crop_height,0,height-start_y);


    return std::make_tuple(start_x, start_y, crop_width, crop_height);
}

std::tuple<int,int,int,int> Frame::enlarge_crop_to_size(int start_x, int start_y, int crop_width, int crop_height, int desired_width, int desired_height){
    CHECK(desired_width>=crop_width) << "For now this only enlarges the crop to fit this desired width. I did not yet implement a shrinking";
    CHECK(desired_height>=crop_height) << "For now this only enlarges the crop to fit this desired height. I did not yet implement a shrinking";

    auto valid_crop=get_valid_crop(start_x, start_y, crop_width, crop_height);
    start_x=std::get<0>(valid_crop);
    start_y=std::get<1>(valid_crop);
    crop_width=std::get<2>(valid_crop);
    crop_height=std::get<3>(valid_crop);

    int diff_width=desired_width-crop_width;
    int diff_height=desired_height-crop_height;

    //ENLARGE THE WIDTH
    int half_diff_width=diff_width/2; //floors the result
    //decrease_start_x by at most half_diff_width and increase crop_width also by at least half_diff_width
    int new_start_x=radu::utils::clamp(start_x-half_diff_width,0,width);
    int increase_in_width=start_x-new_start_x;
    crop_width+=increase_in_width;
    diff_width-=increase_in_width;
    start_x=new_start_x;
    //increase also the crop_width by as much as we can
    int new_crop_width=radu::utils::clamp(crop_width+half_diff_width, 0, width-start_x);
    increase_in_width=new_crop_width-crop_width;
    diff_width-=increase_in_width;
    crop_width=new_crop_width;
    //at this point we could still possibly not have reached the desired width because either new_start_x or new_crop_width has clamped. So we increase each one of them independenlty now
    if(crop_width!=desired_width){
        diff_width=desired_width-crop_width;
        //try to decrease the start_x
        new_start_x=radu::utils::clamp(start_x-diff_width,0,width);
        increase_in_width=start_x-new_start_x;
        crop_width+=increase_in_width;
        diff_width-=increase_in_width;
        start_x=new_start_x;
        if(increase_in_width==diff_width){
            //we matched the desired with, we are done
        }else{
            //we have to increase the crop width
            new_crop_width=radu::utils::clamp(crop_width+diff_width, 0, width-start_x);
            increase_in_width=new_crop_width-crop_width;
            diff_width-=increase_in_width;
            crop_width=new_crop_width;
        }
    }






    //ENLARGE HEIGHT
    int half_diff_height=diff_height/2; //floors the result
    //decrease_start_y by at most half_diff_height and increase crop_height also by at least half_diff_height
    int new_start_y=radu::utils::clamp(start_y-half_diff_height,0,height);
    int increase_in_height=start_y-new_start_y;
    crop_height+=increase_in_height;
    diff_height-=increase_in_height;
    start_y=new_start_y;
    //increase also the crop_width by as much as we can
    int new_crop_height=radu::utils::clamp(crop_height+half_diff_height, 0, height-start_y);
    increase_in_height=new_crop_height-crop_height;
    diff_height-=increase_in_height;
    crop_height=new_crop_height;
    //at this point we could still possibly not have reached the desired height because either new_start_y or new_crop_height has clamped. So we increase each one of them independenlty now
    if(crop_height!=desired_height){
        diff_height=desired_height-crop_height;
        //try to decrease the start_x
        new_start_y=radu::utils::clamp(start_y-diff_height,0,height);
        increase_in_height=start_y-new_start_y;
        crop_height+=increase_in_height;
        diff_height-=increase_in_height;
        start_y=new_start_y;
        if(increase_in_height==diff_height){
            //we matched the desired with, we are done
        }else{
            //we have to increase the crop width
            new_crop_height=radu::utils::clamp(crop_height+diff_height, 0, height-start_y);
            increase_in_height=new_crop_height-crop_height;
            diff_height-=increase_in_height;
            crop_height=new_crop_height;
        }
    }


    CHECK(crop_width==desired_width) << "We didn't reach the desired_width for some reason. The current width is " << crop_width << " while the desired width is " << desired_width << ". The maximum width of the frame is "<< this->width;
    CHECK(crop_height==desired_height) << "We didn't reach the height for some reason. The current height is " << crop_height << " while the desired height is " << desired_height << ". The maximum height of the frame is "<< this->height;
    

    return std::make_tuple(start_x, start_y, crop_width, crop_height);
}

void Frame::rescale_K(const float scale){
    K.block(0,0,2,2)*=scale;
    K(0,2) = (K(0,2) + 0.5)*scale  - 0.5;
    K(1,2) = (K(1,2) + 0.5)*scale  - 0.5;
}

Frame Frame::subsample(const float subsample_factor, bool subsample_imgs){
    CHECK(subsample_factor>=1.0) << "Subsample factor is below 1.0 which means it would actually do a upsampling. For that please use frame.upsample()";

    Frame new_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats


    // cv::Mat rgb_8u;
    // cv::Mat rgb_32f; // becasue some algorithms like the cnns require floating point tensors
    // // cv::Mat_<cv::Vec4b> rgba_8u; //Opengl likes 4 channels images
    // // cv::Mat_<cv::Vec4f> rgba_32f; //Opengl likes 4 channels images
    // cv::Mat gray_8u;
    // cv::Mat gray_32f;
    // cv::Mat grad_x_32f;
    // cv::Mat grad_y_32f;
    // cv::Mat gray_with_gradients; //gray image and grad_x and grad_y into one RGB32F image, ready to be uploaded to opengl

    // cv::Mat thermal_16u;
    // cv::Mat thermal_32f;
    // cv::Mat thermal_vis_32f; //for showing only we make the thermal into a 3 channel one so imgui shows it in black and white

    // cv::Mat img_original_size; //the image in the original size, iwthout any subsampling. Usefult for texturing

    // cv::Mat mask;
    // cv::Mat depth;

    if (subsample_imgs){
        if(!rgb_8u.empty()) cv::resize(rgb_8u, new_frame.rgb_8u, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!rgb_32f.empty()) cv::resize(rgb_32f, new_frame.rgb_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!gray_8u.empty()) cv::resize(gray_8u, new_frame.gray_8u, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!gray_32f.empty()) cv::resize(gray_32f, new_frame.gray_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!grad_x_32f.empty()) cv::resize(grad_x_32f, new_frame.grad_x_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!grad_y_32f.empty()) cv::resize(grad_y_32f, new_frame.grad_y_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!gray_with_gradients.empty()) cv::resize(gray_with_gradients, new_frame.gray_with_gradients, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!thermal_16u.empty()) cv::resize(thermal_16u, new_frame.thermal_16u, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!thermal_32f.empty()) cv::resize(thermal_32f, new_frame.thermal_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        if(!thermal_vis_32f.empty()) cv::resize(thermal_vis_32f, new_frame.thermal_vis_32f, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
        //mask and depth do inter_NEAREST because otherwise the mask will not be binary anymore and the depth will merge depth from depth dicontinuities which is bad
        if(!mask.empty()) cv::resize(mask, new_frame.mask, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_NEAREST);
        if(!depth.empty()) cv::resize(depth, new_frame.depth, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_NEAREST);
        if(!depth_along_ray.empty()) cv::resize(depth_along_ray, new_frame.depth_along_ray, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_NEAREST);
        if(!confidence.empty()) cv::resize(confidence, new_frame.confidence, cv::Size(), 1.0/subsample_factor, 1.0/subsample_factor, cv::INTER_AREA);
    }

    //deal with the other things that changed now that the image is smaller
    // new_frame.K/=subsample_factor;
    // new_frame.K(2,2)=1.0;
    new_frame.rescale_K(1.0/subsample_factor);
    // new_frame.height=std::round( (float)height/subsample_factor);
    // new_frame.width=std::round( (float)width/subsample_factor);
    //need to use the cvround because the cv::resize uses that one to compute teh new size of the image and std::round gives different results
    new_frame.height=cvRound( (float)height/subsample_factor);
    new_frame.width=cvRound( (float)width/subsample_factor);

    //copy all stuff;
    return new_frame;
}

Frame Frame::upsample(const float upsample_factor, bool upsample_imgs){
    CHECK(upsample_factor>=1.0) << "Upsample factor is below 1.0 which means it would actually do a subsampling. For that please use frame.subsample()";

    Frame new_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats


    // cv::Mat rgb_8u;
    // cv::Mat rgb_32f; // becasue some algorithms like the cnns require floating point tensors
    // // cv::Mat_<cv::Vec4b> rgba_8u; //Opengl likes 4 channels images
    // // cv::Mat_<cv::Vec4f> rgba_32f; //Opengl likes 4 channels images
    // cv::Mat gray_8u;
    // cv::Mat gray_32f;
    // cv::Mat grad_x_32f;
    // cv::Mat grad_y_32f;
    // cv::Mat gray_with_gradients; //gray image and grad_x and grad_y into one RGB32F image, ready to be uploaded to opengl

    // cv::Mat thermal_16u;
    // cv::Mat thermal_32f;
    // cv::Mat thermal_vis_32f; //for showing only we make the thermal into a 3 channel one so imgui shows it in black and white

    // cv::Mat img_original_size; //the image in the original size, iwthout any subsampling. Usefult for texturing

    // cv::Mat mask;
    // cv::Mat depth;

    if (upsample_imgs) {
        if(!rgb_8u.empty()) cv::resize(rgb_8u, new_frame.rgb_8u, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!rgb_32f.empty()) cv::resize(rgb_32f, new_frame.rgb_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!gray_8u.empty()) cv::resize(gray_8u, new_frame.gray_8u, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!gray_32f.empty()) cv::resize(gray_32f, new_frame.gray_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!grad_x_32f.empty()) cv::resize(grad_x_32f, new_frame.grad_x_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!grad_y_32f.empty()) cv::resize(grad_y_32f, new_frame.grad_y_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!gray_with_gradients.empty()) cv::resize(gray_with_gradients, new_frame.gray_with_gradients, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!thermal_16u.empty()) cv::resize(thermal_16u, new_frame.thermal_16u, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!thermal_32f.empty()) cv::resize(thermal_32f, new_frame.thermal_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        if(!thermal_vis_32f.empty()) cv::resize(thermal_vis_32f, new_frame.thermal_vis_32f, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
        //mask and depth do inter_NEAREST because otherwise the mask will not be binary anymore and the depth will merge depth from depth dicontinuities which is bad
        if(!mask.empty()) cv::resize(mask, new_frame.mask, cv::Size(), upsample_factor, upsample_factor,  cv::INTER_NEAREST);
        if(!depth.empty()) cv::resize(depth, new_frame.depth, cv::Size(), upsample_factor, upsample_factor, cv::INTER_NEAREST);
        if(!depth_along_ray.empty()) cv::resize(depth_along_ray, new_frame.depth_along_ray, cv::Size(), upsample_factor, upsample_factor, cv::INTER_NEAREST);
        if(!confidence.empty()) cv::resize(confidence, new_frame.confidence, cv::Size(), upsample_factor, upsample_factor, cv::INTER_LINEAR);
    }


    //deal with the other things that changed now that the image is smaller
    // new_frame.K*=upsample_factor;
    // new_frame.K(2,2)=1.0;
    new_frame.rescale_K(upsample_factor);
    // new_frame.height=std::round( (float)height/subsample_factor);
    // new_frame.width=std::round( (float)width/subsample_factor);
    //need to use the cvround because the cv::resize uses that one to compute teh new size of the image and std::round gives different results
    new_frame.height=cvRound( (float)height*upsample_factor);
    new_frame.width=cvRound( (float)width*upsample_factor);

    //copy all stuff;
    return new_frame;
}


Frame Frame::undistort(){
    CHECK( !distort_coeffs.isZero() ) << "The distorsion coefficients are zero so there is nothing to undistort";
    CHECK( !get_rgb_mat().empty()) << "The undistorsion assumes we have either a rgb_8u or rgb_32f but both are empty";

    // Frame new_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats

    Eigen::Matrix<float, 4, 1> distort_coeffs_reduced; //we trim to the 4 coefficients
    distort_coeffs_reduced(0)=distort_coeffs(0);
    distort_coeffs_reduced(1)=distort_coeffs(1);
    distort_coeffs_reduced(2)=distort_coeffs(2);
    distort_coeffs_reduced(3)=distort_coeffs(3);

    //get opencv matrices
    cv::Mat K_mat, distort_coeffs_mat, R, new_K_mat, rmap1, rmap2;

    cv::eigen2cv(K, K_mat);
    cv::eigen2cv(distort_coeffs_reduced, distort_coeffs_mat);

    cv::Size image_size = get_rgb_mat().size(); //TODO we kind assume here that we use the rgb3f one and 


    new_K_mat=cv::getOptimalNewCameraMatrix(K_mat, distort_coeffs_mat, image_size, 1.0 );
    cv::initUndistortRectifyMap (K_mat, distort_coeffs_mat, R, new_K_mat, image_size, CV_32FC1, rmap1, rmap2);

    Frame new_frame=remap(rmap1, rmap2);

    cv::cv2eigen(new_K_mat, new_frame.K);

    // new_frame.clone_mats(); //since the operations are done in place, we clone the memory assigned for the mats so the two frames actually make a deep copy of the mats

    // //undistort all images, we clone the input mat because the remap operates inplace and the src and destination mats are actually the same since we just did a shallow copy
    // if(!rgb_8u.empty())  cv::remap(rgb_8u.clone(), new_frame.rgb_8u, rmap1, rmap2, cv::INTER_LANCZOS4 );
    // if(!rgb_32f.empty())  cv::remap(rgb_32f.clone(), new_frame.rgb_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!gray_8u.empty())  cv::remap(gray_8u.clone(), new_frame.gray_8u, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!gray_32f.empty())  cv::remap(gray_32f.clone(), new_frame.gray_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!grad_x_32f.empty())  cv::remap(grad_x_32f.clone(), new_frame.grad_x_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!grad_y_32f.empty())  cv::remap(grad_y_32f.clone(), new_frame.grad_y_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!gray_with_gradients.empty())  cv::remap(gray_with_gradients.clone(), new_frame.gray_with_gradients, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!thermal_16u.empty())  cv::remap(thermal_16u.clone(), new_frame.thermal_16u, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!thermal_32f.empty())  cv::remap(thermal_32f.clone(), new_frame.thermal_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!thermal_vis_32f.empty())  cv::remap(thermal_vis_32f.clone(), new_frame.thermal_vis_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    // if(!mask.empty())  cv::remap(mask.clone(), new_frame.mask, rmap1, rmap2, cv::INTER_NEAREST);
    // if(!depth.empty())  cv::remap(depth.clone(), new_frame.depth, rmap1, rmap2, cv::INTER_NEAREST);

    return new_frame;
}

void Frame::clone_mats(){
    if(!rgb_8u.empty()) rgb_8u=rgb_8u.clone();
    if(!rgb_32f.empty()) rgb_32f=rgb_32f.clone();
    if(!gray_8u.empty()) gray_8u=gray_8u.clone();
    if(!gray_32f.empty()) gray_32f=gray_32f.clone();
    if(!grad_x_32f.empty()) grad_x_32f=grad_x_32f.clone();
    if(!grad_y_32f.empty()) grad_y_32f=grad_y_32f.clone();
    if(!gray_with_gradients.empty()) gray_with_gradients=gray_with_gradients.clone();
    if(!thermal_16u.empty()) thermal_16u=thermal_16u.clone();
    if(!thermal_32f.empty()) thermal_32f=thermal_32f.clone();
    if(!thermal_vis_32f.empty()) thermal_vis_32f=thermal_vis_32f.clone();
    if(!normal_32f.empty()) normal_32f=normal_32f.clone();
    if(!img_original_size.empty()) img_original_size=img_original_size.clone();
    if(!mask.empty()) mask=mask.clone();
    if(!depth.empty()) depth=depth.clone();
    if(!depth_along_ray.empty()) depth_along_ray=depth_along_ray.clone();
    if(!confidence.empty()) confidence=confidence.clone();
}

void Frame::unload_images(){
    if(!rgb_8u.empty()) rgb_8u.release(); CHECK(rgb_8u.empty());
    if(!rgb_32f.empty()) rgb_32f.release(); CHECK(rgb_32f.empty());
    if(!gray_8u.empty()) gray_8u.release(); CHECK(gray_8u.empty());
    if(!gray_32f.empty()) gray_32f.release(); CHECK(gray_32f.empty());
    if(!grad_x_32f.empty()) grad_x_32f.release(); CHECK(grad_x_32f.empty());
    if(!grad_y_32f.empty()) grad_y_32f.release(); CHECK(grad_y_32f.empty());
    if(!gray_with_gradients.empty()) gray_with_gradients.release(); CHECK(gray_with_gradients.empty());
    if(!thermal_16u.empty()) thermal_16u.release(); CHECK(thermal_16u.empty());
    if(!thermal_32f.empty()) thermal_32f.release(); CHECK(thermal_32f.empty());
    if(!thermal_vis_32f.empty()) thermal_vis_32f.release(); CHECK(thermal_vis_32f.empty());
    if(!normal_32f.empty()) normal_32f.release(); CHECK(normal_32f.empty());
    if(!img_original_size.empty()) img_original_size.release(); CHECK(img_original_size.empty());
    if(!mask.empty()) mask.release(); CHECK(mask.empty());
    if(!depth.empty()) depth.release(); CHECK(depth.empty());
    if(!depth_along_ray.empty()) depth_along_ray.release(); CHECK(depth_along_ray.empty());
    if(!confidence.empty()) confidence.release(); CHECK(confidence.empty());

     is_shell=true;
}

Frame Frame::remap(cv::Mat& rmap1, cv::Mat& rmap2){

    Frame new_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats

    new_frame.clone_mats(); //since the operations are done in place, we clone the memory assigned for the mats so the two frames actually make a deep copy of the mats

    //undistort all images, we clone the input mat because the remap operates inplace and the src and destination mats are actually the same since we just did a shallow copy
    if(!rgb_8u.empty())  cv::remap(rgb_8u.clone(), new_frame.rgb_8u, rmap1, rmap2, cv::INTER_LANCZOS4 );
    if(!rgb_32f.empty())  cv::remap(rgb_32f.clone(), new_frame.rgb_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!gray_8u.empty())  cv::remap(gray_8u.clone(), new_frame.gray_8u, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!gray_32f.empty())  cv::remap(gray_32f.clone(), new_frame.gray_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!grad_x_32f.empty())  cv::remap(grad_x_32f.clone(), new_frame.grad_x_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!grad_y_32f.empty())  cv::remap(grad_y_32f.clone(), new_frame.grad_y_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!gray_with_gradients.empty())  cv::remap(gray_with_gradients.clone(), new_frame.gray_with_gradients, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!thermal_16u.empty())  cv::remap(thermal_16u.clone(), new_frame.thermal_16u, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!thermal_32f.empty())  cv::remap(thermal_32f.clone(), new_frame.thermal_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!thermal_vis_32f.empty())  cv::remap(thermal_vis_32f.clone(), new_frame.thermal_vis_32f, rmap1, rmap2, cv::INTER_LANCZOS4);
    if(!mask.empty())  cv::remap(mask.clone(), new_frame.mask, rmap1, rmap2, cv::INTER_NEAREST);
    if(!depth.empty())  cv::remap(depth.clone(), new_frame.depth, rmap1, rmap2, cv::INTER_NEAREST);
    if(!depth_along_ray.empty())  cv::remap(depth_along_ray.clone(), new_frame.depth_along_ray, rmap1, rmap2, cv::INTER_NEAREST);
    if(!confidence.empty())  cv::remap(confidence.clone(), new_frame.confidence, rmap1, rmap2, cv::INTER_LANCZOS4);

    return new_frame;
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
                //the point is not at x,y but at x, heght-1-y. That's because we got the depth from the depth map at x,y and we have to take into account that opencv mat has origin at the top left. However the camera coordinate system actually has origin at the bottom left (same as the origin of the uv space in opengl) So the point in screen coordinates will be at x, height-1-y
                // point_2D << x, depth.rows-1-y, 1.0;
                point_2D << x, y, 1.0;
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
            // point_screen << x,height-1-y,1.0;
            //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
            point_screen << x+0.5, y+0.5, 1.0; //we sum 0.5 so that the direction passes through the center of the square which defines the pixel

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

std::shared_ptr<Mesh> Frame::pixels2coords() const{
    CHECK(width>0) <<"Width of this frame was not assigned";
    CHECK(height>0) <<"Height of this frame was not assigned";

    MeshSharedPtr points_mesh=Mesh::create();
    points_mesh->V.resize(width*height, 3);

    int idx_insert=0;
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            Eigen::Vector3f point_screen;
            //the point is not at x,y but at x, heght-y. That's because we got the depth from the depth map at x,y and we have to take into account that opencv mat has origin at the top left. However the camera coordinate system actually has origin at the bottom left (same as the origin of the uv space in opengl) So the point in screen coordinates will be at x, height-y
            // point_screen << x,height-1-y,1.0;
            //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
            point_screen << x,y,1.0;

            points_mesh->V.row(idx_insert)= point_screen.cast<double>();

            idx_insert++;

        }
    }

    points_mesh->m_vis.m_show_points=true;
    points_mesh->m_width=width;
    points_mesh->m_height=height;

    return points_mesh;

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

    cloud->apply_model_matrix_to_cpu(false);

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
        if(!cloud->V.row(i).isZero()){
            // VLOG(1)<< " before " <<  V_transformed.row(i);
            V_transformed(i,0)/=V_transformed(i,2);
            V_transformed(i,1)/=V_transformed(i,2);
            // V_transformed(i,2)=1.0;
            // VLOG(1)<< " after "<< V_transformed.row(i);

            //V_transformed is now int he screen coordinates which has origin at the lower left as per normal UV coordinates of opengl. However Opencv considers the origin to be at the top left so we need the y to be height-1-V_transformed(i,1)
            int x=V_transformed(i,0);
            // int y=height-1-V_transformed(i,1);
            //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
            int y=V_transformed(i,1);
            // VLOG(1) << " x and y is " << x << " " << y;
            // VLOG(1) << height << " rows is " << color_mat.rows;
            // VLOG(1) << "K is " << K;
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
    }
    // VLOG(1) << "nr_points_projected" << nr_points_projected;

    cloud->m_vis.set_color_pervertcolor();
    cloud->m_is_dirty=true;


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

Eigen::MatrixXd Frame::compute_uv(std::shared_ptr<Mesh>& cloud) const{


    //ATTEMPT 2
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

    cloud->apply_model_matrix_to_cpu(false);


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
        if(!cloud->V.row(i).isZero()){
            V_transformed(i,0)/=V_transformed(i,2);
            V_transformed(i,1)/=V_transformed(i,2);
            // V_transformed(i,2)=1.0;

            //V_transformed is now int he screen coordinates which has origin at the lower left as per normal UV coordinates of opengl. However Opencv considers the origin to be at the top left so we need the y to be height-V_transformed(i,1)
            int x=V_transformed(i,0);
            // int y=height-1-V_transformed(i,1);
            //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
            int y=V_transformed(i,1);
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
    }
    // VLOG(1) << "nr_points_projected" << nr_points_projected;

    cloud->m_vis.set_color_pervertcolor();


    return cloud->UV;


}

cv::Mat Frame::naive_splat(std::shared_ptr<Mesh>& cloud, Eigen::MatrixXf values){

    CHECK(width!=-1) << "Width was not set";
    CHECK(height!=-1) << "Height was not set";
    CHECK(values.cols()<=4) << "We only support up to 4 channels for the values. However the nr of columns in the values is " << values.cols();
    CHECK(cloud->V.rows()==values.rows() ) << "We need a value for every point in the cloud. However the cloud has nr of points " << cloud->V.rows() << " while values is " << values.rows();


    cv::Mat mat_vals;
    if(values.cols()==1){
        mat_vals = cv::Mat(this->height, this->width, CV_32FC1);
    }else if(values.cols()==2){
        mat_vals = cv::Mat(this->height, this->width, CV_32FC2);
    }else if(values.cols()==3){
        mat_vals = cv::Mat(this->height, this->width, CV_32FC3);
    }else if(values.cols()==4){
        mat_vals = cv::Mat(this->height, this->width, CV_32FC4);
    }
    mat_vals=0.0;
    //make a mat for depth in order to splat for each pixel only the point with the lowest depth
    cv::Mat mat_depth=cv::Mat(this->height, this->width, CV_32FC1);
    mat_depth=std::numeric_limits<float>::max();;



    cloud->apply_model_matrix_to_cpu(false);


    Eigen::MatrixXd V_cam_coords;
    V_cam_coords.resize(cloud->V.rows(), cloud->V.cols());
    V_cam_coords.setZero();



    // transform from world into this frame coords
    Eigen::Affine3d trans=tf_cam_world.cast<double>();
    int nr_valid=0;
    for (int i = 0; i < cloud->V.rows(); i++) {
        if(!cloud->V.row(i).isZero()){
            V_cam_coords.row(i)=trans.linear()*cloud->V.row(i).transpose() + trans.translation();
            nr_valid++;
        }
    }


    //project
    Eigen::MatrixXd V_screen_coords=V_cam_coords*K.cast<double>().transpose();
    int nr_points_projected=0;
    for (int i = 0; i < cloud->V.rows(); i++) {
        if(!cloud->V.row(i).isZero()){
            V_screen_coords(i,0)/=V_screen_coords(i,2);
            V_screen_coords(i,1)/=V_screen_coords(i,2);
            // V_transformed(i,2)=1.0;

            //V_transformed is now int he screen coordinates which has origin at the lower left as per normal UV coordinates of opengl. However Opencv considers the origin to be at the top left so we need the y to be height-V_transformed(i,1)
            int x=V_screen_coords(i,0);
            // int y=height-1-V_transformed(i,1);
            //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
            int y=V_screen_coords(i,1);
            if(y<0 || y>=this->height || x<0 || x>this->width){
                continue;
            }
            nr_points_projected++;


            float cur_depth=std::fabs(V_cam_coords(i,2));
            //check if the depth is lower than the one already in the mat_depth
            float depth_pixel=mat_depth.at<float>(y, x);
            if (cur_depth<depth_pixel){
                for (int c = 0; c < values.cols(); c++){
                    // mat_vals.at<cv::Vec3f>(y, x) [c] = values(i,c);
                    if(values.cols()==1){
                        mat_vals.at<float>(y, x) = values(i,c);
                    }else if(values.cols()==2){
                        mat_vals.at<cv::Vec2f>(y, x) [c] = values(i,c);
                    }else if(values.cols()==3){
                        mat_vals.at<cv::Vec3f>(y, x) [c] = values(i,c);
                    }else if(values.cols()==4){
                        mat_vals.at<cv::Vec4f>(y, x) [c] = values(i,c);
                    }
                }
                //change also the depth
                mat_depth.at<float>(y, x)=cur_depth;
            }




        }
    }
    // VLOG(1) << "nr_points_projected" << nr_points_projected;



    return mat_vals;

}

Eigen::Vector3d Frame::unproject(const float x, const float y, const float depth){

    Eigen::Affine3d tf_world_cam=tf_cam_world.inverse().cast<double>();

    Eigen::Vector3d point_2D;
    // point_2D << x, height-1-y, 1.0;
    //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now
    point_2D << x, y, 1.0;
    Eigen::Vector3d point_3D_camera_coord= K.cast<double>().inverse() * point_2D;
    point_3D_camera_coord*=depth;
    Eigen::Vector3d point_3D_world_coord=tf_world_cam*point_3D_camera_coord; 

    return point_3D_world_coord;
}

Eigen::Vector2d Frame::project(const Eigen::Vector3d& point_world){

    Eigen::Vector3d point_cam = tf_cam_world.cast<double>() *point_world;
    Eigen::Vector3d point_screen=K.cast<double>() * point_cam;
    point_screen.x()/=point_screen.z();
    point_screen.y()/=point_screen.z();
    // point_screen.y()=height-1-point_screen.y();
    //No need to do height-y because the tf_cam_world of the frame look like the one in the link https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html. So the X to the right, y towards bottom and Z towards the frame itself. This is consistent with the uv space of opengl now

    Eigen::Vector2d point_ret;
    point_ret << point_screen.x(), point_screen.y();

    return point_ret;
    
}

cv::Mat Frame::draw_projected_line(const Eigen::Vector3d& p0_world, const Eigen::Vector3d p1_world, const int thickness){
    Eigen::Vector2d p0_screen=project(p0_world);
    Eigen::Vector2d p1_screen=project(p1_world);

    //get the rgb img
    cv::Mat rgb_img;
    if (!rgb_8u.empty()){
        rgb_img=rgb_8u;
    }else if(!rgb_32f.empty()){
        rgb_32f.convertTo(rgb_img, CV_8U, 255);
    }else{
        LOG(FATAL)<<"There is no rgb image in this frame";
    }

    //draw
    cv::Point pt0, pt1;
    pt0.x=p0_screen.x();
    pt0.y=p0_screen.y();
    pt1.x=p1_screen.x();
    pt1.y=p1_screen.y();
    cv::line(rgb_img, pt0, pt1, cv::Scalar( 255, 0, 0 ), thickness );

    return rgb_img;
}

std::tuple<std::shared_ptr<Frame>, std::shared_ptr<Frame>, float, Eigen::Matrix4d>  Frame::rectify_stereo_pair(const int offset_disparity){
    CHECK(has_right_stereo_pair()) <<"We need a stereo pair to rectify";
    Frame& frame_left=*this;
    Frame& frame_right=*m_right_stereo_pair;
    CHECK(!frame_left.is_shell) << "You need to run load_images on the left frame because we need to know the frame size";
    CHECK(!frame_right.is_shell) << "You need to run load_images on the right frame because we need to know the frame size";

    //get the pose delta between the two cams
    Eigen::Affine3d cam_left_cam_right; //transforms cam right to the frame of cam_left
    cam_left_cam_right = (frame_left.tf_cam_world * frame_right.tf_cam_world.inverse() ).cast<double>(); //camRigth to world  * world to cam_left


     //get the matrices into opencv Mat 
    Eigen::Matrix3f K_1 = frame_left.K.cast<float>();
    cv::Mat K_1_mat = cv::Mat(3, 3, CV_32FC1, K_1.data()).clone().t();
    Eigen::Matrix3f K_2 = frame_right.K.cast<float>();
    cv::Mat K_2_mat = cv::Mat(3, 3, CV_32FC1, K_2.data()).clone().t();
    cv::Mat distCoeffs;
    //get the rotation and translation and opencv wants the transform from cam1 to cam 2 so from cam_left to cam right
    Eigen::MatrixXd R_eigen=cam_left_cam_right.inverse().linear().cast<double>();
    Eigen::VectorXd t_eigen=cam_left_cam_right.inverse().translation().cast<double>();
    // VLOG(1) << "R_eigen " << R_eigen;
    // VLOG(1) << "t_eigen " << t_eigen;
    cv::Mat R_mat = cv::Mat(3, 3, CV_64FC1, R_eigen.data()).clone().t();
    cv::Mat t_mat = cv::Mat(3, 1, CV_64FC1, t_eigen.data()).clone();
    // VLOG(1) << " R_Mat " << R_mat;
    // VLOG(1) << " t_Mat " << t_mat;

    // VLOG(1) << " frame left size " << frame_left.get_rgb_mat().size(); 
    // VLOG(1) << " frame right size " << frame_right.get_rgb_mat().size(); 


    cv::Mat rotation_cam1_mat;
    cv::Mat rotation_cam2_mat;
    cv::Mat proj_cam1;
    cv::Mat proj_cam2;
    cv::Mat Q_mat;
    cv::stereoRectify 	( K_1_mat, distCoeffs, K_2_mat, distCoeffs, frame_left.get_rgb_mat().size(), R_mat, t_mat, 
                        rotation_cam1_mat, rotation_cam2_mat, proj_cam1, proj_cam2, Q_mat);

    //perform rectification 
    cv::Mat cam_1_map_x, cam_1_map_y;
    cv::Mat cam_2_map_x, cam_2_map_y;
    cv::initUndistortRectifyMap(K_1_mat, distCoeffs, rotation_cam1_mat, proj_cam1, frame_left.get_rgb_mat().size(), CV_32FC1, cam_1_map_x, cam_1_map_y);
    cv::initUndistortRectifyMap(K_2_mat, distCoeffs, rotation_cam2_mat, proj_cam2, frame_right.get_rgb_mat().size(), CV_32FC1, cam_2_map_x, cam_2_map_y);

    Frame frame_left_rectified=frame_left.remap(cam_1_map_x, cam_1_map_y);
    Frame frame_right_rectified=frame_right.remap(cam_2_map_x, cam_2_map_y);

    // VLOG(1) << "Rctify for camera" << frame_left.cam_id;
    // VLOG(1) <<  "proj_cam1 " << proj_cam1;
    // VLOG(1) <<  "proj_cam2 " << proj_cam2;
    // VLOG(1) <<  "Q" << Q;

    // cv::Mat cam1_rectified;
    // cv::Mat cam2_rectified;
    // cv::remap(frame_4_rot.img, cam1_rectified, cam_1_map_x, cam_1_map_y, cv::INTER_LANCZOS4);
    // cv::remap(frame_5_rot.img, cam2_rectified, cam_2_map_x, cam_2_map_y, cv::INTER_LANCZOS4);

    // //print the new K matrices 
    // // VLOG(1) << "proj cam1" << proj_cam1;
    // Frame new_frame_left, new_frame_right;
    // new_frame_left.img= cam1_rectified;
    frame_left_rectified.K(0,0) =  proj_cam1.at<double>(0,0);
    frame_left_rectified.K(1,1) =  proj_cam1.at<double>(1,1);
    frame_left_rectified.K(0,2) =  proj_cam1.at<double>(0,2);
    frame_left_rectified.K(1,2) =  proj_cam1.at<double>(1,2);
    // // VLOG(1) << "New k is " << new_frame_left.K;
    // new_frame_right.img= cam2_rectified;
    frame_right_rectified.K(0,0) =  proj_cam2.at<double>(0,0);
    frame_right_rectified.K(1,1) =  proj_cam2.at<double>(1,1);
    frame_right_rectified.K(0,2) =  proj_cam2.at<double>(0,2);
    frame_right_rectified.K(1,2) =  proj_cam2.at<double>(1,2);
    float baseline=cam_left_cam_right.translation().norm();


    //since we are rotating the frame, we need to change the tf_cam_world for the frame_left and frame_right
    //the rotation matrices maps from unrectified frame to the rectified frame
    Eigen::Matrix3f rotation_cam1, rotation_cam2;
    cv::cv2eigen(rotation_cam1_mat, rotation_cam1);
    cv::cv2eigen(rotation_cam2_mat, rotation_cam2);
    Eigen::Affine3f tf_rectified_unrectified_cam1, tf_rectified_unrectified_cam2;
    tf_rectified_unrectified_cam1.setIdentity();
    tf_rectified_unrectified_cam2.setIdentity();
    tf_rectified_unrectified_cam1.linear()=rotation_cam1;
    tf_rectified_unrectified_cam2.linear()=rotation_cam2;
    //rotate
    frame_left_rectified.tf_cam_world=  tf_rectified_unrectified_cam1 * frame_left.tf_cam_world;
    frame_right_rectified.tf_cam_world=  tf_rectified_unrectified_cam2 * frame_right.tf_cam_world;

    // VLOG(1) << " rotation_cam1_mat " << rotation_cam1_mat << " eigen " << rotation_cam1;
    // VLOG(1) << " rotation_cam1_mat " << rotation_cam2_mat << " eigen " << rotation_cam2;







    //move the cam2_crop a bit to the right so that the maximum disparity we need to check is lower
    int offsety=0;
    if (offset_disparity!=0){
        cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, offset_disparity, 0, 1, offsety);
        if (!rgb_8u.empty()) cv::warpAffine(frame_right_rectified.rgb_8u, frame_right_rectified.rgb_8u, trans_mat, frame_right_rectified.rgb_8u.size());
        if (!rgb_32f.empty()) cv::warpAffine(frame_right_rectified.rgb_32f, frame_right_rectified.rgb_32f, trans_mat, frame_right_rectified.rgb_32f.size());
    }



    //get make them shared ptr
    std::shared_ptr frame_left_rectified_ptr=std::make_shared<Frame>(frame_left_rectified);
    std::shared_ptr frame_right_rectified_ptr=std::make_shared<Frame>(frame_right_rectified);
    frame_left_rectified_ptr->m_right_stereo_pair=frame_right_rectified_ptr;

    //get also the Q which is useful for getting from disparity to depth. Check stereoRectify from OpenCV
    Eigen::Matrix4d Q;
    cv::cv2eigen(Q_mat, Q);


    // std::tuple<Frame, Frame, float> ret_value= std::make_tuple(frame_left_rectified, frame_right_rectified, baseline);
    std::tuple<std::shared_ptr<Frame>, std::shared_ptr<Frame>, float, Eigen::Matrix4d> ret_value= std::make_tuple(frame_left_rectified_ptr, frame_right_rectified_ptr, baseline, Q);

    return ret_value;

}

Frame Frame::rotate_clockwise_90(){

    //check that the distorsion coefficiets are zero, therefore we are rotating a undistorted image. If we were to deal with a distorted image we whould need to modify also the coefficients probably which is a bit difficult.
    CHECK(distort_coeffs.isZero()) << "We are trying to rotate an image which has not been undistorted. This would require modifing also the dist_coeffs which is a bit involved and I'm to lazy to do it. Please undistort your image first.";
    CHECK(!is_shell) <<"This frame is a shell. You need to call load_images first";



    Frame rotated_frame(*this); //this should copy all the things like weight and height and do a shallow copy of the cv::Mats

    rotated_frame.clone_mats(); //since the operations are done in place, we clone the memory assigned for the mats so the two frames actually make a deep copy of the mats

    //rotate K
    // rotated_frame.K(1,1)=K(0,0); //fx and fy change place
    // rotated_frame.K(0,0)=K(1,1);
    // rotated_frame.K(0,2)=get_rgb_mat().cols-K(1,2); //cx is width-old_cy
    // rotated_frame.K(1,2)=K(0,2); //cy is the same as old_cx

    //undistort all images, we clone the input mat because the remap operates inplace and the src and destination mats are actually the same since we just did a shallow copy
    if(!rgb_8u.empty())  cv::rotate(rgb_8u, rotated_frame.rgb_8u, cv::ROTATE_90_CLOCKWISE);
    if(!rgb_32f.empty())  cv::rotate(rgb_32f, rotated_frame.rgb_32f, cv::ROTATE_90_CLOCKWISE);
    if(!gray_8u.empty())  cv::rotate(gray_8u, rotated_frame.gray_8u, cv::ROTATE_90_CLOCKWISE);
    if(!gray_32f.empty())  cv::rotate(gray_32f, rotated_frame.gray_32f, cv::ROTATE_90_CLOCKWISE);
    if(!grad_x_32f.empty())  cv::rotate(grad_x_32f, rotated_frame.grad_x_32f, cv::ROTATE_90_CLOCKWISE);
    if(!grad_y_32f.empty())  cv::rotate(grad_y_32f, rotated_frame.grad_y_32f, cv::ROTATE_90_CLOCKWISE);
    if(!gray_with_gradients.empty())  cv::rotate(gray_with_gradients, rotated_frame.gray_with_gradients, cv::ROTATE_90_CLOCKWISE);
    if(!thermal_16u.empty())  cv::rotate(thermal_16u, rotated_frame.thermal_16u, cv::ROTATE_90_CLOCKWISE);
    if(!thermal_32f.empty())  cv::rotate(thermal_32f, rotated_frame.thermal_32f, cv::ROTATE_90_CLOCKWISE);
    if(!thermal_vis_32f.empty())  cv::rotate(thermal_vis_32f, rotated_frame.thermal_vis_32f, cv::ROTATE_90_CLOCKWISE);
    if(!mask.empty())  cv::rotate(mask, rotated_frame.mask, cv::ROTATE_90_CLOCKWISE);
    if(!depth.empty())  cv::rotate(depth, rotated_frame.depth, cv::ROTATE_90_CLOCKWISE);

    //rotate K
    rotated_frame.K(1,1)=K(0,0); //fx and fy change place
    rotated_frame.K(0,0)=K(1,1);
    rotated_frame.K(0,2)=rotated_frame.get_rgb_mat().cols-K(1,2); //cx is width-old_cy
    rotated_frame.K(1,2)=K(0,2); //cy is the same as old_cx

    
    //rotate tf_cam_world;
    Eigen::Affine3f tf_rot;
    tf_rot.setIdentity();
    Eigen::Matrix3f r = (Eigen::AngleAxisf( radu::utils::degrees2radians(90), Eigen::Vector3f::UnitZ()) ).toRotationMatrix();
    tf_rot.linear()=r;
    rotated_frame.tf_cam_world=tf_rot*tf_cam_world;

    // Eigen::Affine3f tf_world_cam=tf_cam_world.inverse();
    // rotated_frame.tf_cam_world = (tf_world_cam*tf_rot).inverse();


    //set width and height
    rotated_frame.height=rotated_frame.get_rgb_mat().rows;
    rotated_frame.width=rotated_frame.get_rgb_mat().cols;


    return rotated_frame;

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

void Frame::from_camera(const std::shared_ptr<Camera>& cam, const int width, const int height, const bool flip_z_axis, const bool flip_y_axis){
    Eigen::Matrix4f P = cam->proj_matrix(width, height);
    this->K=opengl_proj_to_intrinsics(P, width, height);

    this->tf_cam_world=cam->view_matrix_affine();

    if (flip_z_axis){
        Eigen::Affine3f tf_world_cam=this->tf_cam_world.inverse();
        Eigen::Matrix3f cam_axes;
        cam_axes=tf_world_cam.linear();
        cam_axes.col(2)=-cam_axes.col(2);
        tf_world_cam.linear()= cam_axes;
        this->tf_cam_world= tf_world_cam.inverse();
    }

    if (flip_y_axis){
        Eigen::Affine3f tf_world_cam=this->tf_cam_world.inverse();
        Eigen::Matrix3f cam_axes;
        cam_axes=tf_world_cam.linear();
        cam_axes.col(1)=-cam_axes.col(1);
        tf_world_cam.linear()= cam_axes;
        this->tf_cam_world= tf_world_cam.inverse();
    }

    this->width=width;
    this->height=height;
}


cv::Mat Frame::get_rgb_mat(){
     //get the rgb img
    cv::Mat rgb_img;
    if (!rgb_8u.empty()){
        rgb_img=rgb_8u;
    }else if(!rgb_32f.empty()){
        rgb_img=rgb_32f;
    }

    return rgb_img;

}




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

std::shared_ptr<Frame> Frame::right_stereo_pair(){
    CHECK( has_right_stereo_pair()) << "We do not have a right stereo pair";
    return m_right_stereo_pair;
}

void Frame::set_right_stereo_pair(Frame& frame){
    m_right_stereo_pair=std::make_shared<Frame>(frame);
}

// template <typename T>
// void Frame::add_field(const std::string name, const T data){
//     // extra_field[name] = data;
// }




} //namespace easy_pbr
