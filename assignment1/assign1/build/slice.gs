#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices=6) out;

in vec3 gLocalPos[];
in vec3 gLocalNormal[];

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uNearDepth;
uniform float uFarDepth;

uniform vec3 uPlanePoint;
uniform vec3 uPlaneNormal;
uniform vec3 uOffset;
uniform float uKeepSide;

out vec3 vNormal;
out vec3 vFragPos;
out float vDepth01;


void emit(vec3 localPos, vec3 localNormal){
    vec3 offsetPos = localPos + uOffset;
    vec4 world = uModel * vec4(offsetPos,1.0);
    vec4 view = uView * world;

    vNormal = normalize(uNormalMatrix * localNormal);
    vFragPos = world.xyz;
    float dist = -view.z;
    vDepth01 = clamp((dist-uNearDepth)/(uFarDepth - uNearDepth), 0.0, 1.0 );

    gl_Position = uProjection * view;
    EmitVertex();
}

void emitTri(vec3 p0,vec3 n0,vec3 p1,vec3 n1,vec3 p2,vec3 n2){
    emit(p0,n0);
    emit(p1,n1);
    emit(p2,n2);
    EndPrimitive();
}

void main(){
    vec3 p0 = gLocalPos[0] , p1 = gLocalPos[1] , p2 = gLocalPos[2];
    vec3 n0 = gLocalNormal[0], n1 = gLocalNormal[1], n2 = gLocalNormal[2];

    float d0 = dot(p0 - uPlanePoint,uPlaneNormal);
    float d1 = dot(p1 - uPlanePoint,uPlaneNormal);
    float d2 = dot(p2 - uPlanePoint,uPlaneNormal);

    bool pos0 = d0 > 0.0 , pos1 = d1 > 0.0, pos2 = d2 > 0.0;
    int numPos = int(pos0) + int(pos1) + int(pos2);


    if(numPos == 3){ if(uKeepSide > 0.0) emitTri(p0,n0,p1,n1,p2,n2); return; }
    if(numPos == 0){ if(uKeepSide < 0.0) emitTri(p0,n0,p1,n1,p2,n2); return; }

    int lone;
    if(pos0 == pos1) lone = 2; else if(pos0 == pos2) lone = 1; else lone = 0;
    
    vec3 pa,na,pb,nb,pc,nc; float da,db,dc;
    if(lone==0){ pa=p0;na=n0;da=d0; pb=p1;nb=n1;db=d1; pc=p2;nc=n2;dc=d2; }
    else if(lone==1){ pa=p1;na=n1;da=d1; pb=p2;nb=n2;db=d2; pc=p0;nc=n0;dc=d0; }
    else { pa=p2;na=n2;da=d2; pb=p0;nb=n0;db=d0; pc=p1;nc=n1;dc=d1; }
    
    
    float tb = da/(da-db);
    float tc = da/(da-dc);
    vec3 pib = mix(pa,pb,tb); vec3 nib = normalize(mix(na,nb,tb));
    vec3 pic = mix(pa,pc,tc); vec3 nic = normalize(mix(na,nc,tc));

    bool loneIsPos = da > 0.0;
    if(loneIsPos){
        if(uKeepSide > 0.0) emitTri(pa,na, pib,nib, pic,nic);
        else { emitTri(pib,nib, pb,nb, pc,nc); emitTri(pib,nib, pc,nc, pic,nic); }
    } else {
        if(uKeepSide < 0.0) emitTri(pa,na, pib,nib, pic,nic);
        else { emitTri(pib,nib, pb,nb, pc,nc); emitTri(pib,nib, pc,nc, pic,nic); }
    }
}