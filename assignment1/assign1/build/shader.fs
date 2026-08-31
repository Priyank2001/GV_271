#version 330 core


in vec3 vNormal;
in vec3 vFragPos;
in float vDepth01;


in vec3 vColor;
out vec4 FragColor ;

uniform vec3 uColor;
uniform bool uUseVertexColor;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;

uniform float uKa, uKd, uKs, uShininess;
uniform vec3  uDepthColorNear;
uniform vec3  uDepthColorFar;


void main(){

    if(!uUseVertexColor){
        FragColor = vec4(uColor,1.0);
        return;
    }
    vec3 baseColor = mix(uDepthColorNear, uDepthColorFar , vDepth01);
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);

    vec3 ambient = uKa * baseColor;

    float diff = max(dot(N,L) , 0.0);
    vec3 diffuse =  uKd * diff * baseColor * uLightColor ; 
    float spec = pow(max(dot(N,H),0.0), uShininess);
    vec3 specular =  uKs * spec * uLightColor;

    FragColor  = vec4(ambient + diffuse + specular, 1.0);

}
