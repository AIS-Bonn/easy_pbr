#version 430

layout (local_size_x = 16, local_size_y = 16) in;

uniform sampler2D input_tex;
layout(rgba32f) uniform writeonly image2D colored_tex;

uniform float thermal_min_val;
uniform float thermal_max_val;
uniform vec3[256] colormap;

vec3 jet_color(float v, float vmin, float vmax) {
    vec3 c=vec3(1); //white
    float dv;

    // if (v < vmin)
    //     v = vmin;
    // if (v > vmax)
    //     v = vmax;
    v=clamp(v,vmin,vmax);
    dv = vmax - vmin;

    if (v < (vmin + 0.25 * dv)) {
        c.x = 0;
        c.y = 4 * (v - vmin) / dv;
    } else if (v < (vmin + 0.5 * dv)) {
        c.x = 0;
        c.z = 1 + 4 * (vmin + 0.25 * dv - v) / dv;
    } else if (v < (vmin + 0.75 * dv)) {
        c.x = 4 * (v - vmin - 0.5 * dv) / dv;
        c.z = 0;
    } else {
        c.y = 1 + 4 * (vmin + 0.75 * dv - v) / dv;
        c.z = 0;
    }

    return c;
}

vec3 colorize(float x){
    float x_clamped=clamp(x,0.0, 1.0);
    x_clamped*=255;
    int x_int = int(x_clamped);
    return colormap[x_int];
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax);  //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}


void main(void) {

    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    //grab the input color
    ivec2 img_size=textureSize( input_tex, 0);
    vec2 uv_coord=vec2( float(img_coords.x+0.5)/img_size.x, float(img_coords.y+0.5)/img_size.y);
    vec4 input_color=texture(input_tex, uv_coord);
    // input_color/=input_color.w; //renomarlize in case we accidentally get  

    //colormap it
    vec4 colored_pixel=input_color;
    colored_pixel=vec4(input_color.x, input_color.x, input_color.x, input_color.w); //passthrough 

    //if the thermal color is black or if the weight is zero then we just pass throguh
    // if(input_color.x==0 || input_color.w==0){
    if(input_color.x==0 || input_color.w==0){
        colored_pixel=vec4(input_color.x, input_color.x, input_color.x, input_color.w); //passthrough 
    }else{
        // colored_pixel.xyz=jet_color(input_color.x, 0.331, 0.37);
        // colored_pixel.xyz=jet_color(input_color.x, 0.32, 0.36);
        // colored_pixel.xyz=jet_color(input_color.x, 0.34, 0.36);
        // colored_pixel.xyz=jet_color(input_color.x, 0.34, 0.38); // quite nice with nice blues ands reds
        // colored_pixel.xyz=jet_color(input_color.x, 0.34, 0.42); // used for the mbzirc report


        //tonamping the thermal color to the rgb colors instead of just doing a linear mapping
        // float intensity_linear= map(input_color.x, 0.34, 0.42, 0.0, 1.0);
        // float intensity_tonemapped=intensity_linear; 
        // float intensity_tonemapped=0;
        //TODO tonemap it
        int lvl=1;
        vec4 average_color=textureLod(input_tex, uv_coord, lvl);
        vec4 global_average_color=textureLod(input_tex, vec2(0.5, 0.5), 11); //global average color
        average_color.xyz/=average_color.w; //the averaging done by the mip mapping may blend some pixels in the background too so in that case the alpha channel will be smaller than 1, so we renormelize
        global_average_color.xyz/=global_average_color.w;
        // global_average_color.x=0.346661;
        //map the average color and the global average to the same range as the input_intensity 
        average_color.x=map(average_color.x, thermal_min_val, thermal_max_val, 0.0, 1.0);
        global_average_color.x=map(global_average_color.x, thermal_min_val, thermal_max_val, 0.0, 1.0);

        // //check if the average color is ok
        // intensity_tonemapped=average_color.x;
        // colored_pixel.xyz=jet_color(intensity_tonemapped, 0.33, 0.37); // used for the mbzirc report
        // imageStore(colored_tex, img_coords , colored_pixel );
        // return;

        //the lower you put the white point the more contrast you have, and you also have to reduce the exposure

        //reinahrdt tonemapping paper http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
        //attempt 2 based on https://www.gamedev.net/forums/topic/407348-reinhards-tone-mapping-operator/
        //first map the range so that we have in 0,1 the part we are interested in
        // float input_intensity=map(input_color.x, 0.33, 0.45, 0.0, 1.0);
        float input_intensity=map(input_color.x, thermal_min_val, thermal_max_val, 0.0, 1.0);
        // (Lp) Map average luminance to the middlegrey zone by scaling pixel luminance
        // float exposure=0.24; //this seems to work fine with the white point set to 1.2
        float exposure=0.9; //this seems to work fine with the white point set to 2.0
        float Lp = input_intensity * exposure / global_average_color.x;                       
        // (Ld) Scale all luminance within a displayable range of 0 to 1 whitepoint is explained here https://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3/
        float white_point=20.0; //the more you increase this the dimmer you will make the whites. efectivelly gives more contrast to the whole image while squishing the whites
        // float Ld = (Lp * (1.0f + Lp/(white_point * white_point)))/(1.0f + Lp);
        float Ld = Lp/(1.0f + Lp); //this prevents any color to being blown to full white
        // float Ld = Lp/(1.0f + average_color.x); //this is the equation 9 in the paper
        // float Ld = Lp/(1.0f + average_color.x *exposure/global_average_color.x ); //this is the equatuon 9 but modified with my things
        //store
        // colored_pixel.xyz=jet_color(Ld, 0.0, 1.0);
        colored_pixel.xyz=colorize(Ld);
        // colored_pixel.xyz=colorize(input_intensity);


        //reinhard normalization from equiation 2 in  http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf  
        // float alpha=0.8;
        // intensity_tonemapped= alpha * input_color.x/ average_color.x;
        // intensity_tonemapped= intensity_tonemapped / (1 + intensity_tonemapped);
        // intensity_tonemapped=intensity_linear/ (intensity_linear+1);
        // intensity_tonemapped=map(average_color.x, 0.34, 0.42, 0.0, 1.0);


        // colored_pixel.xyz=jet_color(intensity_tonemapped, 0.0, 1.0); // used for the mbzirc report

    }
   

    //write it down
    imageStore(colored_tex, img_coords , colored_pixel );

}