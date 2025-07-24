#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Instance matrix as 4 separate vec4s 
layout (location = 3) in vec4 aInstanceMatrix0;
layout (location = 4) in vec4 aInstanceMatrix1;
layout (location = 5) in vec4 aInstanceMatrix2;
layout (location = 6) in vec4 aInstanceMatrix3;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 view;
uniform mat4 projection;

void main() {
    // Reconstruct the instance matrix from the 4 vec4s
    mat4 instanceMatrix = mat4(
        aInstanceMatrix0,
        aInstanceMatrix1,
        aInstanceMatrix2,
        aInstanceMatrix3
    );
    
    // Transform position and normal using the instance matrix
    FragPos = vec3(instanceMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
    
    // Final position calculation
    gl_Position = projection * view * vec4(FragPos, 1.0);
}