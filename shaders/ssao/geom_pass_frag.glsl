#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//int
layout(location = 0) in vec3 normal_in;
layout(location = 1) in vec3 position_cam_coords_in; //position of the vertex in the camera coordinate system
layout(location = 2) in vec3 normal_cam_coords_in; //normal of the vertex in the camera coordinate system

//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 color_out;
layout(location = 2) out vec4 normal_out;
// layout(location = 3) out vec4 normal_world_out;

void main(){

    color_out = vec4(1.0, 0.0, 0.0, 1.0);
    normal_out = vec4(normal_cam_coords_in, 1.0);
    position_out = vec4(position_cam_coords_in, 1.0);
    // position_out = vec4(position_cam_coords_in.z, 0.0, 0.0, 1.0);
    // normal_world_out = vec4(normal_in, 1.0);
}
