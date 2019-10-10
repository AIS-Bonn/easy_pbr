#version 430 core

// #extension GL_ARB_shading_language_include : require
// #include "color_utils.glsl"

//in
layout(location = 0) in vec3 position_in;

//out 
layout(location = 0) out vec4 position_out;


void main(){

    position_out=vec4(position_in, 1.0);
    // position_out_frag=vec4(0.0, 1.0, 1.0, 1.0);
}