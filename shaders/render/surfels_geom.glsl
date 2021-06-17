#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//in
layout(location = 0) in vec3 v_pos_in[];
layout(location = 1) in vec3 v_normal_in[];
layout(location = 2) in vec3 tangent_u_in[];
layout(location = 3) in float length_v_in[];
layout(location = 4) in vec3 color_per_vertex_in[];

//out
layout(location=0) out vec3 position_eye_out;
layout(location=1) out vec3 normal_out;
layout(location=2) out vec2 tex_coord;
layout(location=3) out vec3 color_per_vertex_out;


//uniforms
uniform mat4 M;
uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
uniform mat4 MVP;


void main()
{

    //make the quad
    //get 2 vectors in the tangent plane
    //first one is the cross product between the normal and any other vector (just ensure it's not paralel to it)
    // vec3 random_vec=vec3(v_normal_in[0].y, v_normal_in[0].x, v_normal_in[0].z); //to ensure it's not paralel we just shuffle the numbers in the vec3
    // vec3 u=normalize(cross(v_normal_in[0],random_vec ));
    // vec3 v=normalize(cross(v_normal_in[0], u));

    //attempt 2 by getting the u and v directly
    vec3 u= tangent_u_in[0]; //this is ALREADY scaled by the length
    vec3 v=  cross(v_normal_in[0], normalize(u) ) * length_v_in[0];  // this is also already scaled by the length



    vec3 pos_quad_corner;


    pos_quad_corner=v_pos_in[0] + u;
    tex_coord = vec2(-1.0, -1.0);
    gl_Position = MVP * vec4(pos_quad_corner ,1.0);
    position_eye_out = vec3 (MV * vec4 (pos_quad_corner , 1.0));
    normal_out=normalize(vec3(M*vec4(v_normal_in[0],0.0)));
    color_per_vertex_out=color_per_vertex_in[0];
    EmitVertex();


    pos_quad_corner=v_pos_in[0] + v;
    tex_coord = vec2(1.0, -1.0);
    gl_Position = MVP * vec4(pos_quad_corner ,1.0);
    position_eye_out = vec3 (MV * vec4 (pos_quad_corner , 1.0));
    normal_out=normalize(vec3(M*vec4(v_normal_in[0],0.0)));
    color_per_vertex_out=color_per_vertex_in[0];
    EmitVertex();


    pos_quad_corner=v_pos_in[0] - v;
    tex_coord = vec2(-1.0, 1.0);
    gl_Position = MVP * vec4(pos_quad_corner ,1.0);
    position_eye_out = vec3 (MV * vec4 (pos_quad_corner , 1.0));
    normal_out=normalize(vec3(M*vec4(v_normal_in[0],0.0)));
    color_per_vertex_out=color_per_vertex_in[0];
    EmitVertex();


    pos_quad_corner=v_pos_in[0] - u;
    tex_coord = vec2(1.0, 1.0);
    gl_Position = MVP * vec4(pos_quad_corner ,1.0);
    position_eye_out = vec3 (MV * vec4 (pos_quad_corner , 1.0));
    normal_out=normalize(vec3(M*vec4(v_normal_in[0],0.0)));
    color_per_vertex_out=color_per_vertex_in[0];
    EmitVertex();
    EndPrimitive();


}
