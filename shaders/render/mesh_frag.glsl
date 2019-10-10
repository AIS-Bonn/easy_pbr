#version 430 core

//in
layout(location = 0) in vec3 normal_in;
layout(location = 1) in vec3 position_cam_coords_in; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
layout(location = 2) in vec3 normal_cam_coords_in; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 3) in vec3 color_per_vertex_in;
layout(location = 4) in vec2 uv_in;
layout(location = 5) in vec3 position_world_in;


//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
layout(location = 0) out vec4 position_out; 
layout(location = 1) out vec3 diffuse_out;
// layout(location = 3) out vec4 normal_out;
layout(location = 3) out vec2 normal_out;

// //uniform
uniform int color_type;
uniform sampler2D tex; //the rgb tex that is used for coloring

//encode the normal using the equation from Cry Engine 3 "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
vec2 encode_normal(vec3 normal){
    if(normal==vec3(0)){ //we got a non existant normal, like if you have a point cloud without normals so we just output a zero
        return vec2(0);
    }
    vec2 normal_encoded = normalize(normal.xy) * sqrt(normal.z*0.5+0.5);
    return normal_encoded;
}

//  //encode normals as done by david bernard in https://hub.jmonkeyengine.org/t/solved-strange-shining-problem/32962/4 and https://github.com/davidB/jme3_ext_deferred/blob/master/src/main/resources/ShaderLib/DeferredUtils.glsllib
//  // return +/- 1
// vec2 signNotZero(vec2 v) {
// 	return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
// }
//  // Assume normalized input. Output is on [-1, 1] for each component.
// vec2 float32x3_to_oct(in vec3 v) {
// 	// Project the sphere onto the octahedron, and then onto the xy plane
// 	vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
// 	// Reflect the folds of the lower hemisphere over the diagonals
// 	return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
// }
//  /* The caller should store the return value into a GL_RGB8 texture or attribute without modification. */
// vec3 snorm12x2_to_unorm8x3(vec2 f) {
// 	vec2 u = vec2(round(clamp(f, -1.0, 1.0) * 2047 + 2047));
// 	float t = floor(u.y / 256.0);
// 	// If storing to GL_RGB8UI, omit the final division
// 	return floor(vec3(u.x / 16.0, fract(u.x / 16.0) * 256.0 + t, u.y - t * 256.0)) / 255.0;
// }
//  vec3 encode_normal(vec3 normal){
//     return snorm12x2_to_unorm8x3(float32x3_to_oct(normal));
// }


void main(){

    // if(position_world_in.y>3){
    //     discard;
    // }


    if(color_type==2){ //TEXTURE
        vec4 tex_color=texture(tex, uv_in);
        // the texture sampling may mix the pixel with the background which has alpha of zero so it makes the color darker and also dimmer, if we renomalize we make it brighter again
        if(tex_color.w!=0 ){
            tex_color/=tex_color.w;
        }
        diffuse_out = vec3(tex_color.xyz);
    }else{
        diffuse_out = color_per_vertex_in; //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
    }

    // normal_out = vec4(normal_cam_coords_in, 1.0);

    //output normals as done in cryengine 3 presentation "a bit more defferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    // normal_out = normalize(normal_cam_coords_in.xy) * sqrt(normal_cam_coords_in.z*0.5+0.5);
    normal_out=encode_normal(normal_cam_coords_in);

    // position_out = vec4(position_cam_coords_in, 1.0);
}
