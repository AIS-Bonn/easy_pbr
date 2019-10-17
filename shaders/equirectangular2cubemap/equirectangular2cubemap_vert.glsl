#version 430 core

//in
in vec3 position;

//out
layout(location = 0) out vec3 view_ray_out;

uniform mat3 V_inv_rot;
uniform mat4 P_inv;

void main(){

    // position_out=V_rot*position;
    gl_Position = vec4(position, 1.0); //just pases the positions as they are so we have a mesh covering the whole screen which we can rasterize against

    //attempt 1
    // vec4 view_ray = vec4(position.xyz, 1.0);
    // view_ray = P_inv * view_ray;
    // view_ray_out = inverse(V_rot)*view_ray.xyz;

    //attempt 2 (the previous one works but the rotation is somehow inverted)
    vec4 view_ray = vec4(position.xyz, 1.0);
    view_ray =  P_inv * view_ray;
    view_ray_out = V_inv_rot * view_ray.xyz;
}
