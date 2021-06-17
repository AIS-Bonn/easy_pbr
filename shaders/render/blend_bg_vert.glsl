#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

//out
layout (location = 1) out vec2 uv_out;

void main(){

   gl_Position = vec4(position, 1.0);
   uv_out= uv;


}
