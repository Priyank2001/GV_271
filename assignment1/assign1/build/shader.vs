#version 330 core


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 vColor;

uniform mat4 uModel;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uNormalMatrix;
uniform float uNearDepth;
uniform float uFarDepth;

out vec3 vNormal;
out vec3 vFragPos;
out float vDepth01;



void main(){
    // Understand this part of the code....
     
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    
    vec4 viewPos = uView * worldPos ;
    float dist = -viewPos.z;
    vDepth01 = clamp((dist - uNearDepth)/(uFarDepth - uNearDepth),0.0,1.0);

    gl_Position = uProjection * uModel * vec4(aPos,1.0);
    vColor = aColor;
}
