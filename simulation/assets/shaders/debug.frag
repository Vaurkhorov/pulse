#version 330 core
out vec4 FragColor;

in vec3 Color;

// Toggle: If true, ignore vertex color and use a single uniform color (For Traffic Lights)
uniform bool useUniformColor;
uniform vec3 uniformColor;

void main()
{
    if (useUniformColor) {
        FragColor = vec4(uniformColor, 1.0);
    } else {
        FragColor = vec4(Color, 1.0);
    }
}