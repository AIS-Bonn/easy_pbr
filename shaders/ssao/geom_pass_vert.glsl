#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
in vec3 position;
in vec3 normal;

//out
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec3 position_cam_coords_out; //position of the vertex in the camera coordinate system
layout(location = 2) out vec3 normal_cam_coords_out; //normal of the vertex in the camera coordinate system


//uniforms
uniform mat4 M;
uniform mat4 MV;
uniform mat4 MVP;

void main(){

   gl_Position = MVP*vec4(position, 1.0);

   normal_out=normalize(vec3(M*vec4(normal,0.0))); //normals are not affected by translation so the homogenous component is 0
   position_cam_coords_out= vec3(MV*(vec4(position, 1.0))); //from object to world and from world to view
   normal_cam_coords_out=normalize(vec3(MV*vec4(normal, 0.0)));
}
