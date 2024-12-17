#version 330
// A fragment shader for rendering fragments in the Phong reflection model.
layout (location=0) out vec4 FragColor;

// Inputs: the texture coordinates, world-space normal, and world-space position
// of this fragment, interpolated between its vertices.
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragWorldPos;
in vec4 FragLightSpacePos;
in mat3 TBN;
// Uniforms: MUST BE PROVIDED BY THE APPLICATION.

// The mesh's base (diffuse) texture.
uniform sampler2D baseTexture;
uniform bool hasNormalMap;
uniform sampler2D normalMap;
uniform sampler2D specMap;
uniform sampler2D shadowMap;

// Material parameters for the whole mesh: k_a, k_d, k_s, shininess.
uniform vec4 material;

// Ambient light color.
uniform vec3 ambientColor;

// Direction and color of a single directional light.
uniform vec3 directionalLight; // this is the "I" vector, not the "L" vector.
uniform vec3 directionalColor;



// Location of the camera.
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragLightSpacePos)
{
    // clip space to NDC
    vec3 projCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
    
    // normalize to [0,1]
    projCoords = (projCoords * 0.5) + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;// closest frags depth
    float currentDepth = projCoords.z; // current frags depth 
    
    // comparing shadowmap depth with current light depth
    float shadow = 0.0;
    if (currentDepth > closestDepth)
    {
        shadow = 1.0;
    }
    return shadow;
}


void main() {
    // TODO: using the lecture notes, compute ambientIntensity, diffuseIntensity, 
    // and specularIntensity.
    
    vec3 N = normalize(Normal);
    
    if (hasNormalMap)
    {
        vec3 norm = texture(normalMap, TexCoord).rgb;
        norm = norm * 2.0 - 1.0;
        N = normalize(TBN * norm);
    }
    
    vec3 L = normalize(directionalLight);
    vec3 V = normalize(viewPos - FragWorldPos);
    vec3 R = reflect(-L, N);

    vec3 ambientIntensity = material.x * ambientColor;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuseIntensity = material.y * directionalColor * diff;

    float specStrength = texture(specMap, TexCoord).r;
    float spec = pow(max(dot(V, R), 0.0), material.w); 
    vec3 specularIntensity = material.z * directionalColor * spec;

    float shadow = ShadowCalculation(FragLightSpacePos);
    
    vec3 lightIntensity = ambientIntensity + ((1.0 - shadow) * (diffuseIntensity + specularIntensity));
    FragColor = vec4(lightIntensity, 1) * texture(baseTexture, TexCoord);
    
    
    // depth map rendering does not work properly
    // there should be gradients of depth on all objects but pins are all white even when they jump
    // when missiles come down they keep their original depth and stay black even though they hit the white pins
    
    //vec3 projCoords = FragLightSpacePos.xyz / FragLightSpacePos.w;

    //projCoords = (projCoords * 0.5) + 0.5;

    //float closestDepth = texture(shadowMap, projCoords.xy).r;
    //float currentDepth = projCoords.z;
    
    
    //FragColor = vec4(FragLightSpacePos.xyz / FragLightSpacePos.w, 1.0);
    //FragColor = vec4(vec3(shadow), 1.0);
    if (hasNormalMap)
    {
        //FragColor = vec4(vec3(0.5), 1.0);
    }
    //FragColor = vec4(vec3(specStrength), 1.0);
    //FragColor = vec4(vec3(closestDepth), 1.0);
    
}