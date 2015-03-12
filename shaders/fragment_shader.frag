#version 330 core

in float brightness;
out vec3 color;
 
void main(){
    color = vec3(brightness);
}