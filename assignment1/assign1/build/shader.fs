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
uniform int uColormapId; // 0 = viridis (sequential), 1 = coolwarm (diverging)

vec3 colormapViridis(float t){
    t = clamp(t, 0.0, 1.0);
    vec3 c0=vec3(0.267,0.005,0.329), c1=vec3(0.283,0.141,0.458),
         c2=vec3(0.254,0.265,0.530), c3=vec3(0.164,0.471,0.558),
         c4=vec3(0.478,0.821,0.318), c5=vec3(0.993,0.906,0.144);
    float s = t * 5.0;
    int i = int(floor(s));
    float f = fract(s);
    if(i<=0) return mix(c0,c1,f);
    if(i==1) return mix(c1,c2,f);
    if(i==2) return mix(c2,c3,f);
    if(i==3) return mix(c3,c4,f);
    return mix(c4,c5,clamp(f,0.0,1.0));
}

// Diverging map: suited to signed/bipolar data (e.g. distance from the cut plane),
// included to contrast with viridis's sequential use here.
vec3 colormapCoolWarm(float t){
    t = clamp(t, 0.0, 1.0);
    vec3 cold=vec3(0.230,0.299,0.754), mid=vec3(0.865,0.865,0.865), warm=vec3(0.706,0.016,0.150);
    return t < 0.5 ? mix(cold, mid, t*2.0) : mix(mid, warm, (t-0.5)*2.0);
}

void main(){

    if(!uUseVertexColor){
        FragColor = vec4(uColor,1.0);
        return;
    }
    vec3 baseColor = (uColormapId == 0) ? colormapViridis(vDepth01) : colormapCoolWarm(vDepth01);


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
