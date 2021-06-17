#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
layout(location=0) in vec3 world_view_ray_in;

//out
layout(location = 0) out vec4 out_color;

uniform samplerCube radiance_cubemap_tex;

//uniform

const float PI = 3.14159265359;


void main(){

    // The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    vec3 N=normalize(world_view_ray_in);

    vec3 irradiance = vec3(0.0);

    // vec3 color = texture(radiance_cubemap_tex, normal ).rgb;
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, N);
    up            = cross(N, right);

    float sampleDelta = 0.02;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(radiance_cubemap_tex, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));


    out_color = vec4(irradiance, 1.0);


}
