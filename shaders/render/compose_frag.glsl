#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
layout(location=1) in vec2 uv_in;
layout(location=2) in vec3 view_ray_in;
layout(location=3) in vec3 world_view_ray_in;

//out
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 bloom_color;

uniform sampler2D normal_tex;
// uniform sampler2D position_cam_coords_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D metalness_and_roughness_tex;
uniform sampler2D log_depth_tex;
uniform sampler2D ao_tex;
uniform sampler2D ao_gtex;
uniform sampler2D depth_tex;
uniform sampler2D background_tex;
uniform samplerCube environment_cubemap_tex;
uniform samplerCube irradiance_cubemap_tex;
uniform samplerCube prefilter_cubemap_tex;
uniform sampler2D brdf_lut_tex;
//WARNING: currently we cannot create more texture samplers because then we would have more than 16 texture samplers and some gpu drivers don't like. We might want to put same sampler in a sampler array for later

//uniform
uniform mat4 V_inv; //project from pos_cam_coords back to world coordinates
// uniform mat4 V; //from projecting a light position from world into the main camera
uniform vec3 eye_pos;
uniform int color_type;
uniform vec3 solid_color;
uniform vec3 ambient_color;
uniform float ambient_color_power;
uniform sampler2D ao_img;
uniform float ssao_subsample_factor;
uniform vec2 viewport_size;
uniform bool enable_ssao;
uniform bool enable_edl_lighting;
uniform float edl_strength;
uniform bool show_background_img;
uniform bool show_environment_map;
uniform bool show_prefiltered_environment_map;
uniform bool enable_ibl;
uniform float projection_a; //for calculating position from depth according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
uniform float projection_b;
uniform float exposure;
uniform bool enable_bloom;
uniform float bloom_threshold;
uniform float environment_map_blur;
uniform int prefilter_nr_mipmaps;
uniform bool using_fat_gbuffer;
uniform bool is_ortho;
uniform bool get_ao_from_precomputation;
//pcss shadow things
const int MAX_NR_PCSS_SAMPLES=256;
uniform vec2 pcss_blocker_samples[MAX_NR_PCSS_SAMPLES];
uniform vec2 pcss_pcf_samples[MAX_NR_PCSS_SAMPLES];
uniform int nr_pcss_blocker_samples;
uniform int nr_pcss_pcf_samples;


//for edl
const int neighbour_count=8;
uniform vec2 neighbours[neighbour_count];

//support for maximum 8 lights
struct SpotLight {
    vec3 pos;
    vec3 color; //the color of the light
    float power; // how much strenght does the light have
    mat4 VP; //projects world coordinates into the light
    sampler2D shadow_map;
    bool create_shadow;
    float near_plane;
};
uniform SpotLight spot_lights[3]; //cannot create more than 3 light because then we would have more than 16 texture samplers and some gpu drivers don't like. We might want to put same sampler in a sampler array for later
// uniform Light omni_lights[8]; //At the moment I drop support for omni light at least partially until I have a class that can draw shadow maps into a omni light
uniform int nr_active_spot_lights;


const float PI = 3.14159265359;



float linear_depth(float depth_sample){
    // depth_sample = 2.0 * depth_sample - 1.0;
    // float z_linear = 2.0 * z_near * z_far / (z_far + z_near - depth_sample * (z_far - z_near));
    // return z_linear;

    // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    // float ProjectionA = z_far / (z_far - z_near);
    // float ProjectionB = (-z_far * z_near) / (z_far - z_near);
    
    float linearDepth;
    if (is_ortho){
        linearDepth= depth_sample;
    }else{
        linearDepth= projection_b / (depth_sample - projection_a);
    }

    return linearDepth;

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
    if (normal==vec3(0)){
        return normal;
    }

    if(using_fat_gbuffer){
        return normalize(normal);
    }else{
        return normalize(normal * 2.0 - 1.0);
    }
}

