#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


layout (location = 0) in vec3 position;

//uniforms
uniform mat4 P_inv;
uniform mat3 V_inv_rot;

//out
layout (location = 0) out vec3 world_view_ray_out;

void main(){

   gl_Position = vec4(position, 1.0);


    //attempt 2 based on https://community.khronos.org/t/compute-fragments-ray-direction-worlds-coordinates/68424/2
    vec4 view_ray = vec4(position.xy, 0.0, 1.0);
    view_ray = P_inv * view_ray;

    //view ray that rotates when the camera rotates. This view rays are in the world coordinates and not in the cam_coords like the view_ray_out
    world_view_ray_out = V_inv_rot * view_ray.xyz;

}
