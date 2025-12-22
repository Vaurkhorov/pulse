#version 330 core
out vec4 FragColor;

in vec3 Color;

uniform bool useUniformColor;
uniform vec3 uniformColor;

void main()
{
    if (useUniformColor) {
        // --- TRAFFIC LIGHTS (Procedural Glow) ---
        // Convert square point into a circle
        vec2 coord = gl_PointCoord - vec2(0.5); 
        float dist = length(coord);

        if (dist > 0.5) discard; // Cut off corners to make it round

        // Create a soft glow gradient (1.0 at center, 0.0 at edge)
        float glow = 1.0 - (dist * 2.0);
        glow = pow(glow, 0.5); // Make the core brighter

        // Apply glow to alpha channel
        FragColor = vec4(uniformColor, glow);
    } 
    else {
        // --- HEATMAP (Holographic Line) ---
        // Set alpha to 0.7 for a see-through glass look
        FragColor = vec4(Color, 0.7); 
    }
}