#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 groundColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    // Simple lighting calculation
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    vec3 result = (0.3 + diffuse) * groundColor;
    FragColor = vec4(result, 1.0);
}
