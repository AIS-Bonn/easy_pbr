#version 430 core

//in
layout(location = 0) in vec3 color_per_vertex_in;

//out 
layout(location = 0) out vec4 out_color;

//uniforms
// uniform bool enable_point_color; // whether to use point color or color per vertex

void main(){

    out_color = vec4(color_per_vertex_in, 1.0);
}
