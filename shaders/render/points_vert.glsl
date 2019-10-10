#version 430 core
//in
in vec3 position;
in vec3 normal;
in vec3 color_per_vertex;
in int label_pred_per_vertex;
in int label_gt_per_vertex;

//out
layout(location = 0) out vec3 color_per_vertex_out;

//uniforms
uniform mat4 MVP;
uniform vec3 point_color;
uniform int color_type;

#define MAX_NR_CLASSES 255
uniform vec3 color_scheme[MAX_NR_CLASSES];

void main(){

    gl_Position = MVP*vec4(position, 1.0);

    if(color_type==0){ //solid
        color_per_vertex_out=point_color;
    }else if(color_type==1){ //per vert color
       color_per_vertex_out=color_per_vertex;
    }else if(color_type==2){ //texture NOT APLICABLE HERE
        color_per_vertex_out=vec3(0);
    }else if(color_type==3){ //semantic pred
        color_per_vertex_out=color_scheme[label_pred_per_vertex];
    }else if(color_type==4){ //semantic gt
        color_per_vertex_out=color_scheme[label_gt_per_vertex];
    }else if(color_type==5){ //normal vector
        color_per_vertex_out=(normal+1.0)/2.0;
    }else if(color_type==6){ //SSAO NOT APPLICABLE HERE
        color_per_vertex_out=vec3(0);
    }

}

