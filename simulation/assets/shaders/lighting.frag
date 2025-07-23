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
    vec3 asphaltColor = texture(ourTexture, TexCoords).rgb;
    vec3 finalColor = asphaltColor * lighting;


    // --- 2. "Paint" the lane markings on top ---

    // Define line colors
    vec3 centerLineColor = vec3(0.9, 0.8, 0.2); // Yellow-ish white
    vec3 laneLineColor = vec3(1.0, 1.0, 1.0);   // Pure white

    // Define line widths as a percentage of the road width
    float centerLineWidth = 0.015; // 1.5% of the road width
    float laneLineWidth = 0.01;   // 1.0% of the road width

    // --- Center Line ---
    // TexCoords.x is the coordinate across the road's width (from 0.0 to 1.0)
    // The center is at 0.5. We check if we are within half a line-width of the center.
    if (abs(TexCoords.x - 0.5) < centerLineWidth / 2.0) {
        finalColor = centerLineColor * lighting;
    }

    // --- Dashed Lane Lines for a 4-lane road (at 1/4 and 3/4 positions) ---
    bool isLaneLinePosition = abs(TexCoords.x - 0.25) < laneLineWidth / 2.0 || abs(TexCoords.x - 0.75) < laneLineWidth / 2.0;

    if (isLaneLinePosition) {
        // TexCoords.y is the coordinate along the road's length.
        // We use fract() to create a repeating dashed pattern.
        // This creates dashes that are on for half the time and off for half the time.
        if (fract(TexCoords.y * 10.0) > 0.5) {
            finalColor = laneLineColor * lighting;
        }
    }

    FragColor = vec4(finalColor, 1.0);
}
//We first calculate the final color of the asphalt texture with full lighting, as before.

//We then check the texture coordinate TexCoords.x, which represents the position across the road's width (from 0.0 on one edge to 1.0 on the other).

//Center Line: If the coordinate is very close to the middle (0.5), we overwrite the asphalt color with the yellow line color.

//Dashed Lines: We check if the coordinate is near the 1/4 (0.25) or 3/4 (0.75) marks. If it is, we use the TexCoords.y coordinate (which runs along the road's length) to create a repeating dashed pattern.

