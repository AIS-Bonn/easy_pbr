#version 430 core

// #extension GL_ARB_shading_language_include : require
// #include "color_utils.glsl"

//in
layout(location = 0) in vec3 position_cam_coords_in;
layout(location = 1) in vec2 uv_in;
layout(location = 2) in vec4 shadow_coord_in;
layout(location = 3) in float dist_to_cam_linear;
layout(location = 4) in vec3 normal_cam_coords_in;

//out 
layout(location = 0) out vec4 out_color;

//samplers and images
uniform sampler2D shadow_map_depth; 
uniform sampler2D tex_to_bake; 
layout(rgba32f) uniform coherent image2D bake_tex;
layout(r32f) uniform coherent image2D rgb_global_weight_tex;

//uniforms
uniform int bake_tex_size = 0;
uniform bool is_thermal=false;
uniform float max_dist;
uniform float dist_falloff_start;
uniform float dist_falloff_end;
uniform vec2 thermal_hacky_offset;

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax);  //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}





//bunch of color utils that should maybe go in another shader once I figure our how to do includes 
float luma(vec3 color) {
  return dot(color, vec3(0.299, 0.587, 0.114));
}

float luma(vec4 color) {
  return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

// Define saturation macro, if not already user-defined
#ifndef saturate
#define saturate(v) clamp(v, 0, 1)
#endif

// Constants
const float HCV_EPSILON = 1e-10;
const float HSL_EPSILON = 1e-10;
const float HCY_EPSILON = 1e-10;

const float SRGB_GAMMA = 1.0 / 2.2;
const float SRGB_INVERSE_GAMMA = 2.2;
const float SRGB_ALPHA = 0.055;


// Used to convert from linear RGB to XYZ space
const mat3 RGB_2_XYZ = (mat3(
    0.4124564, 0.3575761, 0.1804375,
    0.2126729, 0.7151522, 0.0721750,
    0.0193339, 0.1191920, 0.9503041
));

// Used to convert from XYZ to linear RGB space
const mat3 XYZ_2_RGB = (mat3(
     3.2404542,-1.5371385,-0.4985314,
    -0.9692660, 1.8760108, 0.0415560,
     0.0556434,-0.2040259, 1.0572252
));

const vec3 LUMA_COEFFS = vec3(0.2126, 0.7152, 0.0722);


// Returns the luminance of a !! linear !! rgb color
float get_luminance(vec3 rgb) {
    return dot(LUMA_COEFFS, rgb);
}

// Converts a linear rgb color to a srgb color (approximated, but fast)
vec3 rgb_to_srgb_approx(vec3 rgb) {
    return pow(rgb, vec3(SRGB_GAMMA));
}

// Converts a srgb color to a rgb color (approximated, but fast)
vec3 srgb_to_rgb_approx(vec3 srgb) {
    return pow(srgb, vec3(SRGB_INVERSE_GAMMA));
}

// Converts a single linear channel to srgb
float linear_to_srgb(float channel) {
    if(channel <= 0.0031308)
        return 12.92 * channel;
    else
        return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;
}

// Converts a single srgb channel to rgb
float srgb_to_linear(float channel) {
    if (channel <= 0.04045)
        return channel / 12.92;
    else
        return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
}

// Converts a linear rgb color to a srgb color (exact, not approximated)
vec3 rgb_to_srgb(vec3 rgb) {
    return vec3(
        linear_to_srgb(rgb.r),
        linear_to_srgb(rgb.g),
        linear_to_srgb(rgb.b)
    );
}

// Converts a srgb color to a linear rgb color (exact, not approximated)
vec3 srgb_to_rgb(vec3 srgb) {
    return vec3(
        srgb_to_linear(srgb.r),
        srgb_to_linear(srgb.g),
        srgb_to_linear(srgb.b)
    );
}

// Converts a color from linear RGB to XYZ space
vec3 rgb_to_xyz(vec3 rgb) {
    return RGB_2_XYZ * rgb;
}

// Converts a color from XYZ to linear RGB space
vec3 xyz_to_rgb(vec3 xyz) {
    return XYZ_2_RGB * xyz;
}

// Converts a color from XYZ to xyY space (Y is luminosity)
vec3 xyz_to_xyY(vec3 xyz) {
    float Y = xyz.y;
    float x = xyz.x / (xyz.x + xyz.y + xyz.z);
    float y = xyz.y / (xyz.x + xyz.y + xyz.z);
    return vec3(x, y, Y);
}

// Converts a color from xyY space to XYZ space
vec3 xyY_to_xyz(vec3 xyY) {
    float Y = xyY.z;
    float x = Y * xyY.x / xyY.y;
    float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
    return vec3(x, Y, z);
}

// Converts a color from linear RGB to xyY space
vec3 rgb_to_xyY(vec3 rgb) {
    vec3 xyz = rgb_to_xyz(rgb);
    return xyz_to_xyY(xyz);
}

// Converts a color from xyY space to linear RGB
vec3 xyY_to_rgb(vec3 xyY) {
    vec3 xyz = xyY_to_xyz(xyY);
    return xyz_to_rgb(xyz);
}

// Converts a value from linear RGB to HCV (Hue, Chroma, Value)
vec3 rgb_to_hcv(vec3 rgb)
{
    // Based on work by Sam Hocevar and Emil Persson
    vec4 P = (rgb.g < rgb.b) ? vec4(rgb.bg, -1.0, 2.0/3.0) : vec4(rgb.gb, 0.0, -1.0/3.0);
    vec4 Q = (rgb.r < P.x) ? vec4(P.xyw, rgb.r) : vec4(rgb.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + HCV_EPSILON) + Q.z);
    return vec3(H, C, Q.x);
}

