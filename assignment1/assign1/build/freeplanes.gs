#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices=24) out;   // 3 chained clips -> up to 8 tris

in vec3 gLocalPos[];
in vec3 gLocalNormal[];

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uNearDepth;
uniform float uFarDepth;

const int MAX_FP = 3;              // planes; output budget is 3 * 2^MAX_FP
uniform vec3  uPlaneN[MAX_FP];
uniform float uPlaneW[MAX_FP];
uniform int   uNumPlanes;
uniform int   uCellMask;           // bit p = keep the positive side of plane p
uniform float uStep;

out vec3 vNormal;
out vec3 vFragPos;
out float vDepth01;

void emitWorld(vec3 worldPos, vec3 worldNormal){
    vec4 view = uView * vec4(worldPos, 1.0);
    vNormal = worldNormal;
    vFragPos = worldPos;
    float dist = -view.z;
    vDepth01 = clamp((dist - uNearDepth)/(uFarDepth - uNearDepth), 0.0, 1.0);
    gl_Position = uProjection * view;
    EmitVertex();
}

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
    if(da>0.0){
        outP[0]=pa;outN[0]=na; outP[1]=pib;outN[1]=nib; outP[2]=pic;outN[2]=nic;
        return 1;
    } else {
        outP[0]=pib;outN[0]=nib; outP[1]=pb;outN[1]=nb; outP[2]=pc;outN[2]=nc;
        outP[3]=pib;outN[3]=nib; outP[4]=pc;outN[4]=nc; outP[5]=pic;outN[5]=nic;
        return 2;
    }
}

void main(){
    vec3 curP[24]; vec3 curN[24];
    curP[0] = (uModel * vec4(gLocalPos[0],1.0)).xyz;
    curP[1] = (uModel * vec4(gLocalPos[1],1.0)).xyz;
    curP[2] = (uModel * vec4(gLocalPos[2],1.0)).xyz;
    curN[0] = normalize(uNormalMatrix * gLocalNormal[0]);
    curN[1] = normalize(uNormalMatrix * gLocalNormal[1]);
    curN[2] = normalize(uNormalMatrix * gLocalNormal[2]);
    int nTris = 1;

    // The cell mask says which side of each plane this cell keeps. The same
    // mask also determines the displacement, so the offset is re-derived here
    // rather than carried between passes -- there is no per-cell state to keep.
    vec3 accum = vec3(0.0);

    for(int p = 0; p < uNumPlanes; p++){
        float sgn = ((uCellMask & (1 << p)) != 0) ? 1.0 : -1.0;
        accum += (uStep * sgn) * uPlaneN[p];

        vec3 nxtP[24]; vec3 nxtN[24];
        int outCount = 0;
        for(int t = 0; t < nTris; t++){
            vec3 tp[6]; vec3 tn[6];
            int k = clipTri(curP[t*3+0],curN[t*3+0], curP[t*3+1],curN[t*3+1],
                            curP[t*3+2],curN[t*3+2],
                            sgn*uPlaneN[p], sgn*uPlaneW[p], tp, tn);
            for(int j = 0; j < k*3; j++){
                if(outCount < 24){ nxtP[outCount]=tp[j]; nxtN[outCount]=tn[j]; outCount++; }
            }
        }
        if(outCount == 0) return;      // triangle is not in this cell
        nTris = outCount / 3;
        for(int j = 0; j < outCount; j++){ curP[j]=nxtP[j]; curN[j]=nxtN[j]; }
    }

    accum.z = 0.0;                     // orthographic: z offset is invisible
    for(int t = 0; t < nTris; t++){
        emitWorld(curP[t*3+0]+accum, curN[t*3+0]);
        emitWorld(curP[t*3+1]+accum, curN[t*3+1]);
        emitWorld(curP[t*3+2]+accum, curN[t*3+2]);
        EndPrimitive();
    }
}