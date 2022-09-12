#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



//in
in vec3 position;
in vec3 normal;
in vec3 color_per_vertex;
in float intensity_per_vertex;
in vec2 uv;
in int label_pred_per_vertex;
in int label_gt_per_vertex;

//out
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec3 position_cam_coords_out; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
// layout(location = 2) out vec3 normal_cam_coords_out; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 2) out vec3 color_per_vertex_out;
layout(location = 3) out vec2 uv_out;

//uniforms
uniform mat4 M; //model matrix which moves from an object centric frame into the world frame. If the mesh is already in the world frame, then this will be an identity
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;
uniform int color_type;
uniform vec3 point_color;
#define MAX_NR_CLASSES 255
uniform vec3 color_scheme[MAX_NR_CLASSES];
uniform vec3 color_scheme_height[256];
uniform float min_y;
uniform float max_y;
uniform bool has_normals=false; //if we have normals this will get set to true
uniform float point_size;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax);  //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}

vec3 colorize_height(float x){
    float x_clamped=clamp(x,0.0, 1.0);
    x_clamped*=255;
    int x_int = int(x_clamped);
    return color_scheme_height[x_int];
}

void main(){

    // bool enable_min_max_cap=true;
    // if (position.y> max_y || position.y<min_y){
        // gl_Position = MVP*vec4(vec3(0.0), 1.0);
        // color_per_vertex_out=vec3(0.0);
        // log_depth_out=0.0;
        // normal_cam_coords_out=vec3(0);
        // return;
    // }



    gl_Position = MVP*vec4(position, 1.0);

    // if(position.y>3){

    //     gl_Position = vec4(0);
    // }else{

    //     gl_Position = MVP*vec4(position, 1.0);
    // }


   // https://github.com/potree/potree/blob/develop/src/materials/shaders/pointcloud.fs
   //output also the vlog depth for eye dome lighing
//    vec4 mv_pos = MV * vec4( position, 1.0 );
//    log_depth_out = log2(-mv_pos.z);


//TODO normals also have to be rotated by the model matrix (at the moment it's only identity so its fine)
    position_cam_coords_out= vec3(MV*(vec4(position, 1.0))); //from object to world and from world to view
    if(has_normals){
        normal_out=normalize(vec3(M*vec4(normal,0.0))); //normals are not affected by translation so the homogenous component is 0
    }else{
        normal_out=vec3(0,0,0);
    }

//    color_per_vertex_out=color_per_vertex;
    uv_out=uv;

    if(color_type==0){ //solid
        color_per_vertex_out=point_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else if(color_type==2){ //texture WILL BE DONE IN THE FRAGMENT SHADER
        // color_per_vertex_out=vec3(0);
    }else if(color_type==3){ //semantic pred
        color_per_vertex_out=color_scheme[label_pred_per_vertex];
    }else if(color_type==4){ //semantic gt
        color_per_vertex_out=color_scheme[label_gt_per_vertex];
    }else if(color_type==5){ //normal vector
        color_per_vertex_out=(normal_out+1.0)/2.0;
    // }else if(color_type==6){ //SSAO CANNOT BE DONE HERE AS IT CAN ONLY BE DONE BY THE COMPOSE SHADER
        // color_per_vertex_out=vec3(0);
    }else if(color_type==6){ //height
        float cur_y=position.y;
        float height_normalized=map(cur_y, min_y, max_y, 0.0, 1.0);
        color_per_vertex_out=colorize_height(height_normalized);
    }else if(color_type==7){ //intensity
       color_per_vertex_out=vec3(intensity_per_vertex);
    }else if(color_type==8){ //uv
        color_per_vertex_out=vec3(uv,0.0);
    }


    gl_PointSize = point_size;
    // gl_PointSize = map(-position_cam_coords_out.z, 1.0, 5.0, 1.0, 20); //change the point size depending on the distance in Z in the camera coordinate frame
}