// Converts from pure Hue to linear RGB
vec3 hue_to_rgb(float hue)
{
    float R = abs(hue * 6 - 3) - 1;
    float G = 2 - abs(hue * 6 - 2);
    float B = 2 - abs(hue * 6 - 4);
    return saturate(vec3(R,G,B));
}

// Converts from HSV to linear RGB
vec3 hsv_to_rgb(vec3 hsv)
{
    vec3 rgb = hue_to_rgb(hsv.x);
    return ((rgb - 1.0) * hsv.y + 1.0) * hsv.z;
}

// Converts from HSL to linear RGB
vec3 hsl_to_rgb(vec3 hsl)
{
    vec3 rgb = hue_to_rgb(hsl.x);
    float C = (1 - abs(2 * hsl.z - 1)) * hsl.y;
    return (rgb - 0.5) * C + hsl.z;
}

// Converts from HCY to linear RGB
vec3 hcy_to_rgb(vec3 hcy)
{
    const vec3 HCYwts = vec3(0.299, 0.587, 0.114);
    vec3 RGB = hue_to_rgb(hcy.x);
    float Z = dot(RGB, HCYwts);
    if (hcy.z < Z) {
        hcy.y *= hcy.z / Z;
    } else if (Z < 1) {
        hcy.y *= (1 - hcy.z) / (1 - Z);
    }
    return (RGB - Z) * hcy.y + hcy.z;
}


// Converts from linear RGB to HSV
vec3 rgb_to_hsv(vec3 rgb)
{
    vec3 HCV = rgb_to_hcv(rgb);
    float S = HCV.y / (HCV.z + HCV_EPSILON);
    return vec3(HCV.x, S, HCV.z);
}

// Converts from linear rgb to HSL
vec3 rgb_to_hsl(vec3 rgb)
{
    vec3 HCV = rgb_to_hcv(rgb);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + HSL_EPSILON);
    return vec3(HCV.x, S, L);
}

// Converts from rgb to hcy (Hue, Chroma, Luminance)
vec3 rgb_to_hcy(vec3 rgb)
{
    const vec3 HCYwts = vec3(0.299, 0.587, 0.114);
    // Corrected by David Schaeffer
    vec3 HCV = rgb_to_hcv(rgb);
    float Y = dot(rgb, HCYwts);
    float Z = dot(hue_to_rgb(HCV.x), HCYwts);
    if (Y < Z) {
      HCV.y *= Z / (HCY_EPSILON + Y);
    } else {
      HCV.y *= (1 - Z) / (HCY_EPSILON + 1 - Y);
    }
    return vec3(HCV.x, HCV.y, Y);
}

// Additional conversions converting to rgb first and then to the desired
// color space.

// To srgb
vec3 xyz_to_srgb(vec3 xyz)  { return rgb_to_srgb(xyz_to_rgb(xyz)); }
vec3 xyY_to_srgb(vec3 xyY)  { return rgb_to_srgb(xyY_to_rgb(xyY)); }
vec3 hue_to_srgb(float hue) { return rgb_to_srgb(hue_to_rgb(hue)); }
vec3 hsv_to_srgb(vec3 hsv)  { return rgb_to_srgb(hsv_to_rgb(hsv)); }
vec3 hsl_to_srgb(vec3 hsl)  { return rgb_to_srgb(hsl_to_rgb(hsl)); }
vec3 hcy_to_srgb(vec3 hcy)  { return rgb_to_srgb(hcy_to_rgb(hcy)); }

