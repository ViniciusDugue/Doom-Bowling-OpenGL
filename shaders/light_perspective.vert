#version 330
// A vertex shader for rendering vertices with normal vectors and texture coordinates,
// which creates outputs needed for a Phong reflection fragment shader.
layout (location=0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;
layout (location=2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragWorldPos;
out vec4 FragLightSpacePos;

out mat3 TBN;

void main() {
    // Transform the vertex position from local space to clip space.
    gl_Position = projection * view * model * vec4(vPosition, 1.0);
    // Pass along the vertex texture coordinate.
    TexCoord = vTexCoord;
    // Transform the vertex normal from local space to world space, using the Normal matrix.
    mat4 normalMatrix = transpose(inverse(model));
    Normal = mat3(normalMatrix) * vNormal;
    
    // TODO: transform the vertex position into world space, and assign it to FragWorldPos.
    FragWorldPos = vec3(model * vec4(vPosition, 1.0));
    
    FragLightSpacePos = lightSpaceMatrix * model * vec4(vPosition, 1.0);

    vec3 worldNormal = normalize(mat3(normalMatrix) * vNormal);
    vec3 worldTangent = normalize(mat3(normalMatrix) * vTangent);
    vec3 worldBitangent = normalize(mat3(normalMatrix) * vBitangent);
    
    TBN = mat3(worldTangent, worldBitangent, worldNormal);
}