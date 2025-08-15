#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 carColor;

void main() {
    float ambient = 0.3;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 lighting = (ambient + diff) * vec3(1.0);
    FragColor = vec4(carColor * lighting, 1.0);
}