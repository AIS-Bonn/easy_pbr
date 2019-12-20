#version 430 core

//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D composed_tex;
uniform sampler2D bloom_tex;

uniform int bloom_mip_map_lvl;
uniform float exposure;


//trying ACES as explained in https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl because as explained here: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/ it should be better 
mat3 aces_input = mat3(
   0.59719, 0.35458, 0.04823, // first column (not row!)
   0.07600, 0.90834, 0.01566, // second column
   0.02840, 0.13383, 0.83777  // third column
);
mat3 aces_output = mat3(
   1.60475, -0.53108, -0.07367, // first column (not row!)
   -0.10208, 1.10813, -0.00605, // second column
   -0.00327, -0.07276, 1.07602  // third column
);
vec3 RRTAndODTFit(vec3 v){
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

void main(){


    // vec3 result = textureLod(img, uv_in, mip_map_lvl).rgb * weight[0]; // current fragment's contribution
    vec3 color = texture(composed_tex, uv_in).rgb;
    vec3 bloom = textureLod(bloom_tex, uv_in, bloom_mip_map_lvl).rgb;
    color+=bloom;

    //tonemap and gamma correct 
    color*=exposure;
    color = transpose(aces_input)*color;
    color = RRTAndODTFit(color);
    color = transpose(aces_output)*color;
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    out_color=vec4(color,1.0);

}