//  //decode normals as done by david bernard in https://hub.jmonkeyengine.org/t/solved-strange-shining-problem/32962/4 and https://github.com/davidB/jme3_ext_deferred/blob/master/src/main/resources/ShaderLib/DeferredUtils.glsllib
//  // return +/- 1
// vec2 signNotZero(vec2 v) {
// 	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
// }
//  vec3 oct_to_float32x3(vec2 e) {
// 	vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
// 	if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
// 	return normalize(v);
// }
// vec2 unorm8x3_to_snorm12x2(vec3 u) {
// 	u *= 255.0;
// 	u.y *= (1.0 / 16.0);
// 	vec2 s = vec2(u.x * 16.0 + floor(u.y), fract(u.y) * (16.0 * 256.0) + u.z);
// 	return clamp(s * (1.0 / 2047.0) - 1.0, vec2(-1.0), vec2(1.0));
// }
// vec3 decode_normal(in vec3 unorm8x3Normal){
// 	return oct_to_float32x3(unorm8x3_to_snorm12x2(unorm8x3Normal));
// 	//return hemioct_to_float32x3(unorm8x3_to_snorm12x2(intNormal.rgb));
// }



vec3 position_cam_coords_from_depth(float depth){
    float linear_depth=linear_depth(depth);
    vec3 position_cam_coords;

    //same as at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    // Project the view ray onto the camera's z-axis
    vec3 eye_z_axis=vec3(0.0, 0.0, -1.0); //cam is looking down the negative z
    float viewZDist = dot(eye_z_axis, normalize(view_ray_in));

    // Scale the view ray by the ratio of the linear z value to the projected view ray
    position_cam_coords = normalize(view_ray_in) * (linear_depth / viewZDist);
    // position_cam_coords= normalize(view_ray_in) * linear_depth;
    return position_cam_coords;
}

// https://github.com/potree/potree/blob/65f6eb19ce7a34ce588973c262b2c3558b0f4e60/src/materials/shaders/edl.fs
float radius=1;
float edl_response(float depth){
	vec2 uvRadius = radius / vec2(viewport_size.x, viewport_size.y);

	float sum = 0.0;

	for(int i = 0; i < neighbour_count; i++){
		vec2 uvNeighbor = uv_in + uvRadius * neighbours[i];

		// float neighbourDepth = texture(log_depth_tex, uvNeighbor).x;
		float neighbourDepth = texture(depth_tex, uvNeighbor).x;
        neighbourDepth=linear_depth(neighbourDepth);
        neighbourDepth=log2(neighbourDepth);
		neighbourDepth = (neighbourDepth == 1.0) ? 0.0 : neighbourDepth;

		if(neighbourDepth != 0.0){
			if(depth == 0.0){
			// if(abs(depth - neighbourDepth) < 0.01){
				sum += 100.0;
				// sum += 1.0;
			}else{
				sum += max(0.0, abs(depth - neighbourDepth) );
				// sum += max(0.0, depth - neighbourDepth );
			}
		}
	}

	return sum / float(neighbour_count);
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax); //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}




//DFG equations from https://learnopengl.com/PBR/Lighting
float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
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

vec3 Tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

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

//smoothstep but with 1st and 2nd derivatives that go more smoothly to zero
float smootherstep( float A, float B, float X ){
//    float t = linearstep( A, B, X );
    // float t= X;
    float t = map(X, A , B, 0.0, 1.0);

   return t * t * t * ( t * ( t * 6 - 15 ) + 10 );
}

float compute_bloom_weight(vec3 color){
    vec3 color_tonemapped;
    color_tonemapped=color*exposure;
    // color_tonemapped = transpose(aces_input)*color_tonemapped;
    // color_tonemapped = RRTAndODTFit(color_tonemapped);
    // color_tonemapped = transpose(aces_output)*color_tonemapped;


    // float bloom_threshold=0.9;
    float brightness=luminance(color_tonemapped);
    float above_thresh=brightness-bloom_threshold;
    float bloom_weight=0.0;
    if(above_thresh>0.0){
        // bloom_weight=map(above_thresh, 0.0 , 0.1, 0.0, 1.0);
        // bloom_weight=smoothstep(0.0, 0.1, above_thresh);
        bloom_weight=smootherstep(0.0, 1.2, above_thresh);
        // bloom_weight=1.0;
        // bloom_weight=above_thresh;
    }

    // if (brightness>bloom_threshold){
        // return true;
    // }else{
        // return false;
    // }

    return bloom_weight;
}


