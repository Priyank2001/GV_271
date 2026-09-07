#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNormal;

out vec3 gLocalPos;
out vec3 gLocalNormal;

void main(){
    // No transform here on purpose — the geometry shader clips in local
    // (pre-rotation) space, then applies model/view/projection itself
    // only to the vertices it actually decides to emit.
    gLocalPos = aPos;
    gLocalNormal = aNormal;
    gl_Position = vec4(aPos, 1.0); // unused by GS input stage, required by spec
}