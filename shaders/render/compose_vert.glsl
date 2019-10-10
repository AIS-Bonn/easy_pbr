#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

//uniforms 
uniform mat4 P_inv;

//out 
layout (location = 1) out vec2 uv_out;
layout (location = 2) out vec3 view_ray_out;

void main(){

   gl_Position = vec4(position, 1.0);
   uv_out= uv;

   //the position we get here are already project, so we unproject them 
   vec4 position_cam_coords = P_inv*vec4(position, 1.0);
    view_ray_out = vec3(position_cam_coords.xy / position_cam_coords.z, 1.0f);


    //attempt 2 based on https://community.khronos.org/t/compute-fragments-ray-direction-worlds-coordinates/68424/2
    vec4 view_ray = vec4(position.xy, 0.0, 1.0);
    view_ray = P_inv * view_ray;
    view_ray_out = view_ray.xyz;

//    view_ray_out = vec3(position.xy / position.z, 1.0f);
//    view_ray_out = (P_inv*vec4(position.xyz, 1.0f)).xyz;
//    view_ray_out = vec3(position.xy+vec2(0.5, 0.5), 1.0f);
}