#version 430 core
#extension GL_ARB_explicit_attrib_location : require


//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 color_output;




void main(){


    vec4 result = vec4(1.0, 1.0, 0.0, 1.0);


    color_output=result;


}