// To xyz
vec3 srgb_to_xyz(vec3 srgb) { return rgb_to_xyz(srgb_to_rgb(srgb)); }
vec3 hue_to_xyz(float hue)  { return rgb_to_xyz(hue_to_rgb(hue)); }
vec3 hsv_to_xyz(vec3 hsv)   { return rgb_to_xyz(hsv_to_rgb(hsv)); }
vec3 hsl_to_xyz(vec3 hsl)   { return rgb_to_xyz(hsl_to_rgb(hsl)); }
vec3 hcy_to_xyz(vec3 hcy)   { return rgb_to_xyz(hcy_to_rgb(hcy)); }

// To xyY
vec3 srgb_to_xyY(vec3 srgb) { return rgb_to_xyY(srgb_to_rgb(srgb)); }
vec3 hue_to_xyY(float hue)  { return rgb_to_xyY(hue_to_rgb(hue)); }
vec3 hsv_to_xyY(vec3 hsv)   { return rgb_to_xyY(hsv_to_rgb(hsv)); }
vec3 hsl_to_xyY(vec3 hsl)   { return rgb_to_xyY(hsl_to_rgb(hsl)); }
vec3 hcy_to_xyY(vec3 hcy)   { return rgb_to_xyY(hcy_to_rgb(hcy)); }

// To HCV
vec3 srgb_to_hcv(vec3 srgb) { return rgb_to_hcv(srgb_to_rgb(srgb)); }
vec3 xyz_to_hcv(vec3 xyz)   { return rgb_to_hcv(xyz_to_rgb(xyz)); }
vec3 xyY_to_hcv(vec3 xyY)   { return rgb_to_hcv(xyY_to_rgb(xyY)); }
vec3 hue_to_hcv(float hue)  { return rgb_to_hcv(hue_to_rgb(hue)); }
vec3 hsv_to_hcv(vec3 hsv)   { return rgb_to_hcv(hsv_to_rgb(hsv)); }
vec3 hsl_to_hcv(vec3 hsl)   { return rgb_to_hcv(hsl_to_rgb(hsl)); }
vec3 hcy_to_hcv(vec3 hcy)   { return rgb_to_hcv(hcy_to_rgb(hcy)); }

// To HSV
vec3 srgb_to_hsv(vec3 srgb) { return rgb_to_hsv(srgb_to_rgb(srgb)); }
vec3 xyz_to_hsv(vec3 xyz)   { return rgb_to_hsv(xyz_to_rgb(xyz)); }
vec3 xyY_to_hsv(vec3 xyY)   { return rgb_to_hsv(xyY_to_rgb(xyY)); }
vec3 hue_to_hsv(float hue)  { return rgb_to_hsv(hue_to_rgb(hue)); }
vec3 hsl_to_hsv(vec3 hsl)   { return rgb_to_hsv(hsl_to_rgb(hsl)); }
vec3 hcy_to_hsv(vec3 hcy)   { return rgb_to_hsv(hcy_to_rgb(hcy)); }

// To HSL
vec3 srgb_to_hsl(vec3 srgb) { return rgb_to_hsl(srgb_to_rgb(srgb)); }
vec3 xyz_to_hsl(vec3 xyz)   { return rgb_to_hsl(xyz_to_rgb(xyz)); }
vec3 xyY_to_hsl(vec3 xyY)   { return rgb_to_hsl(xyY_to_rgb(xyY)); }
vec3 hue_to_hsl(float hue)  { return rgb_to_hsl(hue_to_rgb(hue)); }
vec3 hsv_to_hsl(vec3 hsv)   { return rgb_to_hsl(hsv_to_rgb(hsv)); }
vec3 hcy_to_hsl(vec3 hcy)   { return rgb_to_hsl(hcy_to_rgb(hcy)); }

// To HCY
vec3 srgb_to_hcy(vec3 srgb) { return rgb_to_hcy(srgb_to_rgb(srgb)); }
vec3 xyz_to_hcy(vec3 xyz)   { return rgb_to_hcy(xyz_to_rgb(xyz)); }
vec3 xyY_to_hcy(vec3 xyY)   { return rgb_to_hcy(xyY_to_rgb(xyY)); }
vec3 hue_to_hcy(float hue)  { return rgb_to_hcy(hue_to_rgb(hue)); }
vec3 hsv_to_hcy(vec3 hsv)   { return rgb_to_hcy(hsv_to_rgb(hsv)); }
vec3 hsl_to_hcy(vec3 hcy) { return rgb_to_hcy(hsl_to_rgb(hcy)); }





