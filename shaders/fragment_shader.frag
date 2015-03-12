#version 330 core

in vec3 gl_Color;
out vec3 color;
 
void main(){
    color = gl_Color;
}