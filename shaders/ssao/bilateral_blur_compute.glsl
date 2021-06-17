#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout (local_size_x = 16, local_size_y = 16) in;


// uniform float z_near;
// uniform float z_far;
uniform sampler2D depth_linear_tex;
uniform sampler2D ao_raw_tex;
layout(r32f) uniform writeonly image2D ao_blurred_img;
uniform float sigma_spacial;
uniform float sigma_depth;
uniform int ao_power;
uniform int pyr_lvl;


void main() {

    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    //for sampling we need it in range [0,1]
    ivec2 img_size=textureSize( ao_raw_tex, 0);
    // ivec2 img_size=textureSize( depth_linear_tex, 0);
    vec2 uv_coord=vec2(float(img_coords.x+0.5)/img_size.x, float(img_coords.y+0.5)/img_size.y);

    //sample the depth and convert
    // float depth_linear=texture(depth_linear_tex, uv_coord).x;
    float depth_linear=textureLod(depth_linear_tex, uv_coord, pyr_lvl).x;
    // float depth_linear=textureLod(depth_linear_tex, uv_coord, 0).x;
    float ao_raw=texture(ao_raw_tex, uv_coord).x;


    // https://github.com/tranvansang/bilateral-filter/blob/master/fshader.fragA
    float divisor_s = -1./(2.*sigma_spacial*sigma_spacial); //divisor in the exp function of the spacial component
    float divisor_l = -1./(2.*sigma_depth*sigma_depth);


    float c_total=ao_raw;
    float w_total=1.0;
    float half_size = sigma_spacial * 2;

    for(float x=-half_size; x<=half_size; x++){
        for(float y=-half_size; y<=half_size; y++){
            vec2 offset=vec2(x,y)/vec2(img_size);
            float offset_depth = texture(depth_linear_tex, uv_coord + offset).x;
            float offset_ao = texture(ao_raw_tex, uv_coord + offset).x;

            float dist_spacial = length(vec2(x,y));
            float dist_depth =offset_depth - depth_linear;

            float w_spacial = exp(divisor_s*float(dist_spacial*dist_spacial));
            float w_d = exp(divisor_l*float(dist_depth*dist_depth));
            float w = w_spacial*w_d;

            w_total+=w;
            c_total+=offset_ao*w;

        }
    }

    float final_ao=c_total/w_total;
    final_ao = smoothstep(0.0, 0.995, final_ao); //everything above 0.9 gets put to 1. We do this to avoid dampening the bright parts when we use pow
    final_ao=pow(final_ao, ao_power);
    // float final_ao=1.0;



    imageStore(ao_blurred_img, img_coords, vec4(final_ao, 0.0, 0.0, 1.0) );


}