//get background color from the background img or environment map dependig on the flags
vec3 get_bg_color(){
    vec3 color=vec3(0);
    if (show_background_img){
        color = texture(background_tex, uv_in).xyz;
    }else if(show_environment_map){
        color = texture(environment_cubemap_tex, normalize(world_view_ray_in) ).rgb;
    }else if(show_prefiltered_environment_map){
        color = textureLod(prefilter_cubemap_tex, normalize(world_view_ray_in), environment_map_blur ).rgb;
    }
    return color;
}

bool should_show_bg(){
    if (show_background_img || show_environment_map || show_prefiltered_environment_map){
        return true;
    }else{
        return false;
    }

}

vec3 get_edl_color(vec3 albedo, float depth){
    vec3 color=vec3(0);

    depth=linear_depth(depth);
    depth=log2(depth);
    depth = (depth == 1.0) ? 0.0 : depth;
    float res = edl_response(depth); 
    // float edl_strength=16.0;
    float shade = exp(-res * 300.0 * edl_strength);
    float ao= enable_ssao && !get_ao_from_precomputation? texture(ao_tex, uv_in).x : 1.0;
    color = albedo* shade * ao;

    return color;

}

// percentage close filtering like in http://ogldev.atspace.co.uk/www/tutorial42/tutorial42.html
float shadow_map_pcf(int light_idx, vec3 proj_in_light){

    float shadow_factor=0;

    ivec2 shadow_map_size=textureSize(spot_lights[light_idx].shadow_map,0);
    // ivec2 shadow_map_size=ivec2(1024);
    float xOffset = 1.0/shadow_map_size.x;
    float yOffset = 1.0/shadow_map_size.y;
    for (int y = -1 ; y <= 1 ; y++) {
        for (int x = -1 ; x <= 1 ; x++) {
            vec2 Offsets = vec2(x * xOffset, y * yOffset);
            vec2 UV = proj_in_light.xy + Offsets;
            float closest_depth = texture(spot_lights[light_idx].shadow_map, UV).x;
            float current_depth = proj_in_light.z;
            float epsilon = 0.0001;
            if (closest_depth + epsilon < current_depth){
                continue; //in shadow
            }else{
                shadow_factor+=1.0;
            }
        }
    }

    //normalize by the kernel size
    shadow_factor=shadow_factor / (3*3);

    return shadow_factor;

}


//https://github.com/pboechat/PCSS/blob/master/application/shaders/blinn_phong_textured_and_shadowed.fs.glsl
float shadow_map_pcf_rand_samples(vec3 shadowCoords, sampler2D shadowMap, float uvRadius){
    float epsilon = 0.0001;
	float shadow_factor = 0;
	for (int i = 0; i < nr_pcss_pcf_samples; i++){
        vec2 rand_dir=pcss_pcf_samples[i]; //is in range [-1,1]
        float current_depth = shadowCoords.z;
        float closest_depth = texture(shadowMap,  shadowCoords.xy + rand_dir* uvRadius).x;
        if (closest_depth + epsilon < current_depth){
            continue; //in shadow
        }else{
            shadow_factor+=1.0;
        }
	}
	return shadow_factor / nr_pcss_pcf_samples;
}

