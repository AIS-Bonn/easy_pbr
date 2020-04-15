// #version 430 core

// //in
// layout(location = 0) in vec3 color_per_vertex_in;

// //out 
// layout(location = 0) out vec4 out_color;

// //uniforms
// // uniform bool enable_point_color; // whether to use point color or color per vertex

// void main(){

//     out_color = vec4(color_per_vertex_in, 1.0);
// }

#version 430 core

//in
layout(location = 0) in vec3 normal_in;
layout(location = 1) in vec3 position_cam_coords_in; //position of the vertex in the camera coordinate system (so the world coordinate is multipled by tf_cam_world or also known as the view matrix)
layout(location = 2) in vec3 color_per_vertex_in;
layout(location = 3) in vec2 uv_in;
// layout(location = 5) in float log_depth_in;


//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
// layout(location = 0) out vec4 position_out; 
layout(location = 1) out vec4 diffuse_out;
layout(location = 2) out vec3 normal_out;
layout(location = 3) out vec2 metalness_and_roughness_out;

// //uniform
uniform int color_type;
uniform sampler2D tex; //the rgb tex that is used for coloring
uniform bool has_normals=false; //if we have normals this will get set to true
//only for solid rendering where there is only one value for metaless and roughness instead of a map
uniform float metalness;
uniform float roughness;
uniform bool using_fat_gbuffer;

//encode the normal using the equation from Cry Engine 3 "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
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
        if(!has_normals){
            return vec3(0.0);
        }else{
            return normal * 0.5 + 0.5;
        }
    }
}


void main(){

    // float log_depth_val=log_depth_in;

    //from https://github.com/potree/potree/blob/develop/src/materials/shaders/pointcloud.fs
    // bool points_as_circle=false;
    bool points_as_circle=false;
    float u,v; //local uvs inside the point
    if(points_as_circle){
        u = 2.0 * gl_PointCoord.x - 1.0;
        v = 2.0 * gl_PointCoord.y - 1.0;

        float cc = u*u + v*v;
		if(cc > 1.0){
			discard;
        }

        // float wi = 0.0 - ( u*u + v*v);
		// vec4 pos = vec4(vViewPosition, 1.0);
		// pos.z += wi * vRadius;
		// float linearDepth = -pos.z;
		// pos = projectionMatrix * pos;
		// pos = pos / pos.w;
		// float expDepth = pos.z;
		// depth = (pos.z + 1.0) / 2.0;
        // gl_FragDepthEXT = depth;

    }
    


    if(color_type==2){ //TEXTURE
        vec4 tex_color=texture(tex, uv_in);
        // the texture sampling may mix the pixel with the background which has alpha of zero so it makes the color darker and also dimmer, if we renomalize we make it brighter again
        if(tex_color.w!=0 ){
            tex_color/=tex_color.w;
        }
        diffuse_out = vec4(tex_color.rgb, 1.0);
    }else{
        diffuse_out = vec4(color_per_vertex_in, 1.0); //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
    }

    normal_out=encode_normal(normal_in);
    metalness_and_roughness_out=vec2(metalness, roughness);
  
}

