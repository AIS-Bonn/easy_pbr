#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D composed_tex;
uniform sampler2D bloom_tex;
uniform sampler2D depth_tex;
uniform sampler2D normal_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D metalness_and_roughness_tex;
uniform sampler2D ao_tex;

uniform bool enable_bloom;
uniform int bloom_start_mip_map_lvl;
uniform int bloom_max_mip_map_lvl;
uniform float exposure;
// uniform vec3 background_color;
uniform bool show_background_img;
uniform bool show_environment_map;
uniform bool show_prefiltered_environment_map;
uniform bool enable_multichannel_view;
uniform vec2 size_final_image;
uniform float multichannel_interline_separation;
uniform float multichannel_line_width;
uniform float multichannel_line_angle;
uniform float multichannel_start_x;
uniform int tonemap_type;
uniform bool using_fat_gbuffer;
uniform float hue_shift;
uniform float saturation_shift;
uniform float value_shift;


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

//from google filament renderer https://github.com/google/filament/blob/master/shaders/src/tone_mapping.fs
float luminance(const vec3 linear) {
    return dot(linear, vec3(0.2126, 0.7152, 0.0722));
}
vec3 Tonemap_Linear(const vec3 x) {
    return x;
}

vec3 Tonemap_Reinhard(const vec3 x) {
    // Reinhard et al. 2002, "Photographic Tone Reproduction for Digital Images", Eq. 3
    return x / (1.0 + luminance(x));
}

vec3 Tonemap_Unreal(const vec3 x) {
    // Unreal, Documentation: "Color Grading"
    // Adapted to be close to Tonemap_ACES, with similar range
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    return x / (x + 0.155) * 1.019;
}

vec3 Tonemap_FilmicALU(const vec3 x) {
    // Hable 2010, "Filmic Tonemapping Operators"
    // Based on Duiker's curve, optimized by Hejl and Burgess-Dawson
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    vec3 c = max(vec3(0.0), x - 0.004);
    return (c * (c * 6.2 + 0.5)) / (c * (c * 6.2 + 1.7) + 0.06);
}

//decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
// vec3 decode_normal(vec2 normal){
//     //comment this out because the if condition actually costs us 1ms per frame. And it's not relevant usually as for point cloud which dont have normals we will use only EDL
//     // if(!has_normals ){ //if we got a normal that is zero like the one we would get from a point cloud without normals, then we just output a zero to indicate no lighting needs to be done
//         // return vec3(0);
//     // }
//     vec3 normal_decoded;
//     normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
//     normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
//     return normal_decoded;
// }

//encode as xyz https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 decode_normal(vec3 normal){
    if (using_fat_gbuffer){
        return normalize(normal);
    }else{
        return normalize(normal * 2.0 - 1.0);
    }
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax); //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}

//smoothstep but with 1st and 2nd derivatives that go more smoothly to zero
float smootherstep( float A, float B, float X ){
//    float t = linearstep( A, B, X );
    // float t= X;
    float t = map(X, A , B, 0.0, 1.0);

   return t * t * t * ( t * ( t * 6 - 15 ) + 10 );
}

//https://gist.github.com/sugi-cho/6a01cae436acddd72bdf
//http://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
//returns hsv in range [0,1]
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}





