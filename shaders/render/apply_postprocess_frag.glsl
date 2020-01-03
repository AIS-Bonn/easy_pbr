#version 430 core

//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D composed_tex;
uniform sampler2D bloom_tex;
uniform sampler2D depth_tex;

uniform bool enable_bloom;
uniform int bloom_mip_map_lvl;
uniform float exposure;
uniform vec3 background_color;
uniform bool show_background_img;
uniform bool show_environment_map;


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

    float depth=texture(depth_tex, uv_in).x;

    vec3 color;
    if(depth==1.0){
        // //there is no mesh or anything covering this pixel, we either read the backgrpund or just set the pixel to the background color
         if (show_background_img || show_environment_map){
            color = texture(composed_tex, uv_in).rgb;
         }else{
            //  color=background_color;
            discard;
         }
    }else{
        //pixel is covered by mesh so we read the color it has
        color = texture(composed_tex, uv_in).rgb;
    }

    // vec3 result = textureLod(img, uv_in, mip_map_lvl).rgb * weight[0]; // current fragment's contribution
    vec3 color_posprocessed=color;
    // vec3 color_posprocessed=vec3(0.0);
    if (enable_bloom){

        //add the bloom from all the blurred textures
        float bloom_global_weight=0.5;
        for (int i=0; i<bloom_mip_map_lvl; i++){
            vec4 bloom = textureLod(bloom_tex, uv_in, i);
            float bloom_weight=bloom.w;
            color_posprocessed+=bloom.rgb*bloom_weight*bloom_global_weight;
        }
        // vec4 bloom = textureLod(bloom_tex, uv_in, bloom_mip_map_lvl);
        // float bloom_weight=bloom.w;
        // color_posprocessed=color+bloom.rgb*bloom_weight;

        //DEBUG show just one bloom tex
        // vec4 bloom = textureLod(bloom_tex, uv_in, 5);
        // color_posprocessed=bloom.rgb;
    }

    // if(color!=background_color){
    // if(depth==1.0 && !show_background_img && !show_environment_map  ){
        //tonemap and gamma correct 
        color_posprocessed*=exposure;
        color_posprocessed = transpose(aces_input)*color_posprocessed;
        color_posprocessed = RRTAndODTFit(color_posprocessed);
        color_posprocessed = transpose(aces_output)*color_posprocessed;
        // gamma correct
        color_posprocessed = pow(color_posprocessed, vec3(1.0/2.2)); 
    // }


    out_color=vec4(color_posprocessed,1.0);

}
