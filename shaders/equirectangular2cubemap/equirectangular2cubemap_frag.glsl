#version 430 core

//in
layout(location = 0) in vec3 position_ndc_in;

//out
layout(location = 0) out vec4 out_color;

//uniform
uniform sampler2D equirectangular_tex;
uniform vec3 line_color;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main(){

    vec2 uv = SampleSphericalMap(normalize(position_ndc_in)); // make sure to normalize localPos
    vec3 color = texture(equirectangular_tex, uv).xyz;

    out_color = vec4(color, 1.0);
    // out_color = vec4(1.0);
}
