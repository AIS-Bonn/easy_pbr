#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
flat layout(location = 0) in vec3 color_per_vertex_in;

//out
layout(location = 0) out vec4 out_color;

//uniform

void main(){

    out_color = vec4(color_per_vertex_in, 1.0);
}
