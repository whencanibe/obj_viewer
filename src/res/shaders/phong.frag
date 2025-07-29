#version 330 core
in vec3 vPos;
in vec3 vNrm;

uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNrm);
    vec3 L = normalize(uLightPos - vPos);
    vec3 V = normalize(uViewPos - vPos);
    vec3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), 32.0);

    vec3 color = vec3(0.8) * diff + vec3(1.0) * spec * 0.4;
    FragColor  = vec4(color, 1.0);
}
