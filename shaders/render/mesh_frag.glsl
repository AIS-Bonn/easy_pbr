#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require


//in
layout(location = 0) in vec3 normal_in;
layout(location = 1) in vec3 position_cam_coords_in; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
// layout(location = 2) in vec3 normal_cam_coords_in; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 2) in vec3 precomputed_ao_in;
layout(location = 3) in vec3 color_per_vertex_in;
layout(location = 4) in vec2 uv_in;
layout(location = 5) in vec3 position_world_in;
layout(location = 6) in mat3 TBN_in;



//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
layout(location = 0) out vec4 position_out;
layout(location = 1) out vec4 diffuse_out;
// layout(location = 3) out vec4 normal_out;
layout(location = 3) out vec3 normal_out;
layout(location = 4) out vec2 metalness_and_roughness_out;
layout(location = 5) out int mesh_id_out;
layout(location = 6) out vec2 uv_out;


// //uniform
uniform mat4 V;
uniform bool render_uv_to_gbuffer;
uniform int color_type;
uniform sampler2D diffuse_tex;
uniform sampler2D metalness_tex;
uniform sampler2D roughness_tex;
uniform sampler2D normals_tex;
uniform sampler2D matcap_tex;
uniform bool has_diffuse_tex; //If the texture tex actually exists and can be sampled from
uniform bool has_metalness_tex; //If the texture tex actually exists and can be sampled from
uniform bool has_roughness_tex; //If the texture tex actually exists and can be sampled from
uniform bool has_normals_tex; //If the texture tex actually exists and can be sampled from
uniform bool has_matcap_tex; //If the texture tex actually exists and can be sampled from
//only for solid rendering where there is only one value for metaless and roughness instead of a map
uniform float metalness;
uniform float roughness;
uniform float opacity;
uniform int mesh_id;
uniform bool using_fat_gbuffer;
//ssao stuff
uniform bool colors_are_precomputed_ao;
uniform bool enable_ssao;
uniform float ao_power;
uniform bool get_ao_from_precomputation;


//encode the normal using the equation from Cry Engine 3 "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
// vec2 encode_normal(vec3 normal){
//     if(normal==vec3(0)){ //we got a non existant normal, like if you have a point cloud without normals so we just output a zero
//         return vec2(0);
//     }
//     //in the rare case in which we have a normal like (0, 0, 1) then normalize the xy will not work so we add a small epsilon
//     if(normal.xy==vec2(0)){
//         normal.x+=0.001;
//         normal=normalize(normal);
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

    float metalness_out=metalness;
    float roughness_out=roughness;
    vec3 normal_to_encode=normal_in;

    //if we have a normal map texture we get the normal from there
    if (has_normals_tex){
        vec3 normal = texture(normals_tex, uv_in).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(TBN_in * normal);
        normal_to_encode=normal;
    }


    if(color_type==2){ //TEXTURE
        if(has_diffuse_tex){
            vec4 tex_color=texture(diffuse_tex, uv_in);
            // the texture sampling may mix the pixel with the background which has alpha of zero so it makes the color darker and also dimmer, if we renomalize we make it brighter again
            // if(tex_color.w!=0 ){
                // tex_color/=tex_color.w;
            // }
            if (tex_color.w<=0.01){
                // diffuse_out=vec4(vec3(1,0,0),1.0);
                // diffuse_out=vec4(vec3(1,1,1),1.0);
                discard;
            }else{
                diffuse_out = vec4( vec3(tex_color.xyz), opacity);
            }
        }else{
            diffuse_out=vec4( vec3(0.0), opacity );
        }

        if (has_metalness_tex){
            metalness_out=texture(metalness_tex, uv_in).x;
        }
        if (has_roughness_tex){
            roughness_out=texture(roughness_tex, uv_in).x;
        }
        // if (has_normals_tex){
            //nothing happens because its not implemented yey
            // vec3 normal_deviation=texture(normals_tex, uv_in).xyz - vec3(0.5);
            // normal_to_encode+=normal_deviation;
            // normal_to_encode=normalize(normal_to_encode);
        // }
    }else if(color_type==5){ //normal vector //NORMAL WILL BE OUTPUTTED FROM FRAGMENT SHADER BECAUSE sometime we might want to do normal mapping and only the framgne thas acces to that
        diffuse_out=vec4( (normal_to_encode+1.0)/2.0, opacity );
    }else if(color_type==9){ //normal vector in view coordinates
        vec3 normal_vis=normal_to_encode;
        normal_vis=normalize(vec3(V*vec4(normal_vis,0.0)));
        diffuse_out=vec4( (normal_vis+1.0)/2.0, opacity );
    }else if(color_type==10){ //normal vector in view coordinates
        vec3 normal_view_coord=normalize(vec3(V*vec4(normal_to_encode,0.0)));
        // diffuse_out=vec4( (normal_vis+1.0)/2.0, 1.0 );
        // vec2 muv = vec2(normal_view_coord)*0.5+vec2(0.5,0.5); //from https://github.com/nidorx/matcaps
        vec2 muv = vec2(normal_view_coord+1.0)*0.5; //from https://github.com/nidorx/matcaps
        if(has_matcap_tex){
            diffuse_out = vec4( texture(matcap_tex, vec2(muv.x, muv.y)).xyz,   opacity);
        }else{
            diffuse_out=vec4( vec3(0.0), opacity );
        }
    }else{
        diffuse_out=vec4(color_per_vertex_in, opacity); //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
    }

    if( enable_ssao && get_ao_from_precomputation && colors_are_precomputed_ao){
        diffuse_out=vec4(diffuse_out.xyz*pow(precomputed_ao_in.x, ao_power),  diffuse_out.w); 
    }

    metalness_and_roughness_out=vec2(metalness_out, roughness_out);
    // metalness_and_roughness_out=vec2(0.5, 0.5);

    // normal_out = vec4(normal_cam_coords_in, 1.0);

    //output normals as done in cryengine 3 presentation "a bit more defferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    // normal_out = normalize(normal_cam_coords_in.xy) * sqrt(normal_cam_coords_in.z*0.5+0.5);
    normal_out=encode_normal(normal_to_encode);

    mesh_id_out=mesh_id;

    if(render_uv_to_gbuffer){
        uv_out=uv_in;
    }

    // position_out = vec4(position_cam_coords_in, 1.0);
}
