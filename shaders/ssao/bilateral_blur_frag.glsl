#version 330 core
#extension GL_ARB_separate_shader_objects : require
#extension GL_ARB_explicit_attrib_location : require

layout(location=1) in vec2 texCoord;

layout(location=0,index=0) out vec4 out_Color;


//uniforms
const float KERNEL_RADIUS = 7;

// layout(location=0) uniform float g_Sharpness=0;
uniform float sigma_spacial;
uniform float sigma_depth;
uniform vec2  g_InvResolutionDirection; // either set x to 1/width or y to 1/height
uniform float ao_power;

uniform sampler2D texSource;
uniform sampler2D texLinearDepth;




//-------------------------------------------------------------------------

// vec4 BlurFunction(vec2 uv, float r, vec4 center_c, float center_d, inout float w_total)
// {
//   vec4  c = texture2D( texSource, uv );
//   float d = texture2D( texLinearDepth, uv).x;

// //   const float BlurSigma = float(KERNEL_RADIUS) * 0.5;
//   const float BlurSigma = float(KERNEL_RADIUS) *0.5;
//   const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);

//   float ddiff = (d - center_d) * g_Sharpness;

//     float sigma_spacial=3;
//     float divisor_s = -1./(2.*sigma_spacial*sigma_spacial); //divisor in the exp function of the spacial component
// float dist_spacial = r*r;
//             float w_spacial = exp(divisor_s*float(dist_spacial*dist_spacial));
// // float dist_depth =offset_depth - depth_linear;


// //   float w = exp(-r*r*BlurFalloff - ddiff*ddiff);
// //   float w = -r*r*BlurFalloff - ddiff*ddiff;
// //   w_total += w;



//     // float dist_spacial = length(vec2(x,y));



// float w=w_spacial;
// w_total+=w;

//   return c*w;
// }




vec4 BlurFunction(vec2 uv, float r, vec4 center_c, float center_d, float divisor_s, float divisor_d , inout float w_total){
    vec4  c = texture2D( texSource, uv );
    float d = texture2D( texLinearDepth, uv).x;

    //if the current pixel has no depth, don't accumulate anything
    if(d==1.0){
        return vec4(0);
    }


    float dist_spacial = r;
    float dist_depth = abs(d- center_d);

    float w_spacial =  exp(divisor_s*float(dist_spacial*dist_spacial));
    float w_d = - exp(divisor_d*float(dist_depth*dist_depth));
    float w = w_spacial*w_d;


    w_total+=w;
    return c*w;
}



void main()
{
  vec4  center_c = texture2D( texSource, texCoord );
  float center_d = texture2D( texLinearDepth, texCoord).x;

  if(center_d==1.0){
      discard;
  }

  vec4  c_total = center_c;
  float w_total = 1.0;

//   for (float r = 1; r <= KERNEL_RADIUS; ++r)
//   {
//     vec2 uv = texCoord + g_InvResolutionDirection * r;
//     c_total += BlurFunction(uv, r, center_c, center_d, w_total);
//   }

//   for (float r = 1; r <= KERNEL_RADIUS; ++r)
//   {
//     vec2 uv = texCoord - g_InvResolutionDirection * r;
//     c_total += BlurFunction(uv, r, center_c, center_d, w_total);
//   }


    float divisor_s = -1./(2.*sigma_spacial*sigma_spacial); //divisor in the exp function of the spacial component
    float divisor_d = -1./(2.*sigma_depth*sigma_depth);
    float half_size = sigma_spacial * 2;

    //attempt2
  for (float r = -half_size; r <= half_size; ++r){
    vec2 uv = texCoord + vec2(g_InvResolutionDirection.x, 0.0) * r;
    c_total += BlurFunction(uv, r, center_c, center_d, divisor_s, divisor_d, w_total);
  }

  for (float r = -half_size; r <= half_size; ++r){
    vec2 uv = texCoord - vec2(0.0, g_InvResolutionDirection.y)  * r;
    c_total += BlurFunction(uv, r, center_c, center_d, divisor_s, divisor_d, w_total);
  }


// //attemot3
//   for (float r = -KERNEL_RADIUS/2; r <= KERNEL_RADIUS/2; ++r)
//   {
//     vec2 uv = texCoord + vec2(g_InvResolutionDirection.x, 0.0) * r;
//     c_total += BlurFunction(uv, r, center_c, center_d, w_total);
//   }




  out_Color = c_total/w_total;

    out_Color.x=pow(out_Color.x, ao_power);

}
