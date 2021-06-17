#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
layout(location=0) in vec2 uv_in;

//out
layout(location = 0) out float depth_linear_out;

//uniforms
uniform sampler2D depth_tex;
uniform float projection_a; //for calculating position from depth according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
uniform float projection_b;
uniform int pyr_lvl;


float linear_depth(float depth_sample){
    // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    float linearDepth = projection_b / (depth_sample - projection_a);
    return linearDepth;
}


void main() {

    //sample the depth and convert
    float depth_raw=textureLod(depth_tex, uv_in, pyr_lvl).x;
    float depth_linear;
    if(depth_raw==1.0){
        depth_linear=1.0; //we consider a 1.0 as infinite depth, so something that is not covered by the mesh
    }else{
        depth_linear= linear_depth(depth_raw);
    }


    depth_linear_out=depth_linear;

}
