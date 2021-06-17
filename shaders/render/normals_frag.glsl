#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



//in
// layout(location = 0) in vec3 position_in;
// layout(location = 1) in vec3 normal_in;


//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
layout(location = 0) out vec4 out_color;

// //uniform
uniform vec3 line_color;




void main(){

    out_color= vec4(line_color, 1.0);

}
