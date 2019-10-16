#version 430 core

//in
layout(location=1) in vec2 uv_in;
layout(location=2) in vec3 view_ray_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D normal_tex;
// uniform sampler2D position_cam_coords_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D metalness_and_roughness_tex;
uniform sampler2D log_depth_tex;
uniform sampler2D ao_tex;
uniform sampler2D depth_tex;
uniform sampler2D background_tex;

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
uniform float shading_factor;
uniform float light_factor;
uniform bool enable_edl_lighting;
uniform float edl_strength;
uniform bool use_background_img;
uniform float projection_a; //for calculating position from depth according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
uniform float projection_b;


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
};
uniform SpotLight spot_lights[8];
// uniform Light omni_lights[8]; //At the moment I drop support for omni light at least partially until I have a class that can draw shadow maps into a omni light
uniform int nr_active_spot_lights;


const float PI = 3.14159265359;



float linear_depth(float depth_sample)
{
    // depth_sample = 2.0 * depth_sample - 1.0;
    // float z_linear = 2.0 * z_near * z_far / (z_far + z_near - depth_sample * (z_far - z_near));
    // return z_linear;

    // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    // float ProjectionA = z_far / (z_far - z_near);
    // float ProjectionB = (-z_far * z_near) / (z_far - z_near);
    float linearDepth = projection_b / (depth_sample - projection_a);
    return linearDepth;

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
float response(float depth){
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


const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}



void main(){

    //PBR again========= https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/1.1.lighting/1.1.pbr.fs
    float depth=texture(depth_tex, uv_in).x;
    if(depth==1.0){
        //there is no mesh or anything covering this pixel, we discard it so the pixel will show whtever the background was set to
        if (use_background_img){
            // out_color=texture(background_tex, uv_in);
            vec2 uv = SampleSphericalMap(normalize(view_ray_in)); // make sure to normalize localPos
            vec3 color = texture(background_tex, uv).xyz;
            //tonemap
            // color = color / (color + vec3(1.0));
            // gamma correct
            color = pow(color, vec3(1.0/2.2)); 
            out_color = vec4(color, 1.0);
            return;
        }else{
            discard;
        }
    }
    vec3 albedo=pow( texture(diffuse_tex, uv_in).xyz, vec3(2.2) );

    // //edl lighting https://github.com/potree/potree/blob/65f6eb19ce7a34ce588973c262b2c3558b0f4e60/src/materials/shaders/edl.fs
    vec3 color=vec3(0);
    if(enable_edl_lighting){
        // float depth = texture(log_depth_tex, uv_in).x;
        depth=linear_depth(depth);
        depth=log2(depth);
        if(depth!=1.0){ //if we have a depth of 1.0 it means for this pixels we haven't stored anything. Storing something in this texture is only done for points
            depth = (depth == 1.0) ? 0.0 : depth;
            float res = response(depth);
            // float edl_strength=16.0;
            float shade = exp(-res * 300.0 * edl_strength);
            color = albedo* shade;
        }
    }else{
        //PBR-----------

        vec3 N= decode_normal(texture(normal_tex, uv_in).xy ); 
        vec3 P_c = position_cam_coords_from_depth(depth); //position of the fragment in camera coordinates
        vec3 P_w = vec3(V_inv*vec4(P_c,1.0));
        vec3 V = normalize( eye_pos -P_w ); //view vector that goes from position of the fragment towards the camera
        float metalness=texture(metalness_and_roughness_tex, uv_in).x;
        float roughness=texture(metalness_and_roughness_tex, uv_in).y;
        float ao= enable_ssao? texture(ao_tex, uv_in).x : 1.0; 



        // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
        // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metalness);

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
                continue;
            }


            //shadow check similar to https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
            float closest_depth = texture(spot_lights[i].shadow_map, proj_in_light.xy).x;
            float current_depth = proj_in_light.z;  
            float epsilon = 0.000001;
            if (closest_depth + epsilon < current_depth){
                continue;
            }
            

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
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

        }   
        
        // ambient lighting (note that the next IBL tutorial will replace 
        // this ambient lighting with environment lighting).
        vec3 ambient = vec3(ambient_color_power) * ambient_color * ao;

        color = ambient + Lo;

    }


    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    

    out_color = vec4(color, 1.0);
   
}