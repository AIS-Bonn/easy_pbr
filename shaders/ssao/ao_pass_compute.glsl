#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout (local_size_x = 16, local_size_y = 16) in;

// uniform sampler2D position_cam_coords_tex;
uniform sampler2D normal_cam_coords_tex;
uniform sampler2D normal_tex;
uniform sampler2D rvec_tex;
// layout(r32f) uniform writeonly image2D ao_img;
layout(rgba32f) uniform writeonly image2D ao_img;
uniform mat4 proj;

// const int MAX_KERNEL_SIZE = 128;
// uniform vec3 gKernel[MAX_KERNEL_SIZE];
// uniform float gSampleRad;

const int MAX_NR_SAMPLES=256;
uniform int nr_samples;
uniform vec3 random_samples[MAX_NR_SAMPLES];

uniform float kernel_radius;
uniform int pyr_lvl;

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
    return normal_decoded;
}

void main() {
    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    // //debug
    // imageStore(ao_img, img_coords, vec4(0.5, 0.0, 0.0, 1.0) );

    //for sampling we need it in range [0,1]
    ivec2 img_size=textureSize( position_cam_coords_tex, pyr_lvl);
    vec2 uv_coord=vec2(float(img_coords.x+0.5)/img_size.x, float(img_coords.y+0.5)/img_size.y);

    //if we are on the background then we just store a 1 in the a0 to indicate total visibility and we're done
    // vec4 pos= texelFetch(position_cam_coords_tex, img_coords, pyr_lvl);
    vec4 pos= textureLod(position_cam_coords_tex, uv_coord, pyr_lvl);
    // vec4 pos= texture(position_cam_coords_tex, uv_coord);

    // //debug
    // // imageStore(ao_img, img_coords, vec4(pos.xyz, 1.0) );
    // imageStore(ao_img, img_coords, vec4(pos.w, 0.0, 0.0, 1.0) );


    // if(pos.w==0.0){ //on the background the alpha value is 0.0, we ensured that in the geometry pass
    //     imageStore(ao_img, img_coords, vec4(1.0, 0.0, 0.0, 1.0) );
    //     return;
    // }





    vec3 origin=pos.xyz;
    vec3 normal=texture(normal_cam_coords_tex, uv_coord).xyz;

    //if the normal is pointing awa from the camera it means we are viewing the face from behind (in the case we have culling turned off). We have to flip the normal
    //we take the positiuon of the fragment in cam coords and get the normalized vector so this is the direction from cam to the fragment. All normals dot producted with this direction vector should be negative (pointing towards the camera)
    vec3 dir = normalize(origin);
    if(dot(normal, dir)>0.0 ){
        normal=-normal;
    }

    vec2 noise_scale=vec2(img_size)/vec2( textureSize( rvec_tex, 0) );
    vec3 rvec = texture(rvec_tex, uv_coord * noise_scale).xyz; //scaling the uv coords will make the random vecs tile over the screen in tiles of 4x4 (if the texture is 4x4 of course)
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
        offset = proj * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        //if offset is too far from the center discard it or the sampling of the texture at such a ig distance will destroy cache coherency
        vec4 origin_proj=proj*vec4(origin,1.0);
        origin_proj.xy/=origin_proj.w;
        origin_proj.xy = origin_proj.xy * 0.5 + 0.5;
        float max_pixel_distance=0.08; //it's not actually pixels because the image space is already normalized in [0,1]
        if(length(offset.xy-origin_proj.xy)> max_pixel_distance ){
            continue;
        }

        // get sample depth:
        float sampleDepth = texture(position_cam_coords_tex, offset.xy).z;
        // float sampleDepth = texelFetch(position_cam_coords_tex, ivec2(offset.xy*img_size ), pyr_lvl  ).z; //A lot faster than the texture version which may also do interpolation

        bool is_sample_within_radius=abs(origin.z - sampleDepth) < kernel_radius;
        if(is_sample_within_radius){ //only consider occlusion for the samples that are actually withing radius, some will project to very far away objects or even the background and should not be considered
            nr_times_visible += (sampleDepth <= sample_point.z  ? 1.0 : 0.0); //https://learnopengl.com/Advanced-Lighting/SSAO
            nr_valid_samples++;
        }
        // nr_times_visible += (sampleDepth <= sample_point.z  ? 1.0 : 0.0) * int(is_sample_within_radius) ;
        // nr_valid_samples += 1 * int(is_sample_within_radius);

    }

    nr_times_visible =  (nr_times_visible / nr_valid_samples);
    nr_times_visible = clamp(nr_times_visible, 0.0, 1.0); //TODO why are some valued negative? it seems that if i dont clamp I end up with black spots in my ao map whiich when blurred become even bigger so that would indicate that they are negative
    // occlusion = pow(occlusion,1); //dont do the pow here because it will push values further apart and make blurring more difficult


    float confidence=float(nr_valid_samples)/nr_samples; //samples which are near the border of the mesh will have no valid samples because all of them will fall on the background. These low confidence points will be put to fully visible
    confidence=confidence*pos.w; //because some positions are a blend with the background
    nr_times_visible=mix(1.0, nr_times_visible, confidence); //low confidencewill go to fully visible, high confidence will retain whaterver occlusion it had before


    imageStore(ao_img, img_coords, vec4(nr_times_visible, 0.0, 0.0, 1.0) );



}
