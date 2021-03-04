#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) out vec4 out_color;
void main(){

    out_color = vec4(0.0, 0.0, 0.0, 1.0);
}
