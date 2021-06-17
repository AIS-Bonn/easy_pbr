#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require



//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D color_with_transparency_tex;

uniform vec3 background_color;
// uniform bool show_background_img;
// uniform bool show_environment_map;
// uniform bool show_prefiltered_environment_map;




void main(){

    vec4 color = texture(color_with_transparency_tex, uv_in);
    float weight = color.w;
    color.rgb = mix(background_color, color.rgb, weight);

    out_color=vec4(color.xyz, 1.0);

}
