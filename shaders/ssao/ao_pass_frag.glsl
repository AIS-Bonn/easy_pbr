#version 430 core

//in
layout(location=1) in vec2 uv_in;
layout(location=2) in vec3 view_ray_in;

//out
layout(location = 0) out vec4 ao_out;


//uniforms
uniform sampler2D normal_cam_coords_tex;
uniform sampler2D depth_tex;
uniform sampler2D rvec_tex;
uniform float z_near;
uniform float z_far;
uniform mat4 P;
uniform mat4 P_inv;
const int MAX_NR_SAMPLES=256;
uniform int nr_samples;
uniform vec3 random_samples[MAX_NR_SAMPLES];
uniform float kernel_radius;
uniform int pyr_lvl;



float linear_depth(float depth_sample)
{
    depth_sample = 2.0 * depth_sample - 1.0;
    float z_linear = 2.0 * z_near * z_far / (z_far + z_near - depth_sample * (z_far - z_near));
    return z_linear;
}

vec3 position_cam_coords_from_depth(float depth){
    float linear_depth=linear_depth(depth);
    vec3 position_cam_coords;
    position_cam_coords= normalize(view_ray_in) * linear_depth;
    return position_cam_coords;
}


float random(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax);  //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}

//decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
vec3 decode_normal(vec2 normal){
    vec3 normal_decoded;
    normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
    normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
    normal_decoded=normalize(normal_decoded);
    return normal_decoded;


    // vec3 normal_decoded;
    // normal_decoded.z=length(normal.xy);
    // normal_decoded.xy=normal.xy;
    // normal_decoded=normalize(normal_decoded);
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



void main() {


    // float depth=texture(depth_tex, uv_in).x;
    vec4 depth_v4=textureLod(depth_tex, uv_in, pyr_lvl);
    // float depth=depth_v4.x/depth_v4.w;
    float depth=depth_v4.x;
    if(depth==1.0){
        discard;
    }


    // //debug why the gbuffer is not updating
    // ao_out=vec4(1.0);

    vec2 normal_encoded=texture(normal_cam_coords_tex, uv_in).xy;
    if(normal_encoded==vec2(0)){ //we have something like a point cloud without normals. so we just it to everything visible
        ao_out=vec4(1.0);
        return;
    }
    // vec3 normal_encoded=textureLod(normal_cam_coords_tex, uv_in, pyr_lvl).xyz;
    vec3 normal=decode_normal(normal_encoded);





    //get position in cam coordinates
    vec4 position_cam_coords;
    position_cam_coords.xyz= position_cam_coords_from_depth(depth); 
    vec3 origin=position_cam_coords.xyz;
    // ao_out=vec4(position_cam_coords.x, 1.0, 1.0, 1.0);
    // return;


;

    // ao_out=vec4(normal.x, 1.0, 1.0, 1.0);
    // return;

    //if the normal is pointing awa from the camera it means we are viewing the face from behind (in the case we have culling turned off). We have to flip the normal 
    //we take the positiuon of the fragment in cam coords and get the normalized vector so this is the direction from cam to the fragment. All normals dot producted with this direction vector should be negative (pointing towards the camera)
    vec3 dir = normalize(origin);
    if(dot(normal, dir)>0.0 ){
        normal=-normal;
    }


    ivec2 img_size=textureSize( depth_tex, pyr_lvl);
    // ivec2 img_size=textureSize( depth_tex, 0);

    vec2 noise_scale=vec2(img_size)/vec2( textureSize( rvec_tex, 0) );
    vec3 rvec = texture(rvec_tex, uv_in * noise_scale).xyz; //scaling the uv coords will make the random vecs tile over the screen in tiles of 4x4 (if the texture is 4x4 of course)
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 rot = mat3(tangent, bitangent, normal);


    float nr_times_visible = 0.0;
    int nr_valid_samples=0;
    for (int i = 0; i < nr_samples; ++i) {
        // get sample position:
        vec3 sample_point = rot * random_samples[i];
        sample_point = sample_point * kernel_radius + origin;

        // project sample position:
        vec4 offset = vec4(sample_point, 1.0);
        offset = P * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        //if offset is too far from the center discard it or the sampling of the texture at such a ig distance will destroy cache coherency 
        vec4 origin_proj=P*vec4(origin,1.0);
        origin_proj.xy/=origin_proj.w;
        origin_proj.xy = origin_proj.xy * 0.5 + 0.5;
        float max_pixel_distance=0.08; //it's not actually pixels because the image space is already normalized in [0,1]
        if(length(offset.xy-origin_proj.xy)> max_pixel_distance ){
            continue;
        } 
        
        // get sample depth:
        float sample_depth=textureLod(depth_tex, offset.xy, pyr_lvl).x;
        float sample_z = position_cam_coords_from_depth( sample_depth ).z ;
        // float sampleDepth = texelFetch(position_cam_coords_tex, ivec2(offset.xy*img_size ), pyr_lvl  ).z; //A lot faster than the texture version which may also do interpolation 
        
        // bool is_sample_within_radius=abs(origin.z - sampleDepth) < kernel_radius; 
        bool is_sample_within_radius=abs(origin.z - sample_z) < kernel_radius; 
        if(is_sample_within_radius){ //only consider occlusion for the samples that are actually withing radius, some will project to very far away objects or even the background and should not be considered
            // nr_times_visible += (sampleDepth <= sample_point.z  ? 1.0 : 0.0); //https://learnopengl.com/Advanced-Lighting/SSAO
            nr_times_visible += (sample_z <= sample_point.z  ? 1.0 : 0.0); //https://learnopengl.com/Advanced-Lighting/SSAO
            nr_valid_samples++;
        }
        // nr_times_visible += (sampleDepth <= sample_point.z  ? 1.0 : 0.0) * int(is_sample_within_radius) ; 
        // nr_valid_samples += 1 * int(is_sample_within_radius);

    }

    nr_times_visible =  (nr_times_visible / nr_valid_samples);
    nr_times_visible = clamp(nr_times_visible, 0.0, 1.0); //TODO why are some valued negative? it seems that if i dont clamp I end up with black spots in my ao map whiich when blurred become even bigger so that would indicate that they are negative
    // occlusion = pow(occlusion,1); //dont do the pow here because it will push values further apart and make blurring more difficult 


    float confidence=float(nr_valid_samples)/nr_samples; //samples which are near the border of the mesh will have no valid samples because all of them will fall on the background. These low confidence points will be put to fully visible
    // confidence=confidence*pos.w; //because some positions are a blend with the background
    nr_times_visible=mix(1.0, nr_times_visible, confidence); //low confidencewill go to fully visible, high confidence will retain whaterver occlusion it had before


    // imageStore(ao_img, img_coords, vec4(nr_times_visible, 0.0, 0.0, 1.0) );

    ao_out=vec4(nr_times_visible, 0.0, 1.0, 1.0);
    // ao_out=vec4(1.0);


    //debug 
    // ao_out=vec4(origin, 1.0);
 

}



