#version 430 core

//in
layout(location=1) in vec2 uv_in;
layout(location=2) in vec3 view_ray_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D normal_cam_coords_tex;
uniform sampler2D position_cam_coords_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D metalness_and_roughness_tex;
uniform sampler2D log_depth_tex;
// uniform sampler2D specular_tex;
// uniform sampler2D shininess_tex;
uniform sampler2D ao_tex;
uniform sampler2D depth_tex;
uniform sampler2D background_tex;

//uniform
// uniform mat4 P_inv;
uniform mat4 V_inv; //project from pos_cam_coords back to world coordinates
uniform mat4 V; //from projecting a light position from world into the main camera 
uniform int color_type;
uniform vec3 solid_color;
uniform vec3 ambient_color;
uniform float ambient_color_power;
uniform vec3 specular_color;
uniform float shininess;
uniform sampler2D ao_img;
uniform float ssao_subsample_factor;
uniform vec2 viewport_size;
uniform bool enable_ssao;
uniform float shading_factor;
uniform float light_factor;
uniform bool enable_edl_lighting;
uniform float edl_strength;
uniform bool use_background_img;
uniform float z_near;
uniform float z_far;

//for edl 
const int neighbour_count=8;
uniform vec2 neighbours[neighbour_count];

//support for maximum 8 lights 
struct SpotLight {
//    vec4 position; // the position is in xyz,  where the w component will be 1 for omilights and 0 for directional lights as per  https://www.tomdalling.com/blog/modern-opengl/08-even-more-lighting-directional-lights-spotlights-multiple-lights/
    vec3 pos;
    // vec3 lookat_cam_coords;
    vec3 color; //the color of the light
    float power; // how much strenght does the light have
    mat4 VP; //projects world coordinates into the light 
    sampler2D shadow_map;
};
uniform SpotLight spot_lights[8];
// uniform Light omni_lights[8]; //At the moment I drop support for omni light at least partially until I have a class that can draw shadow maps into a omni light
// uniform sampler2D spot_light_shadow_maps[8];
uniform int nr_active_spot_lights;






float linear_depth(float depth_sample)
{
    // depth_sample = 2.0 * depth_sample - 1.0;
    // float z_linear = 2.0 * z_near * z_far / (z_far + z_near - depth_sample * (z_far - z_near));
    // return z_linear;

    // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    float ProjectionA = z_far / (z_far - z_near);
    float ProjectionB = (-z_far * z_near) / (z_far - z_near);
    float linearDepth = ProjectionB / (depth_sample - ProjectionA);
    return linearDepth;

}

//decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
vec3 decode_normal(vec2 normal){
    if(normal==vec2(0) ){ //if we got a normal that is zero like the one we would get from a point cloud without normals, then we just output a zero to indicate no lighting needs to be done
        return vec3(0);
    }
    vec3 normal_decoded;
    normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
    normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
    return normal_decoded;

    // vec3 normal_decoded;
    // // normal_decoded.z=length(normal.xy)*2.0-1.0;
    // normal_decoded.z=length(normal.xy);
    // normal_decoded.xy=normal.xy/normal_decoded.z;
    // // normal_decoded.xy=normal.xy;
    // // normal_decoded=normalize(normal_decoded);
    // return normal_decoded;
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



// vec3 apply_light(Light light, vec3 diffuse_color, vec3 pos_cam_coords, vec3 normal_cam_coords, vec3 specular_color, float shininess){
//     // return vec3(0);


//     //weight the diffuse_color contrubtion by the dot product between light_vector and normal
//     vec3 dir_to_light;
//     if(light.position.w == 1.0 ){ //its an omni light
//         vec3 vec_pos_to_light = vec3(light.position) - pos_cam_coords;
//         dir_to_light = normalize (vec_pos_to_light);
//     }else{ //its a directional lgith
//         dir_to_light=normalize(vec3(light.position));
//     }
//     float dot_prod = dot (dir_to_light, normal_cam_coords );
//     float clamped_dot_prod = max (dot_prod, 0.0);
//     vec3  diffuse_lit = diffuse_color * clamped_dot_prod * light.color* light.power ;    // Diffuse intensity

//     vec3 reflection_eye = reflect (-dir_to_light, normalize(normal_cam_coords)); //reflect the light *coming in* about the normal vector
//     vec3 surface_to_viewer_eye = normalize (-pos_cam_coords);
//     float dot_prod_specular = dot (reflection_eye, surface_to_viewer_eye);
//     dot_prod_specular = float(abs(dot_prod)==dot_prod) * max (dot_prod_specular, 0.0);
//     float specular_factor = pow (dot_prod_specular, shininess);
//     vec3 specular_lit = specular_color * specular_factor * light.color * light.power;    // specular intensity

//     return diffuse_lit+specular_lit;
//     // return diffuse_lit;
// }


vec3 apply_spot_light(SpotLight light, vec3 diffuse_color, vec3 pos_cam_coords, vec3 normal_cam_coords, vec3 specular_color, float shininess){
    // return vec3(0);

    //check if the current fragment in in the fov of the light. 
    //for this we get the fragment into world coordinates and then we project it into the light
    vec3 pos_world=vec3(V_inv*vec4(pos_cam_coords,1.0));
    vec4 pos_light_space=light.VP * vec4(pos_world, 1.0);

    // pos_light_space=pos_light_space*0.5+0.5;
    // perform perspective divide https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    vec3 proj_in_light = pos_light_space.xyz / pos_light_space.w;
    // transform to [0,1] range because the proj_in_light is now in normalized device coordinates [-1,1] and we want it in [0,1] https://community.khronos.org/t/shadow-mapping-bias-before-the-w-divide/63877/6
    proj_in_light = proj_in_light * 0.5 + 0.5;
    //check if we are outside the image or behind it
    if (pos_light_space.w <= 0.0f || (proj_in_light.x < 0 || proj_in_light.y < 0) || (proj_in_light.x > 1 || proj_in_light.y > 1)) { 
        return vec3(0); //do not add any light
    }


    //shadow check similar to https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
    float closest_depth = texture(light.shadow_map, proj_in_light.xy).x;
    float current_depth = proj_in_light.z;  
    float epsilon = 0.000001;
    // if (closest_depth < current_depth){
    if (closest_depth + epsilon < current_depth){
        // return vec3(closest_depth);
        // return vec3(current_depth);
        return vec3(0.0);
        // return vec3(0.0, 0.4, 0.6)*0.1;
    }
    

    // return vec3(1.0); //debug
    // return vec3(proj_in_light)*100000; //debug
    // return vec3(pos_cam_coords)*10000000; //debug
    // return vec3(diffuse_color); //debug
    // return vec3(normal_cam_coords); //debug



    //weight the diffuse_color contrubtion by the dot product between light_vector and normal
    vec3 dir_to_light;
    vec3 vec_pos_to_light = vec3(V*vec4(light.pos, 1.0)) - pos_cam_coords;
    dir_to_light = normalize (vec_pos_to_light);

    float dot_prod = dot (dir_to_light, normal_cam_coords );
    float clamped_dot_prod = max (dot_prod, 0.0);
    vec3  diffuse_lit = diffuse_color * clamped_dot_prod * light.color* light.power ;    // Diffuse intensity

    vec3 reflection_eye = reflect (-dir_to_light, normalize(normal_cam_coords)); //reflect the light *coming in* about the normal vector
    vec3 surface_to_viewer_eye = normalize (-pos_cam_coords);
    float dot_prod_specular = dot (reflection_eye, surface_to_viewer_eye);
    dot_prod_specular = float(abs(dot_prod)==dot_prod) * max (dot_prod_specular, 0.0);
    float specular_factor = pow (dot_prod_specular, shininess);
    vec3 specular_lit = specular_color * specular_factor * light.color * light.power;    // specular intensity

    return diffuse_lit+specular_lit;
    return diffuse_lit;
}


// float z_far=50000;
// float z_near=0.01;



void main(){

    // vec4 pos_cam_coords=texture(position_cam_coords_tex, uv_in);
    vec4 pos_cam_coords;
    // if(pos_cam_coords.w==0.0){
    //     discard; // is a pixel not covered by a mesh so we discard it and therefore we will see the background color that was used in glclearColor
    // }

    //calculate the pos_cam_coords from the depth buffer like said at the bottom of this article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    float ProjectionA = z_far / (z_far - z_near);
    float ProjectionB = (-z_far * z_near) / (z_far - z_near);
    // Sample the depth and convert to linear view space Z (assume it gets sampled as
    // a floating point value of the range [0,1])
    // float depth = DepthTexture.Sample(PointSampler, texCoord).x;
    // float linearDepth = ProjectionB / (depth - ProjectionA);
    float depth=texture(depth_tex, uv_in).x;
    if(depth==1.0){
        //there is no mesh or anything covering this pixel, we discard it so the pixel will show whtever the background was set to
        if (use_background_img){
            out_color=texture(background_tex, uv_in);
            return;
        }else{
            discard;
        }
    }
    // depth= depth*2.0 -1.0; //transform to depth to the NDC in the range [-1,1] https://stackoverflow.com/a/16597492
    // depth=depth*2.0;
    // float linearDepth = ProjectionB / (depth - ProjectionA);
    // float linearDepth=linear_depth(depth);
    // vec3 view_ray = vec3(pos_cam_coords.xy / pos_cam_coords.z, 1.0f);
    // pos_cam_coords.xyz= view_ray* linearDepth;
    // pos_cam_coords.xyz= normalize(view_ray_in) * linearDepth;
    // pos_cam_coords.xyz= view_ray_in * linearDepth;
    // pos_cam_coords.xyz= normalize(view_ray_in) * depth;
    // pos_cam_coords.xyz= view_ray_in * depth;

    pos_cam_coords.xyz= position_cam_coords_from_depth(depth); 

    //show
    // out_color=vec4(pos_cam_coords.xy, 0.0, 1.0);
    // out_color=vec4(pos_cam_coords.xyz, 1.0);
    // out_color=vec4(view_ray_in, 1.0);
    // out_color=vec4(normalize(view_ray_in).xy, 0.0, 1.0);
    // out_color=vec4(vec3(linearDepth), 1.0);
    // out_color=vec4(vec3(depth), 1.0);
    // out_color=vec4(vec3(linear_depth(depth))*0.01, 1.0);
    // out_color=vec4(vec3(1.0, 0.0, 0.0), 1.0);
    // return;



    /////START DEBUG HERE

    vec4 diffuse_color=texture(diffuse_tex, uv_in);
    //recover normals as done in cryengine 3 presentation "a bit more defferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    vec2 normal_gbuf=texture(normal_cam_coords_tex, uv_in).xy;
    vec4 normal_cam_coords = vec4( decode_normal(normal_gbuf), 1.0);
    
    

    float alpha=diffuse_color.w; //an alpha between 0 and 1 will leave the color unchanged, but if it's above 1 it means we have to normalize because we have contributions from more than one pixel, for example when splatting the alpha is interpreted as a confidence

    vec3 shade=vec3(0);

    if(normal_cam_coords.xyz!=vec3(0)){ //if we have normals we apply shading othersiw we don;t 
     
        normal_cam_coords.xyz/=alpha;
        pos_cam_coords.xyz/=alpha;
        diffuse_color.xyz/=alpha;
        normal_cam_coords.xyz=normalize(normal_cam_coords.xyz);


        for(int i=0; i<nr_active_spot_lights; i++){
            // if(i==0)
            shade+=apply_spot_light(spot_lights[i], diffuse_color.xyz, pos_cam_coords.xyz, normal_cam_coords.xyz, specular_color.xyz, shininess);
        }
    }else{
        shade= diffuse_color.xyz;
    }

    

    //get ao attempt 2
    float ao_term;
    if(enable_ssao){
        ao_term=texture(ao_tex, uv_in).x;
    }else {
        ao_term=1.0;
    }

 

    vec4 color=vec4(shade*ao_term,1.0)+ vec4(ambient_color*ambient_color_power, 1.0);
    vec4 color_without_light=vec4(diffuse_color.xyz*ao_term,1.0) + vec4(ambient_color*ambient_color_power, 1.0);
    color=mix(color_without_light, color, light_factor); //remove the color and leave only the diffuse and ao if necesarry
    color=mix(diffuse_color, color, shading_factor); //remove even the ao term and leaves only diffuse

    //if the diffuse_color is black we output the ao term 
    if(alpha==0.0){
        color=vec4(vec3(1)*ao_term,1.0);
    }


    //edl lighting https://github.com/potree/potree/blob/65f6eb19ce7a34ce588973c262b2c3558b0f4e60/src/materials/shaders/edl.fs
    if(enable_edl_lighting){
        // float depth = texture(log_depth_tex, uv_in).x;
        depth=linear_depth(depth);
        depth=log2(depth);
        if(depth!=1.0){ //if we have a depth of 1.0 it means for this pixels we haven't stored anything. Storing something in this texture is only done for points
            depth = (depth == 1.0) ? 0.0 : depth;
            float res = response(depth);
            // float edl_strength=16.0;
            float shade = exp(-res * 300.0 * edl_strength);
            color = vec4(diffuse_color.xyz * shade, 1.0);
        }
        
    }



    out_color=color;


    //debug 
    // vec3 diff=position_cam_coords_from_depth(depth) - pos_cam_coords.xyz;
    // float dot_val=dot(diff, diff); 
    // out_color=vec4(vec3(dot_val),1.0);
   
}