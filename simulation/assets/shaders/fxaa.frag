#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 u_resolution; // The screen resolution (e.g., 1280, 720)

// FXAA Implementation (by Timothy Lottes)
#define FXAA_REDUCE_MIN   (1.0/128.0)
#define FXAA_REDUCE_MUL   (1.0/8.0)
#define FXAA_SPAN_MAX     8.0

vec3 fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution) {
    vec2 inverseResolution = 1.0 / resolution;
    vec3 rgbNW = texture(tex, (fragCoord + vec2(-1.0, -1.0)) * inverseResolution).xyz;
    vec3 rgbNE = texture(tex, (fragCoord + vec2(1.0, -1.0)) * inverseResolution).xyz;
    vec3 rgbSW = texture(tex, (fragCoord + vec2(-1.0, 1.0)) * inverseResolution).xyz;
    vec3 rgbSE = texture(tex, (fragCoord + vec2(1.0, 1.0)) * inverseResolution).xyz;
    vec3 rgbM  = texture(tex, fragCoord  * inverseResolution).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)),
                     ((lumaNW + lumaSW) - (lumaNE + lumaSE)));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * inverseResolution;
              
    vec3 rgbA = 0.5 * (
        texture(tex, fragCoord * inverseResolution + dir * (1.0/3.0 - 0.5)).xyz +
        texture(tex, fragCoord * inverseResolution + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, fragCoord * inverseResolution + dir * -0.5).xyz +
        texture(tex, fragCoord * inverseResolution + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    return rgbB;
}

void main() {
    // CORRECTED LINE: Pass the pixel coordinate and resolution to the function
    FragColor = vec4(fxaa(screenTexture, TexCoords * u_resolution, u_resolution), 1.0);
}