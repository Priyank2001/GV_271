#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices=12) out;   // up to 4 tris now (2-plane clip)

in vec3 gLocalPos[];
in vec3 gLocalNormal[];

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uNearDepth;
uniform float uFarDepth;
uniform float uWedgeOffsetA;
uniform float uWedgeOffsetB;

uniform vec3 uWedgeNormalA;
uniform vec3 uWedgeNormalB;
uniform vec3 uOffset;

out vec3 vNormal;
out vec3 vFragPos;
out float vDepth01;

void emitWorld(vec3 worldPos, vec3 worldNormal){
    vec3 offsetPos = worldPos + uOffset;
    vec4 view = uView * vec4(offsetPos, 1.0);
    vNormal = worldNormal;
    vFragPos = offsetPos;
    float dist = -view.z;
    vDepth01 = clamp((dist - uNearDepth)/(uFarDepth - uNearDepth), 0.0, 1.0);
    gl_Position = uProjection * view;
    EmitVertex();
}
void emitTri(vec3 p0,vec3 n0,vec3 p1,vec3 n1,vec3 p2,vec3 n2){
    emitWorld(p0,n0); emitWorld(p1,n1); emitWorld(p2,n2);
    EndPrimitive();
}

// Clips ONE triangle against a plane through the origin, keeping dot(p,n)>=0.
// Writes up to 2 output triangles into outP/outN, returns triangle count.
int clipTri(vec3 p0,vec3 n0,vec3 p1,vec3 n1,vec3 p2,vec3 n2, vec3 planeN, float planeW,
            out vec3 outP[6], out vec3 outN[6]){
    float d0=dot(p0,planeN)-planeW, d1=dot(p1,planeN)-planeW, d2=dot(p2,planeN)-planeW;
    bool pos0=d0>0.0, pos1=d1>0.0, pos2=d2>0.0;
    int numPos = int(pos0)+int(pos1)+int(pos2);
    if(numPos==3){ outP[0]=p0;outN[0]=n0; outP[1]=p1;outN[1]=n1; outP[2]=p2;outN[2]=n2; return 1; }
    if(numPos==0){ return 0; }
    int lone;
    if(pos0==pos1) lone=2; else if(pos0==pos2) lone=1; else lone=0;
    vec3 pa,na,pb,nb,pc,nc; float da,db,dc;
    if(lone==0){ pa=p0;na=n0;da=d0; pb=p1;nb=n1;db=d1; pc=p2;nc=n2;dc=d2; }
    else if(lone==1){ pa=p1;na=n1;da=d1; pb=p2;nb=n2;db=d2; pc=p0;nc=n0;dc=d0; }
    else { pa=p2;na=n2;da=d2; pb=p0;nb=n0;db=d0; pc=p1;nc=n1;dc=d1; }
    float tb=da/(da-db); float tc=da/(da-dc);
    vec3 pib=mix(pa,pb,tb); vec3 nib=normalize(mix(na,nb,tb));
    vec3 pic=mix(pa,pc,tc); vec3 nic=normalize(mix(na,nc,tc));
    bool loneIsPos = da>0.0;
    if(loneIsPos){
        outP[0]=pa;outN[0]=na; outP[1]=pib;outN[1]=nib; outP[2]=pic;outN[2]=nic;
        return 1;
    } else {
        outP[0]=pib;outN[0]=nib; outP[1]=pb;outN[1]=nb; outP[2]=pc;outN[2]=nc;
        outP[3]=pib;outN[3]=nib; outP[4]=pc;outN[4]=nc; outP[5]=pic;outN[5]=nic;
        return 2;
    }
}

void main(){
    vec3 p0 = (uModel * vec4(gLocalPos[0], 1.0)).xyz;
    vec3 p1 = (uModel * vec4(gLocalPos[1], 1.0)).xyz;
    vec3 p2 = (uModel * vec4(gLocalPos[2], 1.0)).xyz;
    vec3 n0 = normalize(uNormalMatrix * gLocalNormal[0]);
    vec3 n1 = normalize(uNormalMatrix * gLocalNormal[1]);
    vec3 n2 = normalize(uNormalMatrix * gLocalNormal[2]);

    vec3 stageAP[6]; vec3 stageAN[6];
    int nA = clipTri(p0,n0,p1,n1,p2,n2, uWedgeNormalA, uWedgeOffsetA, stageAP, stageAN);


    for(int t = 0; t < nA; t++){
        vec3 stageBP[6]; vec3 stageBN[6];
        int nB = clipTri(stageAP[t*3+0],stageAN[t*3+0], stageAP[t*3+1],stageAN[t*3+1],
                          stageAP[t*3+2],stageAN[t*3+2], uWedgeNormalB, uWedgeOffsetB, stageBP, stageBN);
        for(int u = 0; u < nB; u++){
            emitTri(stageBP[u*3+0],stageBN[u*3+0], stageBP[u*3+1],stageBN[u*3+1], stageBP[u*3+2],stageBN[u*3+2]);
        }
    }
}