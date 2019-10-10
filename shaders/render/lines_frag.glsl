#version 430 core
layout(location = 0) out vec4 out_color;

uniform vec3 line_color;

void main(){

    out_color = vec4(line_color, 1.0);
}
