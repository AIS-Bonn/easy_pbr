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
layout(location = 2) in vec3 normal_cam_coords_in; //normal of the vertex in the camera coordinate system (so the normal is multipled by the rotation of tf_cam_world or also known as the view matrix)
layout(location = 3) in vec3 color_per_vertex_in;
layout(location = 4) in vec2 uv_in;
layout(location = 5) in float log_depth_in;


//out
//the locations are irrelevant because the link between the frag output and the texture is established at runtime by the shader function draw_into(). They just have to be different locations for each output
// layout(location = 0) out vec4 position_out; 
layout(location = 1) out vec4 diffuse_out;
layout(location = 3) out vec2 normal_out;
layout(location = 4) out float log_depth_out;
layout(location = 5) out vec2 metalness_and_roughness_out;

// //uniform
uniform int color_type;
uniform sampler2D tex; //the rgb tex that is used for coloring
uniform bool has_normals=false; //if we have normals this will get set to true
//only for solid rendering where there is only one value for metaless and roughness instead of a map
uniform float metalness;
uniform float roughness;

//encode the normal using the equation from Cry Engine 3 "A bit more deferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
vec2 encode_normal(vec3 normal){
    if(normal==vec3(0)){ //we got a non existant normal, like if you have a point cloud without normals so we just output a zero
        return vec2(0);
    }
    vec2 normal_encoded = normalize(normal.xy) * sqrt(normal.z*0.5+0.5);
    return normal_encoded;
}


void main(){

    float log_depth_val=log_depth_in;

    //from https://github.com/potree/potree/blob/develop/src/materials/shaders/pointcloud.fs
    bool points_as_circle=false;
    // bool points_as_circle=true;
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
        diffuse_out = vec4(tex_color);
    }else{
        diffuse_out = vec4(color_per_vertex_in, 1.0); //we output whatever we receive from the vertex shader which will be normal color, solid color, semantic_color etc
    }

    // normal_out = vec4(normal_cam_coords_in, 1.0);
        //output normals as done in cryengine 3 presentation "a bit more defferred" https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    // normal_out = normalize(normal_cam_coords_in.xy) * sqrt(normal_cam_coords_in.z*0.5+0.5);
    normal_out=encode_normal(normal_cam_coords_in);
    metalness_and_roughness_out=vec2(metalness, roughness);
    // vec3 normal = normalize(cross(dFdx(position_cam_coords_in), dFdy(position_cam_coords_in)));
    // normal_out=vec4(normal,1.0);
    // normal_out=vec4(normal*0.5 + 0.5,1.0);
    // normal_out=vec4(1.0);
    // diffuse_out = vec4(normal*0.5 + 0.5,1.0);
    // diffuse_out=vec4( dFdx(position_cam_coords_in), 1.0 );

    //attempt 2 to get normals 
    // vec3 position
    // vec3 q0 = dFdx(position_eye.xyz);
    // vec3 q1 = dFdy(position_eye.xyz);
    // vec2 st0 = dFdx(textureCoord.st);
    // vec2 st1 = dFdy(textureCoord.st);

    // float Sx = ( q0.x * st1.t - q1.x * st0.t) / (st1.t * st0.s - st0.t * st1.s);
    // float Tx = (-q0.x * st1.s + q1.x * st0.s) / (st1.t * st0.s - st0.t * st1.s);

    // q0.x = st0.s * Sx + st0.t * Tx;
    // q1.x = st1.s * Sx + st1.t * Tx;

    // vec3 S = normalize( q0 * st1.t - q1 * st0.t);
    // vec3 T = normalize(-q0 * st1.s + q1 * st0.s);

    // vec3 n = texture2D(normal,textureCoord).xyz;
    // n = smoothstep(-1,1,n);

    // mat3 tbn = (mat3(S,T,n));

    //for edl https://github.com/potree/potree/blob/develop/src/materials/shaders/pointcloud.fs
    // log_depth_out=log_depth_val;
    // log_depth_out=0.1;


    //instead of position, store depth and recover the position from the depth 


    // position_out = vec4(position_cam_coords_in, 1.0);
}

