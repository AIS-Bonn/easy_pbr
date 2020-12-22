#version 430 core
#extension GL_ARB_explicit_attrib_location : require



//in
layout(location = 0) in vec3 position_eye_in;
layout(location = 1) in vec3 normal_in; 
layout(location = 2) in vec2 tex_coord_in; 
layout(location = 3) in vec3 color_per_vertex_in; 
// layout(location = 4) in vec2 uv_in


//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
// layout(location = 0) out vec4 position_out; 
layout(location = 1) out vec4 diffuse_out;
// layout(location = 3) out vec4 specular_out;
// layout(location = 4) out vec4 shininess_out;
layout(location = 2) out vec3 normal_out;
layout(location = 3) out vec2 metalness_and_roughness_out;

// //uniform
uniform int color_type;
uniform bool using_fat_gbuffer;
uniform vec3 solid_color;
uniform bool enable_solid_color; // whether to use solid color or color per vertex
uniform vec3 specular_color;
uniform float shininess;
uniform float metalness;
uniform float roughness;
uniform bool enable_visibility_test;
// uniform sampler2D diffuse_tex; 
// uniform sampler2D metalness_tex; 
// uniform sampler2D roughness_tex; 
// uniform sampler2D normals_tex; 
// uniform bool has_diffuse_tex; //If the texture tex actually exists and can be sampled from
// uniform bool has_metalness_tex; //If the texture tex actually exists and can be sampled from
// uniform bool has_roughness_tex; //If the texture tex actually exists and can be sampled from




float map(float value, float inMin, float inMax, float outMin, float outMax) {
    float value_clamped=clamp(value, inMin, inMax); //so the value doesn't get modified by the clamping, because glsl may pass this by referece
    return outMin + (outMax - outMin) * (value_clamped - inMin) / (inMax - inMin);
}

//smoothstep but with 1st and 2nd derivatives that go more smoothly to zero
float smootherstep( float A, float B, float X ){
//    float t = linearstep( A, B, X );
    // float t= X;
    float t = map(X, A , B, 0.0, 1.0);

   return t * t * t * ( t * ( t * 6 - 15 ) + 10 );
}

// //encode the normal using the equation from Cry Engine 3 "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
// vec2 encode_normal(vec3 normal){
//     if(normal==vec3(0)){ //we got a non existant normal, like if you have a point cloud without normals so we just output a zero
//         return vec2(0);
//     }
//     vec2 normal_encoded = normalize(normal.xy) * sqrt(normal.z*0.5+0.5);
//     return normal_encoded;
// }

//encode as xyz https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 encode_normal(vec3 normal){
    if(using_fat_gbuffer){
        return normal;
    }else{
        return normal * 0.5 + 0.5;
    }
}



void main(){

    //make a circle by discardig stuff around the border of the square patch
    float local_r=dot(tex_coord_in,tex_coord_in);
    if(local_r>1.0){
        discard;
    }


    // float surface_confidence=smootherstep(0.0, 1.0,  1.0-local_r); //decreasing confidence from the middle to the edge of the surfel

    // if(color_type==2){ //TEXTURE
    //     if(has_diffuse_tex){
    //         vec4 tex_color=texture(diffuse_tex, uv_in);
    //         diffuse_out = vec4( vec3(tex_color.xyz), 1.0);
    //     }else{
    //         diffuse_out=vec4( vec3(0.0), 1.0 );
    //     }

    //     if (has_metalness_tex){
    //         metalness_out=texture(metalness_tex, uv_in).x;
    //     }
    //     if (has_roughness_tex){
    //         roughness_out=texture(roughness_tex, uv_in).x;
    //     }
    //     // if (has_normals_tex){
    //         //nothing happens because its not implemented yey
    //         // vec3 normal_deviation=texture(normals_tex, uv_in).xyz - vec3(0.5);
    //         // normal_to_encode+=normal_deviation;
    //         // normal_to_encode=normalize(normal_to_encode);
    //     // }
        
    // }else{
    //     diffuse_out=vec4(color_per_vertex_in,1.0); //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
    // }

    //if we enable the visibility test then we just render into the depth, and nothing else. then in the second pass we render only the surfels that are visible
    if(!enable_visibility_test){
        // float surface_confidence=map(local_r, 0.0, 1.0, 1.0, 0.01); //decreasing confidence from the middle to the edge of the surfel
        float surface_confidence=smootherstep(0.0, 1.0,  1.0-local_r); //decreasing confidence from the middle to the edge of the surfel


        float metalness_out=metalness;
        float roughness_out=roughness;

        // if(color_type==2){ //TEXTURE
        //     if(has_diffuse_tex){
        //         vec4 tex_color=texture(diffuse_tex, uv_in);
        //         diffuse_out = vec4( vec3(tex_color.xyz)*surface_confidence, surface_confidence);
        //     }else{
        //         diffuse_out=vec4( vec3(0.0), 1.0 );
        //     }

        //     if (has_metalness_tex){
        //         metalness_out=texture(metalness_tex, uv_in).x;
        //     }
        //     if (has_roughness_tex){
        //         roughness_out=texture(roughness_tex, uv_in).x;
        //     }
        //     // if (has_normals_tex){
        //         //nothing happens because its not implemented yey
        //         // vec3 normal_deviation=texture(normals_tex, uv_in).xyz - vec3(0.5);
        //         // normal_to_encode+=normal_deviation;
        //         // normal_to_encode=normalize(normal_to_encode);
        //     // }
            
        // }else{
        //     diffuse_out=vec4(color_per_vertex_in*surface_confidence,surface_confidence); //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
        // }




        diffuse_out = vec4(color_per_vertex_in*surface_confidence, surface_confidence );
        normal_out = encode_normal( normal_in );
        metalness_and_roughness_out=vec2(metalness_out, roughness_out)*surface_confidence;
        // normal_out = vec4(  encode_normal( normal_eye_in*surface_confidence ), 1.0, 1.0);
        // position_out = vec4(position_eye_in*surface_confidence, 1.0);
    }

   

}