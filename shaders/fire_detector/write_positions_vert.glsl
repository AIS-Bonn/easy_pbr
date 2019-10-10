#version 430 core

//in
in vec3 position;
in vec2 uv;

//out
layout(location = 0) out vec3 position_out;


void main(){

    //uv are in range [0,1] but to put them in screen coordinates and render them we need them in [-1,1]
    vec2 uv_scaled = uv*2 - 1.0;
    gl_Position = vec4(uv_scaled,0,1);

    position_out=position;
}