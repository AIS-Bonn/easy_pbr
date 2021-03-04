#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec3 normal_out;
layout(location = 1) out vec3 metalness_and_roughness_out;
layout(location = 2) out float depth_out;

uniform sampler2D normals_encoded_tex;
uniform sampler2D metalness_and_roughness_tex;
uniform sampler2D depth_tex;


//decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
// vec3 decode_normal(vec2 normal){
//     //comment this out because the if condition actually costs us 1ms per frame. And it's not relevant usually as for point cloud which dont have normals we will use only EDL
//     // if(!has_normals ){ //if we got a normal that is zero like the one we would get from a point cloud without normals, then we just output a zero to indicate no lighting needs to be done
//         // return vec3(0);
//     // }
//     vec3 normal_decoded;
//     normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
//     normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
//     return normal_decoded;

// }

//encode as xyz https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 decode_normal(vec3 normal){
    return normalize(normal * 2.0 - 1.0);
}

void main(){

    //normal
    vec3 normal_encoded=texture(normals_encoded_tex, uv_in).xyz;
    vec3 normal_decoded=decode_normal(normal_encoded); 
    normal_decoded= (normal_decoded+1.0)*0.5;
    vec2 metalnes_and_roughness=texture(metalness_and_roughness_tex, uv_in).xy;
    float depth = texture(depth_tex, uv_in).x;

    normal_out = normal_decoded;
    metalness_and_roughness_out = vec3(metalnes_and_roughness, 0.0);
    depth_out = depth;

    //debug just put ones
    // normal_out = vec3(1.0);
    // metalness_and_roughness_out = vec3(1.0);
    // depth_out = 1.0;

}