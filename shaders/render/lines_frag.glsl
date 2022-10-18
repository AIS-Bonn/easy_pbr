#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
flat layout(location = 0) in vec3 color_per_vertex_in;
flat layout(location = 1) in vec3 startPos;
layout(location = 2) in vec3 vertPos;


//out
layout(location = 0) out vec4 out_color;

//uniform
uniform float opacity;
//for dashed lines https://stackoverflow.com/a/54543267
uniform vec2  u_viewport_size;
uniform float u_dashSize;
uniform float u_gapSize;
uniform bool is_line_dashed;

void main(){
    
    //for dashed line
    if (is_line_dashed){
        vec2  dir  = (vertPos.xy-startPos.xy) * u_viewport_size/2.0;
        float dist = length(dir);

        if (fract(dist / (u_dashSize + u_gapSize)) > u_dashSize/(u_dashSize + u_gapSize))
            discard;
    }

    out_color = vec4(color_per_vertex_in, opacity);
}
