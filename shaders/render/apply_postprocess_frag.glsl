#version 430 core

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
uniform vec3 background_color;
uniform bool show_background_img;
uniform bool show_environment_map;
uniform bool show_prefiltered_environment_map;
uniform bool enable_multichannel_view;
uniform vec2 size_final_image;
uniform float multichannel_interline_separation;
uniform float multichannel_line_width;
uniform float multichannel_line_angle;
uniform float multichannel_start_x;


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
vec3 decode_normal(vec2 normal){
    //comment this out because the if condition actually costs us 1ms per frame. And it's not relevant usually as for point cloud which dont have normals we will use only EDL
    // if(!has_normals ){ //if we got a normal that is zero like the one we would get from a point cloud without normals, then we just output a zero to indicate no lighting needs to be done
        // return vec3(0);
    // }
    vec3 normal_decoded;
    normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
    normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
    return normal_decoded;

}

void main(){

    float depth=texture(depth_tex, uv_in).x;

    vec3 color;
    // float alpha=1.0;
    bool pixel_covered_by_mesh=true;
    if(depth==1.0){
        pixel_covered_by_mesh=false;
        // //there is no mesh or anything covering this pixel, we either read the backgrpund or just set the pixel to the background color
         if (show_background_img || show_environment_map || show_prefiltered_environment_map){
            color = texture(composed_tex, uv_in).rgb;
         }else{
            //  color=background_color;

            // //if it's not covered by a mesh we might still need to color this pixel with the bloom texture so we just accumulate on top of a color of zero
            color=vec3(0.0); 

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
        for (int i=bloom_start_mip_map_lvl; i<bloom_max_mip_map_lvl; i++){
            vec4 bloom = textureLod(bloom_tex, uv_in, i);
            float bloom_weight=bloom.w;
            color_posprocessed+=bloom.rgb*bloom_weight*bloom_global_weight;
        }

        //DEBUG show just one bloom tex
        // vec4 bloom = textureLod(bloom_tex, uv_in, 5);
        // color_posprocessed=bloom.rgb;
    }

    // if(color!=background_color){
    // if(depth==1.0 && !show_background_img && !show_environment_map  ){
        //tonemap and gamma correct 
        color_posprocessed*=exposure;
        int tonemap_type=4;
        if (tonemap_type==0){//linear
            //do nothing
            color_posprocessed = pow(color_posprocessed, vec3(1.0/2.2)); 
        }else if (tonemap_type==1){//reinhardt
            color_posprocessed=Tonemap_Reinhard(color_posprocessed);
            color_posprocessed = pow(color_posprocessed, vec3(1.0/2.2)); 
        }else if (tonemap_type==2){//Unreal
            color_posprocessed=Tonemap_Unreal(color_posprocessed);
        }else if (tonemap_type==3){ //filmicALU
            color_posprocessed=Tonemap_FilmicALU(color_posprocessed);
        }else if (tonemap_type==4){ //ACES
            color_posprocessed = transpose(aces_input)*color_posprocessed;
            color_posprocessed = RRTAndODTFit(color_posprocessed);
            color_posprocessed = transpose(aces_output)*color_posprocessed;
            // gamma correct
            color_posprocessed = pow(color_posprocessed, vec3(1.0/2.2)); 
        }
        // color_posprocessed = transpose(aces_input)*color_posprocessed;
        // color_posprocessed = RRTAndODTFit(color_posprocessed);
        // color_posprocessed = transpose(aces_output)*color_posprocessed;
        // gamma correct
        // color_posprocessed = pow(color_posprocessed, vec3(1.0/2.2)); 
    // }

    //the alpha of the pixel will be 1 if it's covered by mesh, and depeneding on how strong the bloom is we will have a decaying weight
    float color_weight=1.0;
    if(pixel_covered_by_mesh||show_background_img || show_environment_map || show_prefiltered_environment_map){
        color_weight=1.0;
    }else{
        //pixel is not covered by mesh therefore
        if (!enable_bloom){
            color_weight=0.0;
        }else{ //we have bloom
            color_weight=clamp(luminance(color_posprocessed),0.0, 1.0);
            color_weight=smoothstep(0.0, 1.0, color_weight);
        }
    }

    //linear interpolation betweek the background and the color+bloom. We do this so that we can have transparency for the bloom part in case we want to save the viewer as a png and then change the background
    vec3 color_and_bloom_weighted= clamp(color_posprocessed*color_weight, vec3(0.0), vec3(1.0));
    vec3 background_weighted=clamp( background_color*(1.0 - color_weight) , vec3(0.0), vec3(1.0));
    vec3 color_posprocessed_mixed = color_and_bloom_weighted+background_weighted;
    // vec3 color_posprocessed_mixed = color_and_bloom_weighted;
    // vec3 color_posprocessed_mixed = background_weighted;
    // color_posprocessed_mixed=vec3(color_weight);

    vec3 final_color= color_posprocessed_mixed; 

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
                final_color=color_posprocessed_mixed;
            }else if (nr_channel_wrapped==1){ //ao
                final_color=(decode_normal(texture(normal_tex, uv_in).xy ) +1.0)*0.5;
            }else if(nr_channel_wrapped==2){ //normal
                final_color=vec3( texture(ao_tex, uv_in).x  );
            }else if(nr_channel_wrapped==3){ //diffuse
                final_color=texture(diffuse_tex, uv_in).xyz;
            }else if(nr_channel_wrapped==4){ //metalness and roughness
                final_color=vec3( texture(metalness_and_roughness_tex, uv_in).xy, 0.0  );
            }else{
                // final_color=vec3(0.0);
                final_color=color_posprocessed_mixed;
            }
        }

        //if the pixel is close to the line then we just color it white
        float module = mod(pos_screen.x, line_separation_pixels);
        if(module<multichannel_line_width && nr_channel>0 && nr_channel<=max_channels){
            final_color = vec3(1.0);
            color_weight=1.0;
        }

        // final_color = vec3(nr_channel/5.0);
        // final_color = vec3(module/500);
    }

 









    out_color=vec4(final_color, color_weight);
    // out_color=vec4(color_posprocessed, color_weight);

}
