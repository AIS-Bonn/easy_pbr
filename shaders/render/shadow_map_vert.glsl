//in
in vec3 position;

//out


//uniforms
uniform mat4 MVP;

void main(){


   gl_Position = MVP*vec4(position, 1.0);

   
}