void main(){

    // out_color=vec4(1.0, 0.0, 0.0, 0.0);
    vec2 hacky_offset=vec2(0.0, 0.02); //move the sampling a bit higher
    if(is_thermal){
        hacky_offset=thermal_hacky_offset;
    }
    
    ivec2 img_coords = ivec2(bake_tex_size * clamp(uv_in, 0.0, 1.0));

    bool is_frag_visible=true;

    //outside the image check
    vec4 scPostW = shadow_coord_in / shadow_coord_in.w;
    if (shadow_coord_in.w <= 0.0f || (scPostW.x < 0 || scPostW.y < 0) || (scPostW.x >= 1 || scPostW.y >= 1)) { 
        is_frag_visible=false;
    }

    //shadow check
    float dist_from_cam = textureProj(shadow_map_depth, shadow_coord_in).x;
    float epsilon = 0.0002;
    if (dist_from_cam + epsilon < scPostW.z){
        is_frag_visible=false;
    }

    if(is_frag_visible){
        // imageStore(bake_tex, img_coords , vec4(0.0, 0.0, 1.0, 1.0) );
        // return;

        //debug why the fuck do some pixels score sas visible when they obviously should not be 
        // float visibility= dist_from_cam-scPostW.z;
        // visibility= map (visibility, 0.0, 0.1, 0.0, 1.0);
        // imageStore(bake_tex, img_coords , vec4(0.0, 0.0, visibility, 1.0) );
        // return;


        //cur
        vec2 img_local_uv=shadow_coord_in.xy/shadow_coord_in.w; //uv coords into the image that we are using for baking, so the small local one
        img_local_uv+=hacky_offset;
        vec4 color_cur=texture(tex_to_bake,img_local_uv);
        color_cur.w=1.0; //the texture sampling may also get some pixels from the background which is set to alpha 0. so we make it alpha 1 again
        float weight_cur=1.0;

        //if it's rgb image we increase a bit the saturation
        if(!is_thermal){
            vec3 color_cur_3_hsv=rgb_to_hsv(color_cur.xyz);
            color_cur_3_hsv.y=color_cur_3_hsv.y+0.1; //saturation
            color_cur_3_hsv.z=color_cur_3_hsv.z+0.1; //value 
            color_cur_3_hsv.z=clamp(color_cur_3_hsv.z, 0.0, 1.0); //don't make it too bright..
            color_cur.xyz=hsv_to_rgb(color_cur_3_hsv);
            color_cur.z= color_cur.z-0.02;//make less blue
        }

        //modify color cur based on vigneting
        //weight due to vignieting, weigth will be 1.0 in the middle and decaying as it goes to borders
        //img_local_pos_nudged in in range [0,1]
        vec2 vign_dist=img_local_uv-vec2(0.5, 0.5);
        vign_dist=abs(vign_dist); //goes from [0 to 0.5]
        float vign_dist_1d=length(vign_dist); //goes from [0 to 0.7]
        float vign_weight= map(vign_dist_1d, 0.35, 0.5, 1.0, 0.001);
        // if(is_thermal){
            // vign_weight=1.0;//for thermal we disable the vigneetting term
        // }
        //if is thermal we have to make the themperature cooler with more distance
        // if(is_thermal){
        //     // float cooling_factor=map(dist_to_cam_linear, 0.0, max_dist, 1.0, 0.98);
        //     float cooling_factor=map(dist_to_cam_linear*dist_to_cam_linear, 0.0, max_dist*max_dist, 0.99, 1.0);
        //     color_cur.x=color_cur.x*cooling_factor; //closer things we make cooler. even thought we probably should do it the other way around I will leave it like this for now
        //     // color_cur.x=cooling_factor;
        // }
        //weight dist 
        if(dist_to_cam_linear>max_dist){
            return;
        }
        float dist_weight=map(dist_to_cam_linear, dist_falloff_start, dist_falloff_end, 1.0, 0.0001);
        //view angle
        vec3 dir_cam_to_3d_grad = normalize(position_cam_coords_in); // vector that goes from the camera (which is at 0,0,0) to the fragment in 3D (the position in cam coordinates)
        float view_angle=dot(dir_cam_to_3d_grad, normalize(normal_cam_coords_in) ); //in the ideal case this should be -1 if we are looking dead on, if we are looking at a behind face it will be 1
        if(view_angle>-0.001){ //not actually completely behind but close to it
            discard;
        }
        float view_angle_weight=-view_angle;
        //if it's thermal we add another them to the viewing angle that penalizes very dead on measurements
        if(is_thermal){
            // float view_angle_0_1 = map(view_angle, -1, 0, 0, 1); // view angle of dead on will be 0 and the one very grazing will be one
            // float dead_on_angle=smoothstep(-1.0, -0.90, view_angle); //smooth hermite interpolation og the angle in between those two values
            // float dead_on_weight=map(dead_on_angle, -1.0, -0.9,  0.0, 1.0 ); // 

            float sigma=0.05; //amount of thermal info we downweight along the normal direction. the higher this number the more angle we downweight around the normal. Keeping it small will penalize only total direct reflection when the camera is looking dead on a surface
            float mu=0.0;
            float x=acos(-view_angle);
            float dead_on_weight= 1 - exp( -0.5* pow((x-mu)/sigma,2)  );
            dead_on_weight=clamp(dead_on_weight,0.0, 1.0); //TODO for some reason there are moments when this becomes nan which makes everything got to shit, i don't know why yet but it happens on the brandhaus mesher_simplifier_2. Maybe there is a problem with the normals and when normalizing a normal that is 0.0.0 it makes a nan for some view angle thing

            view_angle_weight*=dead_on_weight;
        }
        // //another cooling down based on view angle
        // if(is_thermal){
        //     float cooling_factor=map(view_angle, -1.0, 0.0, 0.99, 1.0);
        //     color_cur.x=color_cur.x*cooling_factor;
        // }

        weight_cur=vign_weight*dist_weight*view_angle_weight;

        // if(is_thermal){
        //     weight_cur=color_cur.x;
        // }

        //debugging
        // weight_cur=0.001;

        //debugging
        // weight_cur=vign_weight; //fine 
        // weight_cur=dist_weight; //fine 
        // weight_cur=view_angle_weight; //breaks it for some reason


        //prev
        vec4 color_prev=imageLoad(bake_tex, img_coords);
        float weight_prev=imageLoad(rgb_global_weight_tex, img_coords).x;


        //cap is converged
        float cap=30;
        // float cap=3000;
        if(is_thermal){
            cap=400;
            // cap=400000;
        }
        if(weight_prev>cap){
            discard;
        }

        // redo mesh
        // make options settable from config file 
        //     bag paused 
        //     light factor 
        //     ssao_radius
        //     ssao_power
        // make the changing of texture doable from the keyboard
        // made the renderer capture the gui or not


        //fused
        float new_weight=weight_prev+weight_cur;
        vec4 new_color=(weight_prev* color_prev + weight_cur*color_cur )/ new_weight;

        // //fusing with exponential averaging
        // float alpha=0.08;
        // if(weight_prev!=0.0){
        //     new_color= color_prev + alpha*(color_cur-color_prev);
        // }

        imageStore(bake_tex, img_coords , vec4(new_color.x, new_color.y, new_color.z, 1.0) );
        imageStore(rgb_global_weight_tex, img_coords , vec4(new_weight,0,0,1) );
        // memoryBarrier();

        //debug store the new weights
        // float new_weight=weight_prev+weight_cur;
        // new_weight/=10.0;
        // imageStore(bake_tex, img_coords , vec4(new_weight.x, new_weight.x, new_weight.x, 1.0) );

        //debug the color_prev //in iteration 1, the color_prev is already fucked up so something happened in iteration 0
        // if(weight_prev!=0.0){
        //     // imageStore(bake_tex, img_coords , vec4(0.0, 0.0, 1.0, 1.0) );
        //     imageStore(bake_tex, img_coords , vec4(color_prev.x, color_prev.x, color_prev.x, 1.0) );
        // }

        // //get what color we bake in iteration 0 
        // imageStore(bake_tex, img_coords , vec4(new_color.x, new_color.x, new_color.x, 1.0) );
        // memoryBarrier();
        // vec4 color_just_stored=imageLoad(bake_tex, img_coords);
        // imageStore(bake_tex, img_coords , vec4(color_just_stored.x, color_just_stored.x, color_just_stored.x, 1.0) );


    }else{
        // imageStore(bake_tex, img_coords , vec4(1.0, 0.0, 0.0, 1.0) );
    }


}