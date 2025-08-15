#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform sampler2D ourTexture; // The ground texture
uniform vec3 lightPos;
uniform vec3 lightColor;

void main()
{
    // --- Standard Blinn-Phong Lighting ---
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.1; // Ground is not very shiny
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting and the ground's texture
    vec3 lighting = (ambient + diffuse + specular);
    vec3 textureColor = texture(ourTexture, TexCoords).rgb;
    
    vec3 result = lighting * textureColor;
    FragColor = vec4(result, 1.0);
}