//mostly from here
// https://www.youtube.com/watch?v=LGFDifcbsoQ&t=700s
float linstep(float low, float high, float v){
    return clamp(  (v-low)/(high-low),  0.0, 1.0 );
}
float sample_vsm(sampler2D shadowMap, vec3 shadow_map_coords){





    //vsm 
    float shadow_factor=0;
    vec2 moments=texture(shadowMap, shadow_map_coords.xy).xy;
    if (moments.x==0){ 
        return 1.0; //if we sample in empty space, assume we are lit
    }
    float compare=shadow_map_coords.z;
    float p= step(compare, moments.x);
    float variance = max(moments.y -moments.x*moments.x, 0.000001);
    float d = compare-moments.x;
    float pMax= variance / (variance + d*d);
    // pMax= clamp(pMax, 0.2, 1.0)-0.2;
    pMax=linstep(0.3, 1.0, pMax);

    shadow_factor=min(max(p,pMax),1.0);



    //normal hard shadow mapping
    // float closest_depth = texture(spot_lights[light_idx].shadow_map, shadow_map_coords.xy).x;
    // float current_depth = shadow_map_coords.z;
    // float shadow_factor=0;
    // float epsilon = 0.0001;
    // if (closest_depth +epsilon < current_depth){
    //     //in shadow
    // }else{
    //     shadow_factor+=1.0;
    // }




    // ivec2 shadow_map_size=textureSize(spot_lights[light_idx].shadow_map,0);
    // // ivec2 shadow_map_size=ivec2(1024);
    // float xOffset = 1.0/shadow_map_size.x;
    // float yOffset = 1.0/shadow_map_size.y;
    // for (int y = -1 ; y <= 1 ; y++) {
    //     for (int x = -1 ; x <= 1 ; x++) {
    //         vec2 Offsets = vec2(x * xOffset, y * yOffset);
    //         vec2 UV = proj_in_light.xy + Offsets;
    //         float closest_depth = texture(spot_lights[light_idx].shadow_map, UV).x;
    //         float current_depth = proj_in_light.z;
    //         float epsilon = 0.0001;
    //         if (closest_depth + epsilon < current_depth){
    //             continue; //in shadow
    //         }else{
    //             shadow_factor+=1.0;
    //         }
    //     }
    // }

    //normalize by the kernel size
    // shadow_factor=shadow_factor / (3*3);

    return shadow_factor;

}


float sample_vsm_pcf(sampler2D shadowMap, vec3 shadow_map_coords, float uvRadius){

	float shadow_factor = 0;
	for (int i = 0; i < nr_pcss_pcf_samples; i++){
        vec2 rand_dir=pcss_pcf_samples[i]; //is in range [-1,1]
        vec3 new_shadow_map_coords= vec3( shadow_map_coords.xy + rand_dir* uvRadius, shadow_map_coords.z );
        shadow_factor+=sample_vsm(shadowMap, new_shadow_map_coords);
       
	}
	return shadow_factor / nr_pcss_pcf_samples;

}

// http://developer.download.nvidia.com/whitepapers/2008/PCSS_DirectionalLight_Integration.pdf
// float SearchWidth(float uvLightSize, float receiverDistance, vec3 pos_in_cam_coords){
	// return uvLightSize * (receiverDistance - cam_near) / pos_in_cam_coords.z;
// }
// https://github.com/pboechat/PCSS/blob/master/application/shaders/blinn_phong_textured_and_shadowed.fs.glsl
float find_blocker_distance(out int nr_blockers,vec3 shadowCoords, sampler2D shadowMap, float light_size_in_uv_space, vec3 pos_in_cam_coords, float light_near){
    // float epsilon = 0.0001;
    float epsilon = 0.000;
	nr_blockers = 0;
	float avgBlockerDistance = 0;
    float current_depth = shadowCoords.z;
	// float searchWidth = SearchWidth(light_size_in_uv_space, shadowCoords.z, pos_in_cam_coords);
	// float searchWidth = light_size_in_uv_space;
    float searchWidth = light_size_in_uv_space * ( current_depth - light_near) / current_depth;
	for (int i = 0; i < nr_pcss_blocker_samples; i++){
        vec2 rand_dir=pcss_blocker_samples[i]; //is in range [-1,1]
		float closest_depth = texture(shadowMap, shadowCoords.xy +  rand_dir* searchWidth).x;
		// if (z < (shadowCoords.z - directionalLightShadowMapBias)){
		// 	blockers++;
		// 	avgBlockerDistance += z;
		// }
        if (closest_depth  < current_depth + epsilon){ //in shadow
            nr_blockers++;
			avgBlockerDistance += closest_depth;
        }else{
            //lit fully
        }
	}
	if (nr_blockers > 0){
		return avgBlockerDistance / nr_blockers;
    }else{
		return -1;
    }
}




