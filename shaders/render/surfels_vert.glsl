#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
in vec3 position;
in vec3 normal;
in vec3 tangent_u;
in float lenght_v;
in vec3 color_per_vertex;
in int label_pred_per_vertex;
in int label_gt_per_vertex;

//out
layout(location = 0) out vec3 v_pos_out; //position in world coords
layout(location = 1) out vec3 v_normal_out;
layout(location = 2) out vec3 tangent_u_out;
layout(location = 3) out float lenght_v_out;
layout(location = 4) out vec3 color_per_vertex_out;


//uniforms
uniform mat4 M; //model matrix which moves from an object centric frame into the world frame. If the mesh is already in the world frame, then this will be an identity
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;
uniform int color_type;
uniform vec3 solid_color;
#define MAX_NR_CLASSES 255
uniform vec3 color_scheme[MAX_NR_CLASSES];

void main(){

    v_pos_out=position;
    v_normal_out=normal;
    tangent_u_out=tangent_u;
    lenght_v_out=lenght_v;
    // color_per_vertex_out=color_per_vertex;

    if(color_type==0){ //solid
        color_per_vertex_out=solid_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else if(color_type==2){ //texture WILL BE DONE IN THE FRAGMENT SHADER
        // color_per_vertex_out=vec3(0);
    }else if(color_type==3){ //semantic pred
        color_per_vertex_out=color_scheme[label_pred_per_vertex];
    }else if(color_type==4){ //semantic gt
        color_per_vertex_out=color_scheme[label_gt_per_vertex];
    }else if(color_type==5){ //normal vector
        color_per_vertex_out=(normal+1.0)/2.0;
    }
    // }else if(color_type==6){ //SSAO CANNOT BE DONE HERE AS IT CAN ONLY BE DONE BY THE COMPOSE SHADER
        // color_per_vertex_out=vec3(0);
    // }

}
