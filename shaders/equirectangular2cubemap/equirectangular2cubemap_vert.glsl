#version 430 core

//in
in vec3 position_ndc;

//out
layout(location = 0) out vec3 position_ndc_out;

uniform mat4 MVP;

void main(){

    position_ndc_out=position_ndc;
    gl_Position = MVP*vec4(position_ndc, 1.0);
}