//for making better lights maybe this references would be helpful
// https://alextardif.com/arealights.html
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// https://developer.download.nvidia.com/presentations/2008/GDC/GDC08_SoftShadowMapping.pdf
// https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf
// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-8-summed-area-variance-shadow-maps
// https://http.download.nvidia.com/developer/presentations/2005/SIGGRAPH/Percentage_Closer_Soft_Shadows.pdf
//https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/DxTechArts/common-techniques-to-improve-shadow-depth-maps.md





void main(){


    //PBR again========= https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.1.lighting/1.1.pbr.fs
    float depth=texture(depth_tex, uv_in).x;
    vec3 color=vec3(0);
    float pixel_weight=1.0;
    float shadow_factor_total=1.0;
    if(depth==1.0){

        // // //there is no mesh or anything covering this pixel, we discard it so the pixel will show whtever the background was set to
        if( should_show_bg() ){
            vec3 color=get_bg_color();
            out_color = vec4(color, 1.0);
            if(enable_bloom){
                float bloom_weight=compute_bloom_weight(color);
                if(bloom_weight>0.0){
                    bloom_color=vec4(color, bloom_weight);
                }else{
                    bloom_color=vec4(0.0);
                }
            }
            return;
        }else{
            discard; //we are on a pixel not covered by a mesh and we are not showing any BG, so we just discard
        }



    }else{
        //this pixel is covering a mesh
        vec4 color_with_weight = texture(diffuse_tex, uv_in);
        // pixel_weight=clamp(color_with_weight.w, 0.0, 1.0);
        pixel_weight=color_with_weight.w;
        if (color_with_weight.w!=0.0 && using_fat_gbuffer){ //normalize it in case we are doing some surfel splatting
            color_with_weight.xyz/=color_with_weight.w;
        }
        vec3 albedo=pow( color_with_weight.xyz, vec3(2.2) );
        vec3 N= decode_normal(texture(normal_tex, uv_in).xyz );

        float ao=1.0;
        if (enable_ssao){
            if (get_ao_from_precomputation){    
                ao= texture(ao_gtex, uv_in).x; 
            }else{
                ao= texture(ao_tex, uv_in).x; 
            }
        }       

        // //edl lighting https://github.com/potree/potree/blob/65f6eb19ce7a34ce588973c262b2c3558b0f4e60/src/materials/shaders/edl.fs
        if(enable_edl_lighting  || N==vec3(0)){ //if we have no normal we may be in a point cloud with no normals and then we can just do edl, no IBL is possible
            color=get_edl_color(albedo, depth);
            color=color*vec3(ao);
        }else{
            //PBR-----------

            vec3 P_c = position_cam_coords_from_depth(depth); //position of the fragment in camera coordinates
            vec3 P_w = vec3(V_inv*vec4(P_c,1.0));
            vec3 V = normalize( eye_pos -P_w ); //view vector that goes from position of the fragment towards the camera
            vec3 R = reflect(-V, N);
            float metalness=texture(metalness_and_roughness_tex, uv_in).x;
            float roughness=texture(metalness_and_roughness_tex, uv_in).y;
            if(color_with_weight.w!=0.0){
                metalness/=color_with_weight.w;
                roughness/=color_with_weight.w;
            }
            // float ao= enable_ssao && !get_ao_from_precomputation ? texture(ao_tex, uv_in).x : 1.0;
            
            // float ao= enable_ssao ? texture(ao_tex, uv_in).x : 1.0;
            float NdotV = max(dot(N, V), 0.0);



            // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
            // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
            // vec3 F0 = vec3(0.04);
            // F0 = mix(F0, albedo, metalness);

            // Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
            vec3 F0 = mix(vec3(0.08), albedo, metalness);
            // F0=vec3(0.1, 0.8, 0.8);
            // F0*=0.1;

            // reflectance equation
            vec3 Lo = vec3(0.0);
            for(int i = 0; i < nr_active_spot_lights; ++i)
            {


                //check if the current fragment in in the fov of the light by pojecting from world back into the light
                vec4 pos_light_space=spot_lights[i].VP * vec4(P_w, 1.0);
                // perform perspective divide https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
                vec3 proj_in_light = pos_light_space.xyz / pos_light_space.w;
                // transform to [0,1] range because the proj_in_light is now in normalized device coordinates [-1,1] and we want it in [0,1] https://community.khronos.org/t/shadow-mapping-bias-before-the-w-divide/63877/6
                proj_in_light = proj_in_light * 0.5 + 0.5;
                //check if we are outside the image or behind it
                if (pos_light_space.w <= 0.0f || (proj_in_light.x < 0 || proj_in_light.y < 0) || (proj_in_light.x > 1 || proj_in_light.y > 1)) {
                    // continue; //it seems that if we don;t check for this we just get more light from the sides of the spotlight
                }


                float shadow_factor = 0.0;
                if(spot_lights[i].create_shadow){
                    
                    // shadow_factor+=shadow_map_pcf(i, proj_in_light);
                    // shadow_factor+=sample_vsm(spot_lights[i].shadow_map, proj_in_light);
                    shadow_factor+=sample_vsm_pcf(spot_lights[i].shadow_map, proj_in_light, 0.005);

                    //avg blocker
                    // float light_size_in_uv_space=0.0000002;
                    // float light_size_in_uv_space=0.01; //works when using directly the light size
                    // float light_size_in_uv_space=0.1; 
                    // float light_size_in_uv_space=0.4; 
                    // int nr_blockers=0;
                    // float avgBlockerDepth=find_blocker_distance(nr_blockers, proj_in_light, spot_lights[i].shadow_map, light_size_in_uv_space, P_c, spot_lights[i].near_plane);
                    // float zReceiver = proj_in_light.z; // Assumed to be eye-space z in this code
                    // float penumbraRatio = (zReceiver - avgBlockerDepth) / avgBlockerDepth;;
                    // float filterRadiusUV = penumbraRatio * light_size_in_uv_space * spot_lights[i].near_plane / proj_in_light.z;
                    // shadow_factor+=sample_vsm_pcf(spot_lights[i].shadow_map, proj_in_light, filterRadiusUV*10);

                    //debug avg blocker depth
                    // if (avgBlockerDepth==-1){
                    //     out_color = vec4(vec3(1.0, 0.0, 0.0), 1.0);
                    // }else{
                    //     out_color = vec4(vec3(avgBlockerDepth), 1.0);
                    // }
                    // return;


                    //debbug nr blockers
                    // if (nr_blockers==0){
                    //     out_color = vec4(vec3(1.0, 0.0, 0.0), 1.0);
                    // }else{
                    //     out_color = vec4(vec3(nr_blockers)/nr_pcss_blocker_samples, 1.0);
                    // }
                    // return;
                    
                    // out_color=vec4(vec3(penumbraRatio),1.0);
                    // out_color=vec4(vec3(filterRadiusUV)*100,1.0);
                    // return;








                    // if (avgBlockerDepth==-1){
                    //     shadow_factor+=1.0; //no blocked, fully lit
                    // }else{
                    //     float zReceiver = proj_in_light.z; // Assumed to be eye-space z in this code
                    //     // float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
                    //     float penumbraRatio = (zReceiver - avgBlockerDepth) / avgBlockerDepth;;
                    //     float filterRadiusUV = penumbraRatio * light_size_in_uv_space * spot_lights[i].near_plane / proj_in_light.z;
                    //     shadow_factor+=sample_vsm_pcf(spot_lights[i].shadow_map, proj_in_light, filterRadiusUV);
                    // }


                    // float penumbra_size=0.01;
                    // shadow_factor+=shadow_map_pcf_rand_samples(proj_in_light, spot_lights[i].shadow_map, penumbra_size);
                    // shadow_factor+=shadow_map_pcf_rand_samples_2(proj_in_light, spot_lights[i].shadow_map, penumbra_size);
                    // shadow_factor=( (shadow_factor / 18.0)*2.0);
                }else{
                    //does not create shadow so we just add color
                    shadow_factor=1.0;
                }
                shadow_factor_total=shadow_factor_total*shadow_factor;


                // //shadow check similar to https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
                // float closest_depth = texture(spot_lights[i].shadow_map, proj_in_light.xy).x;
                // float current_depth = proj_in_light.z;
                // float epsilon = 0.0001;
                // if (closest_depth + epsilon < current_depth){
                //     continue;
                // }


                // calculate per-light radiance
                vec3 L = normalize(spot_lights[i].pos - P_w);
                vec3 H = normalize(V + L);
                float distance = length(spot_lights[i].pos - P_w);
                float attenuation = 1.0 / (distance * distance);
                vec3 radiance = spot_lights[i].color *spot_lights[i].power * attenuation;

                // Cook-Torrance BRDF
                float NDF = DistributionGGX(N, H, roughness);
                float G   = GeometrySmith(N, V, L, roughness);
                vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

                vec3 nominator    = NDF * G * F;
                float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
                vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
                // specular=specular* vec3(0.1, 0.8, 0.8);

                // kS is equal to Fresnel
                vec3 kS = F;
                // for energy conservation, the diffuse and specular light can't
                // be above 1.0 (unless the surface emits light); to preserve this
                // relationship the diffuse component (kD) should equal 1.0 - kS.
                vec3 kD = vec3(1.0) - kS;
                // multiply kD by the inverse metalness such that only non-metals
                // have diffuse lighting, or a linear blend if partly metal (pure metals
                // have no diffuse light).
                kD *= 1.0 - metalness;

                // scale light by NdotL
                float NdotL = max(dot(N, L), 0.0);

                // add to outgoing radiance Lo
                Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow_factor;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
                // Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

            }

            // ambient lighting (we now use IBL as the ambient term)
            vec3 ambient=vec3(0.0);
            if (enable_ibl){
                // vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
                // Adjust Fresnel absed on roughness.
                vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
                vec3 Fs = F0 + Fr * pow(1.0 - NdotV, 5.0);
                // vec3 kS = F;
                // vec3 kD = 1.0 - kS;
                // kD *= 1.0 - metalness;
                vec3 irradiance = texture(irradiance_cubemap_tex, N).rgb;
                // vec3 radiance = radiance(N, V, roughness);
                vec3 radiance = textureLod(prefilter_cubemap_tex, R,  roughness * prefilter_nr_mipmaps).rgb;
                // vec3 diffuse      = irradiance * albedo;

                // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
                // const float MAX_REFLECTION_LOD = 4.0;
                // const float MAX_REFLECTION_LOD = ;
                // vec3 prefilteredColor = textureLod(prefilter_cubemap_tex, R,  roughness * prefilter_nr_mipmaps).rgb;
                // Specular single scattering contribution (preintegrated).
                vec2 brdf  = texture(brdf_lut_tex, vec2(NdotV, roughness)).rg;
                // vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
                vec3 specular = (brdf.x * Fs + brdf.y);


                // ambient = (kD * diffuse) * ao;
                // ambient = (kD * diffuse + specular) * ao;

                // Account for multiple scattering.
                // Based on A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, C. J. Fdez-Agüera, JCGT, 2019.
                float scatter = (1.0 - (brdf.x + brdf.y));
                vec3 Favg = F0 + (1.0 - F0) / 21.0;
                vec3 multi = scatter * specular * Favg / (1.0 - Favg * scatter);
                // Diffuse contribution. Metallic materials have no diffuse contribution.
                vec3 single = (1.0 - metalness) * albedo * (1.0 - F0);
                vec3 diffuse = single * (1.0 - specular - multi) + multi;

                // fragColor = ao * (diffuse * irradiance + specular * radiance);
                ambient=ao * (diffuse * irradiance + specular * radiance);
                // ambient=(diffuse * irradiance + specular * radiance);




            }else{
                ambient = vec3(ambient_color_power) * ambient_color * ao;
            }


            color = vec3(ambient_color_power) * ambient + Lo;
            

        }
    }

    // if(depth==0.0){
    //     color=vec3(1.0, 0.0, 0.0);
    // }else{
    //     color=vec3(0.0, 1.0, 0.0);
    // }


    if(enable_bloom){
        float bloom_weight=compute_bloom_weight(color);
        if(bloom_weight>0.0){
            bloom_color=vec4(color, bloom_weight);
        }else{
            bloom_color=vec4(0.0);
        }
    }

    out_color = vec4(color, pixel_weight);
    // out_color = vec4(color, 1.0-shadow_factor_total);
    // out_color = vec4(vec3(shadow_factor_total), 1.0);



}
