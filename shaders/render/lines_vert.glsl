#version 430 core

//in
in vec3 position;
in vec3 color_per_vertex;


//out
flat layout(location = 0) out vec3 color_per_vertex_out;

//uniform 
uniform int color_type;
uniform mat4 MVP;
uniform vec3 line_color;

void main(){

   gl_Position = MVP*vec4(position, 1.0);


    if(color_type==0){ //solid
        color_per_vertex_out=line_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else{
        //it's none of those color types so it might be soemthing weird like texture rendering. this can happen when we render a frustum and we want to render a part of it with texture. In this case default the line to solid rendering
        color_per_vertex_out=line_color;
    }
}
