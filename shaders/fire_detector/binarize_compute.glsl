#version 430

layout (local_size_x = 16, local_size_y = 16) in;

uniform sampler2D input_tex;
layout(r8) uniform writeonly image2D binary_tex;

//uniform 
uniform float threshold;

void main(void) {

    ivec2 img_coords = ivec2(gl_GlobalInvocationID.xy);

    //grab the input color
    ivec2 img_size=textureSize( input_tex, 0);
    vec4 input_color=texture(input_tex, vec2( float(img_coords.x+0.5)/img_size.x, float(img_coords.y+0.5)/img_size.y) );


    //threshold the first (and only) channel
    if(input_color.x>threshold){
        imageStore(binary_tex, img_coords , vec4(1.0, 1.0, 1.0, 1.0) );
    }else{
        imageStore(binary_tex, img_coords , vec4(0.0, 0.0, 0.0, 1.0) );
    }


}