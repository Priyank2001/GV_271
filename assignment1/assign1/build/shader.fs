#version 330 core

in vec3 vColor;
out vec4 FragColor ;

uniform vec3 uColor;
uniform bool uUseVertexColor;

void main(){
    vec3 color = uUseVertexColor ? vColor : uColor;
    FragColor = vec4(color , 1.0);
}
