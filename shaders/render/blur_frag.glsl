#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 blurred_output;

uniform sampler2D img;

uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
uniform int mip_map_lvl;

void main(){

    // blurred_output=vec4(1.0);

    //https://learnopengl.com/Advanced-Lighting/Bloom
    ivec2 tex_size= textureSize(img, mip_map_lvl);
    vec2 tex_offset = 1.0 / tex_size; // gets size of single texel
    vec4 result = textureLod(img, uv_in, mip_map_lvl) * weight[0]; // current fragment's contribution
    if(horizontal){
        for(int i = 1; i < 5; ++i){
            // result += textureLod(img, uv_in + vec2(tex_offset.x * i, 0.0), mip_map_lvl) * weight[i];
            // result += textureLod(img, uv_in - vec2(tex_offset.x * i, 0.0), mip_map_lvl) * weight[i];
            result += texelFetch(img, ivec2(uv_in*tex_size + ivec2(i, 0)), mip_map_lvl) * weight[i];
            result += texelFetch(img, ivec2(uv_in*tex_size - ivec2(i, 0)), mip_map_lvl) * weight[i];
        }
    }
    else{
        for(int i = 1; i < 5; ++i){
            // result += textureLod(img, uv_in + vec2(0.0, tex_offset.y * i), mip_map_lvl) * weight[i];
            // result += textureLod(img, uv_in - vec2(0.0, tex_offset.y * i), mip_map_lvl) * weight[i];
            result += texelFetch(img, ivec2(uv_in*tex_size + ivec2(0, i)), mip_map_lvl) * weight[i];
            result += texelFetch(img, ivec2(uv_in*tex_size - ivec2(0, i)), mip_map_lvl) * weight[i];
        }
    }

    blurred_output=result;
    // if(dot(uv_in,uv_in)<1.0){
        // blurred_output=vec4(1.0);
    // }
    // blurred_output=vec4(1.0);

}
