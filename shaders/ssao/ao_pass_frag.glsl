#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
layout(location=1) in vec2 uv_in;
layout(location=2) in vec3 view_ray_in;

//out
layout(location = 0) out vec4 ao_out;


//uniforms
uniform sampler2D normal_tex;
uniform sampler2D depth_tex;
// uniform sampler2D depth_linear_tex;
uniform sampler2D rvec_tex;
uniform float projection_a; //for calculating position from depth according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
uniform float projection_b;
uniform mat4 P;
uniform mat4 P_inv;
uniform mat3 V_rot;
const int MAX_NR_SAMPLES=256;
uniform int nr_samples;
uniform vec3 random_samples[MAX_NR_SAMPLES];
uniform float kernel_radius;
uniform int pyr_lvl;
uniform bool using_fat_gbuffer;
uniform bool ssao_estimate_normals_from_depth;
uniform float max_ssao_distance; //it's not actually pixels because the image space is already normalized in [0,1]


float linear_depth(float depth_sample){
    // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    float linearDepth = projection_b / (depth_sample - projection_a);
    return linearDepth;
}

vec3 position_cam_coords_from_depth(float depth){
    float linear_depth=linear_depth(depth);
    vec3 position_cam_coords;
    position_cam_coords= normalize(view_ray_in) * linear_depth;
    return position_cam_coords;
}

// vec3 position_cam_coords_from_linear_depth(float linear_depth){
//     vec3 position_cam_coords;
//     position_cam_coords= normalize(view_ray_in) * linear_depth;
//     return position_cam_coords;
// }


float random(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax);  //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}

// //decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
// vec3 decode_normal(vec2 normal){
//     vec3 normal_decoded;
//     normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
//     normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
//     normal_decoded=normalize(normal_decoded);
//     return normal_decoded;
// }

//encode as xyz https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 decode_normal(vec3 normal){
    if(using_fat_gbuffer){
        return normalize(normal);
    }else{
        return normalize(normal * 2.0 - 1.0);
    }
}



void main() {

    //with linear depth
    // float depth_linear=texture(depth_linear_tex, uv_in).x;
    // if(depth_linear==1.0){
    //     discard;
    // }
    //with non linear depth
    float depth=texture(depth_tex, uv_in).x;
    if(depth==1.0){
        discard;
    }


    //get position in cam coordinates
    vec4 position_cam_coords;
    // position_cam_coords.xyz= position_cam_coords_from_linear_depth(depth_linear);
    position_cam_coords.xyz= position_cam_coords_from_depth(depth);
    vec3 origin=position_cam_coords.xyz;


    vec3 normal;
    if(ssao_estimate_normals_from_depth){
        normal = normalize(cross(dFdx(position_cam_coords.xyz), dFdy(position_cam_coords.xyz)));
    }else{
        vec3 normal_encoded=texture(normal_tex, uv_in).xyz;
        if(normal_encoded==vec3(0)){ //we have something like a point cloud without normals. so we just it to everything visible
            ao_out=vec4(1.0);
            return;
        }
        normal=decode_normal(normal_encoded);
        normal=V_rot*normal; //we need the normal in cam coordinates so we have to rotate it with the rotation part of the view matrix
    }








    //if the normal is pointing awa from the camera it means we are viewing the face from behind (in the case we have culling turned off). We have to flip the normal
    //we take the positiuon of the fragment in cam coords and get the normalized vector so this is the direction from cam to the fragment. All normals dot producted with this direction vector should be negative (pointing towards the camera)
    vec3 dir = normalize(origin);
    if(dot(normal, dir)>0.0 ){
        normal=-normal;
    }


    ivec2 img_size=textureSize( normal_tex, 0); //we get the image size of the highest mipmap. This way we will tile mode the noise, having more randomness and improving the result
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
        // max_ssao_distance=0.05; //it's not actually pixels because the image space is already normalized in [0,1]
        if(length(offset.xy-origin_proj.xy)> max_ssao_distance ){
            continue;
        }

        // get sample depth:
        // float sample_depth_linear=texture(depth_linear_tex, offset.xy).x;
        // float sample_z = position_cam_coords_from_linear_depth( sample_depth_linear ).z ;
        //with depth NOT linear
        float sample_depth=texture(depth_tex, offset.xy).x;
        float sample_z = position_cam_coords_from_depth( sample_depth ).z ;

        bool is_sample_within_radius=abs(origin.z - sample_z) < kernel_radius;
        if(is_sample_within_radius){ //only consider occlusion for the samples that are actually withing radius, some will project to very far away objects or even the background and should not be considered
            nr_times_visible += (sample_z <= sample_point.z  ? 1.0 : 0.0); //https://learnopengl.com/Advanced-Lighting/SSAO
            nr_valid_samples++;
        }

    }

    nr_times_visible =  (nr_times_visible / nr_valid_samples);
    // nr_times_visible = clamp(nr_times_visible, 0.0, 1.0); //TODO why are some valued negative? it seems that if i dont clamp I end up with black spots in my ao map whiich when blurred become even bigger so that would indicate that they are negative

    float confidence=float(nr_valid_samples)/nr_samples; //samples which are near the border of the mesh will have no valid samples because all of them will fall on the background. These low confidence points will be put to fully visible
    nr_times_visible=mix(1.0, nr_times_visible, confidence); //low confidencewill go to fully visible, high confidence will retain whaterver occlusion it had before


    ao_out=vec4(nr_times_visible, 0.0, 1.0, 1.0);


}
