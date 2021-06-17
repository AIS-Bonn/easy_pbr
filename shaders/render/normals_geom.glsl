#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout(points) in;
layout(line_strip, max_vertices = 2) out;

//in
layout(location = 0) in vec3 v_pos_in[];
layout(location = 1) in vec3 v_normal_in[];

//out
// layout(location=0) out vec3 out_color;


//uniforms
uniform mat4 M;
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;
uniform float normals_scale;


void main()
{

    //make a line with an origin and a destination


    vec3 pos_origin=v_pos_in[0];
    gl_Position = MVP * vec4(pos_origin ,1.0);
    EmitVertex();

    vec3 pos_dest=v_pos_in[0] + v_normal_in[0]*normals_scale;
    gl_Position = MVP * vec4(pos_dest ,1.0);
    EmitVertex();



    EndPrimitive();


}
