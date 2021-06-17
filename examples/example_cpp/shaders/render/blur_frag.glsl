#version 430 core
#extension GL_ARB_explicit_attrib_location : require

//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 blurred_output;

//uniform
uniform sampler2D img;
uniform sampler2D depth_tex;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
uniform int mip_map_lvl;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax); //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}


void main(){

    ivec2 tex_size= textureSize(img, mip_map_lvl);
    vec2 tex_offset = 1.0 / tex_size; // gets size of single texel
    vec4 result = textureLod(img, uv_in, mip_map_lvl) * weight[0]; // current fragment's contribution

    float spacing=3.0;


    float depth = texture(depth_tex, uv_in).x;
    // spacing=map(depth, 0.93, 0.999, 0.0, 10.0);
    if(depth==1.0){
        blurred_output= textureLod(img, uv_in, 0);
        return;
    }

    if(horizontal){
        for(int i = 1; i < 5; ++i){
            result += textureLod(img, uv_in + vec2(tex_offset.x * i*spacing, 0.0), mip_map_lvl) * weight[i];
            result += textureLod(img, uv_in - vec2(tex_offset.x * i*spacing, 0.0), mip_map_lvl) * weight[i];
        }
    }
    else{
        for(int i = 1; i < 5; ++i){
            result += textureLod(img, uv_in + vec2(0.0, tex_offset.y * i*spacing), mip_map_lvl) * weight[i];
            result += textureLod(img, uv_in - vec2(0.0, tex_offset.y * i*spacing), mip_map_lvl) * weight[i];
        }
    }

    blurred_output=result;

}
