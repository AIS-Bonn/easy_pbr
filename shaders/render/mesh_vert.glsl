#version 430 core

//in
in vec3 position;
in vec3 normal;
in vec3 color_per_vertex;
in vec2 uv;
in int label_pred_per_vertex;
in int label_gt_per_vertex;

//out
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec3 position_cam_coords_out; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
layout(location = 2) out vec3 normal_cam_coords_out; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 3) out vec3 color_per_vertex_out;
layout(location = 4) out vec2 uv_out;
layout(location = 5) out vec3 position_world_out; //useful for some hacks like when when we want to discard all fragments above a certain height

//uniforms
uniform mat4 M; //model matrix which moves from an object centric frame into the world frame. If the mesh is already in the world frame, then this will be an identity
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;
uniform int color_type;
uniform vec3 solid_color;
#define MAX_NR_CLASSES 255
uniform vec3 color_scheme[MAX_NR_CLASSES];

void main(){


   gl_Position = MVP*vec4(position, 1.0);


   //TODO normals also have to be rotated by the model matrix (at the moment it's only identity so its fine)
   normal_out=normalize(vec3(M*vec4(normal,0.0))); //normals are not affected by translation so the homogenous component is 0
   position_cam_coords_out= vec3(MV*(vec4(position, 1.0))); //from object to world and from world to view
   normal_cam_coords_out=normalize(vec3(MV*vec4(normal, 0.0)));

//    color_per_vertex_out=color_per_vertex;
   uv_out=uv;

   position_world_out=position;

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
    }else if(color_type==6){ //SSAO CANNOT BE DONE HERE AS IT CAN ONLY BE DONE BY THE COMPOSE SHADER
        // color_per_vertex_out=vec3(0);
    }
}
