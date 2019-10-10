#version 430

layout (local_size_x = 16, local_size_y = 16) in;

uniform sampler2D position_cam_coords_tex;
uniform sampler2D normal_cam_coords_tex;
uniform sampler2D normal_tex;
// layout(r32f) uniform writeonly image2D ao_img;
layout(rgba16f) uniform writeonly image2D final_img;


// //uniform
// uniform int color_type;
// uniform vec3 solid_color;
// uniform vec3 ambient_color;
// uniform vec3 specular_color;
// uniform float ambient_color_power;
// uniform float shininess;
// uniform sampler2D ao_img;
// uniform float ssao_subsample_factor;
// uniform vec2 viewport_size;

// struct Light {
//    vec4 position; // the position is in xyz,  where the w component will be 1 for omilights and 0 for directional lights as per  https://www.tomdalling.com/blog/modern-opengl/08-even-more-lighting-directional-lights-spotlights-multiple-lights/
//    vec3 color; //the color of the light
//    float power; // how much strenght does the light have
// };

// vec3 apply_light(Light light, vec3 diffuse_color){
//     // return vec3(0);


//     //weight the diffuse_color contrubtion by the dot product between light_vector and normal
//     vec3 dir_to_light;
//     if(light.position.w == 1.0 ){ //its an omni light
//         vec3 vec_pos_to_light = vec3(light.position) - position_cam_coords_in;
//         dir_to_light = normalize (vec_pos_to_light);
//     }else{ //its a directional lgith
//         dir_to_light=normalize(vec3(light.position));
//     }
//     float dot_prod = dot (dir_to_light, normal_cam_coords_in );
//     float clamped_dot_prod = max (dot_prod, 0.0);
//     vec3  diffuse_lit = diffuse_color * clamped_dot_prod * light.color* light.power ;    // Diffuse intensity

//     vec3 reflection_eye = reflect (-dir_to_light, normalize(normal_cam_coords_in)); //reflect the light *coming in* about the normal vector
//     vec3 surface_to_viewer_eye = normalize (-position_cam_coords_in);
//     float dot_prod_specular = dot (reflection_eye, surface_to_viewer_eye);
//     dot_prod_specular = float(abs(dot_prod)==dot_prod) * max (dot_prod_specular, 0.0);
//     float specular_factor = pow (dot_prod_specular, shininess);
//     vec3 specular_lit = specular_color * specular_factor * light.color * light.power;    // specular intensity

//     return diffuse_lit+specular_lit;
//     // return diffuse_lit;
// }


void main() {
    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    // //for sampling we need it in range [0,1]
    ivec2 img_size=textureSize( position_cam_coords_tex, 0);
    vec2 uv_coord=vec2(float(img_coords.x)/img_size.x, float(img_coords.y)/img_size.y);

    vec3 pos_cam_coords=texelFetch(position_cam_coords_tex, img_coords,0).xyz;
    vec3 normal_cam_coords=texelFetch(normal_cam_coords_tex, img_coords,0).xyz;







   

    imageStore(final_img, img_coords, vec4(normal_cam_coords, 1.0) );
    // imageStore(final_img, img_coords, uvec4(255, 255, 255, 255) );

 

}