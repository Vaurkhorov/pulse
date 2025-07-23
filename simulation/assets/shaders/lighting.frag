#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords; // NEW

uniform vec3 viewPos;
// uniform vec3 objectColor; // We replace this with a texture sampler

uniform sampler2D ourTexture; // NEW: The texture sampler

// Light properties
uniform vec3 lightPos;
uniform vec3 lightColor;

void main()
{
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting and texture
    vec3 lighting = (ambient + diffuse + specular);
    vec3 textureColor = texture(ourTexture, TexCoords).rgb; // NEW
    
    vec3 result = lighting * textureColor; // NEW
    FragColor = vec4(result, 1.0);
}