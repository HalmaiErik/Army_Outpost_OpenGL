#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;

vec4 col;

void main()
{
    col = texture(skybox, textureCoordinates);
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    color = mix(fogColor, col, 0.3);
}
