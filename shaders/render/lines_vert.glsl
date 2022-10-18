#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
in vec3 position;
in vec3 color_per_vertex;


//out
flat layout(location = 0) out vec3 color_per_vertex_out;
//for dashed lines https://stackoverflow.com/a/54543267
flat layout(location = 1) out vec3 startPos;
layout(location = 2) out vec3 vertPos;



//uniform
uniform int color_type;
uniform mat4 MVP;
uniform vec3 line_color;

void main(){

    vec4 pos = MVP*vec4(position, 1.0);
    gl_Position = pos;

    //for dashed lines https://stackoverflow.com/a/54543267
    vertPos     = pos.xyz / pos.w;
    startPos    = vertPos;


    if(color_type==0){ //solid
        color_per_vertex_out=line_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else{
        //it's none of those color types so it might be soemthing weird like texture rendering. this can happen when we render a frustum and we want to render a part of it with texture. In this case default the line to solid rendering
        color_per_vertex_out=line_color;
    }
}
