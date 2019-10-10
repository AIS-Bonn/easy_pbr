#version 430 core

//in
in vec3 position;
in vec2 uv;
in vec3 normal;

//out
layout(location = 0) out vec3 position_cam_coords_out;
layout(location = 1) out vec2 uv_out;
layout(location = 2) out vec4 shadow_coord_out;
layout(location = 3) out float dist_to_cam_linear;
layout(location = 4) out vec3 normal_cam_coords_out;


//uniform
uniform mat4 light_bias_mvp;
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen

void main(){
    // gl_Position =  MVP * vec4(position,1.0);

    //uv are in range [0,1] but to put them in screen coordinates and render them we need them in [-1,1]
    vec2 uv_scaled = uv*2 - 1.0;
    gl_Position = vec4(uv_scaled,0,1);

    // position_out=position;
    vec3 position_cam_coords = vec3(MV*(vec4(position, 1.0))); //from object to world and from world to view
    dist_to_cam_linear = length(position_cam_coords);
    position_cam_coords_out=position_cam_coords;
    normal_cam_coords_out=normalize(vec3(MV*vec4(normal, 0.0)));
    uv_out=uv;
    shadow_coord_out=light_bias_mvp * vec4(position,1.0);
}