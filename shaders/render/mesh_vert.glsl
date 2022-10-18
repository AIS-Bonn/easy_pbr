#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
in vec3 position;
in vec3 normal;
in vec3 tangent; //T vector fom the TBN. We have already the N and we reconstruct the B with a cross product
in vec3 color_per_vertex;
in vec2 uv;
in int label_pred_per_vertex;
in int label_gt_per_vertex;
in float intensity_per_vertex;
in float metalness;
in float roughness;

//out
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec3 position_cam_coords_out; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
// layout(location = 2) out vec3 normal_cam_coords_out; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 2) out vec3 precomputed_ao_out;
layout(location = 3) out vec3 color_per_vertex_out;
layout(location = 4) out vec2 uv_out;
layout(location = 5) out vec3 position_world_out; //useful for some hacks like when when we want to discard all fragments above a certain height
layout(location = 6) out mat3 TBN_out;


//uniforms
uniform mat4 M; //model matrix which moves from an object centric frame into the world frame. If the mesh is already in the world frame, then this will be an identity
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;
uniform int color_type;
uniform vec3 solid_color;
#define MAX_NR_CLASSES 255
uniform vec3 color_scheme[MAX_NR_CLASSES];
uniform vec3 color_scheme_height[256];
uniform float min_y;
uniform float max_y;

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


   gl_Position = MVP*vec4(position, 1.0);

   //tbn matrix
   vec3 bitangent = cross(normal, tangent);  //calculate the bitgent in the object coordinate system. The tangent and normal are also in the object coordinate system

   //get the tbn vectors from the model to the world coordinate system
   vec3 T = normalize(vec3(M * vec4(tangent,   0.0)));
   vec3 B = normalize(vec3(M * vec4(bitangent, 0.0)));
   vec3 N = normalize(vec3(M * vec4(normal,    0.0)));
   mat3 TBN = mat3(T, B, N);
   TBN_out=TBN;


   //TODO normals also have to be rotated by the model matrix (at the moment it's only identity so its fine)
   normal_out=normalize(vec3(M*vec4(normal,0.0))); //normals are not affected by translation so the homogenous component is 0
   position_cam_coords_out= vec3(MV*(vec4(position, 1.0))); //from object to world and from world to view
//    normal_cam_coords_out=normalize(vec3(MV*vec4(normal, 0.0)));
//    normal_cam_coords_out=normalize(vec3(MV*vec4(normal, 0.0)));

    precomputed_ao_out=color_per_vertex;

//    color_per_vertex_out=color_per_vertex;
   uv_out=uv;

   position_world_out=position;

    if(color_type==0){ //solid
        color_per_vertex_out=solid_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else if(color_type==2){ //texture WILL BE DONE IN THE FRAGMENT SHADER
        // color_per_vertex_out=vec3(0);
    }else if(color_type==3){ //semantic pred
        color_per_vertex_out=color_scheme[label_pred_per_vertex];
    }else if(color_type==4){ //semantic gt
        color_per_vertex_out=color_scheme[label_gt_per_vertex];
    // }else if(color_type==5){ //normal vector //NORMAL WILL BE OUTPUTTED FROM FRAGMENT SHADER BECAUSE sometime we might want to do normal mapping and only the framgne thas acces to that
        // color_per_vertex_out=(normal_out+1.0)/2.0;
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
}
