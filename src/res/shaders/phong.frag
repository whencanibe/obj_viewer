#version 330 core
in vec3 vPos;
in vec3 vNrm;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform float uKd;      // Diffuse
uniform float uKs;      // Specular
uniform float uShin;    // shininess

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNrm);
    vec3 L = normalize(uLightPos - vPos);
    vec3 V = normalize(uViewPos - vPos);
    vec3 R = reflect(-L, N);

    float diff = uKd * max(dot(N, L), 0.0);
    float spec = uKs * pow(max(dot(V, R), 0.0), uShin);
    vec3 color = vec3(0.8) * diff + vec3(1.0) * spec;

    FragColor = vec4(color, 1.0);
}
