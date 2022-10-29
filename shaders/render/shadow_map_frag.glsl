#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


layout(location = 0) out vec2 depth_out;

void main(){

    gl_FragDepth=gl_FragCoord.z;

    float depth=gl_FragCoord.z;

    float dx=dFdx(depth);
    float dy=dFdy(depth);
    float moment2 = depth*depth + 0.25 * (dx*dx+dy*dy);

    depth_out=vec2(depth, moment2);

}
