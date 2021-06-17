#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout (local_size_x = 16, local_size_y = 16) in;


uniform float z_near;
uniform float z_far;
uniform sampler2D depth_tex;
layout(r16f) uniform writeonly image2D depth_linear_img;



//https://stackoverflow.com/a/33465663
// depthSample from depthTexture.r, for instance
float linear_depth(float depth_sample)
{
    depth_sample = 2.0 * depth_sample - 1.0;
    float z_linear = 2.0 * z_near * z_far / (z_far + z_near - depth_sample * (z_far - z_near));
    return z_linear;
}

// result suitable for assigning to gl_FragDepth
float depth_sample(float linear_depth)
{
    float non_linear_depth = (z_far + z_near - 2.0 * z_near * z_far / linear_depth) / (z_far - z_near);
    non_linear_depth = (non_linear_depth + 1.0) / 2.0;
    return non_linear_depth;
}


void main() {

    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    //for sampling we need it in range [0,1]
    ivec2 img_size=textureSize( depth_tex, 0);
    vec2 uv_coord=vec2(float(img_coords.x)/img_size.x, float(img_coords.y)/img_size.y);

    //sample the depth and convert
    float depth_raw=texture(depth_tex, uv_coord).x;
    float depth_linear= linear_depth(depth_raw);

    //TODO we shouldt need to divide by this big number, we only do it in order to visualize the depth map. Also it shouldnt affect the result of the bilaterl blur
    imageStore(depth_linear_img, img_coords, vec4(depth_linear, 0.0, 0.0, 1.0) );


}
