#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNrm;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vPos;   // world‑space
out vec3 vNrm;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vPos = worldPos.xyz;
    vNrm = mat3(transpose(inverse(uModel))) * aNrm;
    gl_Position = uProj * uView * worldPos;
}
