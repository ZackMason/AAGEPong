#version 460 core
out vec4 FragColor;

in vec2 voPos;
in vec2 voUV;
flat in int voTexture;

uniform sampler2D uTextures[32];

void main() {

    FragColor = pow(texture(uTextures[voTexture], voUV), vec4(2.2));
}