void main(){

    float depth=texture(depth_tex, uv_in).x;

    vec4 color=vec4(0);
    bool pixel_covered_by_mesh=true;
    if(depth==1.0){
        pixel_covered_by_mesh=false;
        if (show_background_img || show_environment_map || show_prefiltered_environment_map){// //there is no mesh or anything covering this pixel, we either read the backgrpund or just set the pixel to the background color
            color = texture(composed_tex, uv_in);
        }else{ //if it's not covered by a mesh we might still need to color this pixel with the bloom texture so we just accumulate on top of a color of zero
            color=vec4(0.0);
         }
    }else{ //pixel is covered by mesh so we read the color it has
        color = texture(composed_tex, uv_in);
    }

    if (enable_bloom){ //add the bloom from all the blurred textures
        float bloom_global_weight=1.0;
        for (int i=bloom_start_mip_map_lvl; i<bloom_max_mip_map_lvl; i++){
            vec4 bloom = textureLod(bloom_tex, uv_in, i);
            float bloom_weight=bloom.w;
            color.xyz+=bloom.rgb;
            color.w+=bloom_weight*bloom_global_weight;
        }
    }

    //see the weight as a color
    // color.xyz=vec3(color.w);
    // color.xyz=textureLod(bloom_tex, uv_in, 4).rgb;
    // color.xyz=vec3(textureLod(bloom_tex, uv_in, 5).w);
    // color.xyz=vec3(luminance(color.xyz));

    //modify hsv
    // color.xyz=rgb2hsv(color.xyz);
    // color.x+=hue_shift;
    // color.y+=saturation_shift;
    // color.z+=value_shift;
    // color.xyz=hsv2rgb(color.xyz);

    // exposure and tonemap
    color.xyz*=exposure;
    // int tonemap_type=4;
    if (tonemap_type==0){//linear
        //do nothing
        color.xyz = pow(color.xyz, vec3(1.0/2.2));
    }else if (tonemap_type==1){//reinhardt
        color.xyz=Tonemap_Reinhard(color.xyz);
        color.xyz = pow(color.xyz, vec3(1.0/2.2));
    }else if (tonemap_type==2){//Unreal
        color.xyz=Tonemap_Unreal(color.xyz);
    }else if (tonemap_type==3){ //filmicALU
        color.xyz=Tonemap_FilmicALU(color.xyz);
    }else if (tonemap_type==4){ //ACES
        color.xyz = transpose(aces_input)*color.xyz;
        color.xyz = RRTAndODTFit(color.xyz);
        color.xyz = transpose(aces_output)*color.xyz;
        // gamma correct
        color.xyz = pow(color.xyz, vec3(1.0/2.2));
    }

    if(!pixel_covered_by_mesh){
        float lum = clamp(luminance(color.xyz), 0.0, 1.0);
        color.w=lum;
    }

    //debug
    // color.xyz=vec3(color.w);
    // vec3 color_pure=color.xyz;
    // float clamped_weight=color.w;
    //debug
    // out_color=vec4(color.w);
    // return;

    //we want to have a pure color and let the alpha do the weithing between the background and the foreground
    float clamped_weight=clamp(color.w, 0.0, 1.0);
    vec3 color_pure=color.xyz;
    if (!pixel_covered_by_mesh){ //if the pixel is covered by the mes we just output the weight which is 1.0 and the color. However if the pixel is NOT covered then we need to get the pure color and the weight separetlly and let the subsequent blender to the fusing with the bg
        if ( show_background_img || show_environment_map || show_prefiltered_environment_map ){

            clamped_weight=1.0;
        }else{
            if(clamped_weight!=0){
                color_pure.xyz/=clamped_weight;
            }
            // float lum = smootherstep(0.0, 1.2, luminance(color.xyz));
            clamped_weight=smootherstep(0.0, 1.0, clamped_weight);
        }
    }





    // //the alpha of the pixel will be 1 if it's covered by mesh, and depeneding on how strong the bloom is we will have a decaying weight
    // float color_weight=1.0;
    // if(pixel_covered_by_mesh||show_background_img || show_environment_map || show_prefiltered_environment_map){
    //     color_weight=1.0;
    //     // color_weight=texture(diffuse_tex, uv_in).w;
    // }else{
    //     //pixel is not covered by mesh therefore
    //     if (!enable_bloom){
    //         color_weight=0.0;
    //     }else{ //we have bloom
    //         color_weight=clamp(luminance(color_posprocessed),0.0, 1.0);
    //         color_weight=smoothstep(0.0, 1.0, color_weight);
    //     }
    // }

    // //linear interpolation betweek the background and the color+bloom. We do this so that we can have transparency for the bloom part in case we want to save the viewer as a png and then change the background
    // // vec3 color_and_bloom_weighted= clamp(color_posprocessed*color_weight, vec3(0.0), vec3(1.0));
    // // vec3 background_weighted=clamp( background_color*(1.0 - color_weight) , vec3(0.0), vec3(1.0));
    // // vec3 color_posprocessed_mixed = color_and_bloom_weighted+background_weighted;

    // // vec3 final_color= color_posprocessed_mixed;
    // vec3 final_color= color_posprocessed;

    //if we have enabled the multichannel_view, we need to change the final color
    if (enable_multichannel_view ){
        int max_channels=5;

        // float line_separation_pixels=size_final_image.x * multichannel_interline_separation* abs( 1.0 - multichannel_line_angle/90.0); //the more the angle the more separated the lines are separated horizontally
        float line_separation_pixels=size_final_image.x * multichannel_interline_separation;
        vec2 pos_screen = vec2(uv_in.x*size_final_image.x, uv_in.y*size_final_image.y);
        //displace the position on screen in x direction to simulate that the lines are actually skewed
        float line_angle_radians=multichannel_line_angle * 3.1415 / 180.0;
        float tan_angle=tan(line_angle_radians);
        float displacement_x=tan_angle*(size_final_image.y-pos_screen.y);
        pos_screen.x+=displacement_x-multichannel_start_x;

        int nr_channel = int(floor(pos_screen.x/ line_separation_pixels));
        // int nr_channel_wrapped=int(mod(float(nr_channel),max_channels)); //if above channel 5, then wrap back to 0
        int nr_channel_wrapped=nr_channel;

        if(pixel_covered_by_mesh){
            //depending on the channel we put the normal, or the ao, or the diffuse, or the metalness, or the rougghness
            if(nr_channel_wrapped==0){ //final_color
                // final_color=color_posprocessed_mixed;
            }else if (nr_channel_wrapped==1){ //ao
                color_pure=(decode_normal(texture(normal_tex, uv_in).xyz ) +1.0)*0.5;
            }else if(nr_channel_wrapped==2){ //normal
                color_pure=vec3( texture(ao_tex, uv_in).x  );
            }else if(nr_channel_wrapped==3){ //diffuse
                vec4 color_with_weight=texture(diffuse_tex, uv_in);
                if (color_with_weight.w!=0.0){ //normalize it in case we are doing some surfel splatting
                    color_pure.xyz=color_with_weight.rgb/color_with_weight.w;
                }
            }else if(nr_channel_wrapped==4){ //metalness and roughness
                vec2 metalness_roughness=texture(metalness_and_roughness_tex, uv_in).xy;
                float weight = texture(diffuse_tex, uv_in).w;
                if (weight!=0.0){ //normalize it in case we are doing some surfel splatting
                    metalness_roughness/=weight;
                }
                color_pure=vec3(metalness_roughness, 0.0);
            }else{

                //color stays as the final one
                // final_color=vec3(0.0);
                // final_color=color_posprocessed_mixed;
            }
        }

        //if the pixel is close to the line then we just color it white
        float module = mod(pos_screen.x, line_separation_pixels);
        if(module<multichannel_line_width && nr_channel>0 && nr_channel<=max_channels){
            color_pure = vec3(0.1);
            clamped_weight=1.0;
        }

        // final_color = vec3(nr_channel/5.0);
        // final_color = vec3(module/500);
    }



    //modify hsv
    color_pure.xyz=clamp(color_pure.xyz, vec3(0), vec3(1));
    color_pure.xyz=rgb2hsv(color_pure.xyz);
    color_pure.x+=hue_shift;
    color_pure.y+=saturation_shift;
    color_pure.z+=value_shift;
    color_pure.xyz=hsv2rgb(color_pure.xyz);






    out_color=vec4(color_pure, clamped_weight);
    // out_color=vec4(color_posprocessed, color_weight);

}
