#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
in vec3 position;
in vec3 normal;

//out
layout(location = 0) out vec3 v_pos_out; //position in world coords
layout(location = 1) out vec3 v_normal_out;


//uniforms
// uniform mat4 M; //model matrix which moves from an object centric frame into the world frame. If the mesh is already in the world frame, then this will be an identity
// uniform mat4 MV; //project only into camera coordinate, but doesnt project onto the screen
// uniform mat4 MVP;

void main(){

    v_pos_out=position;
    v_normal_out=normal;


}
