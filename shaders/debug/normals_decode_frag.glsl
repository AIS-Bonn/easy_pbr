#version 430 core

//in
layout(location=1) in vec2 uv_in;

//out
layout(location = 0) out vec4 out_color;

uniform sampler2D normals_encoded_tex;


//decode a normal stored in RG texture as explained in the CryEngine 3 talk "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
vec3 decode_normal(vec2 normal){
    //comment this out because the if condition actually costs us 1ms per frame. And it's not relevant usually as for point cloud which dont have normals we will use only EDL
    // if(!has_normals ){ //if we got a normal that is zero like the one we would get from a point cloud without normals, then we just output a zero to indicate no lighting needs to be done
        // return vec3(0);
    // }
    vec3 normal_decoded;
    normal_decoded.z=dot(normal.xy, normal.xy)*2-1;
    normal_decoded.xy=normalize(normal.xy)*sqrt(1-normal_decoded.z*normal_decoded.z);
    return normal_decoded;

}

void main(){

    vec2 normal_encoded=texture(normals_encoded_tex, uv_in).xy;

    vec3 normal_decoded=decode_normal(normal_encoded); 
    normal_decoded= (normal_decoded+1.0)*0.5;


    out_color=vec4(normal_decoded, 0.0);

}