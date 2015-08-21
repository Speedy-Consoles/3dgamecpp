#version 330 core

uniform vec4 diffuseColor;

in vec4 vfColor;
out vec4 fColor;
 
void main() {
    fColor = diffuseColor * vfColor;
}