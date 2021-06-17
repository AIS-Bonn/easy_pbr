#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

//uniforms
uniform mat4 P_inv;
uniform mat3 V_inv_rot;

//out
layout (location = 1) out vec2 uv_out;
layout (location = 2) out vec3 view_ray_out;
layout (location = 3) out vec3 world_view_ray_out;

void main(){

   gl_Position = vec4(position, 1.0);
   uv_out= uv;


    //attempt 2 based on https://community.khronos.org/t/compute-fragments-ray-direction-worlds-coordinates/68424/2
    vec4 view_ray = vec4(position.xy, 0.0, 1.0);
    view_ray = P_inv * view_ray;
    view_ray_out = view_ray.xyz;

    //view ray that rotates when the camera rotates. This view rays are in the world coordinates and not in the cam_coords like the view_ray_out
    world_view_ray_out = V_inv_rot * view_ray.xyz;

//    view_ray_out = vec3(position.xy / position.z, 1.0f);
//    view_ray_out = (P_inv*vec4(position.xyz, 1.0f)).xyz;
//    view_ray_out = vec3(position.xy+vec2(0.5, 0.5), 1.0f);
}
