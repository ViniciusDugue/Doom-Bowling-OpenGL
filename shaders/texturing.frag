#version 330
// A fragment shader for rendering a mesh that has a texture, but no lighting.
layout (location=0) out vec4 FragColor;

// Input from vertices: interpolated texture coordinate.
in vec2 TexCoord;

// Uniform from application: the texture sampler.
uniform sampler2D baseTexture;

void main() {
    vec4 texColor = texture(baseTexture, TexCoord);
    if (texColor.a == 0.0) { // This check might fail for textures without alpha channels
        texColor = vec4(1.0, 0.0, 0.0, 1.0);
    }

    FragColor = texture(baseTexture, TexCoord);
}