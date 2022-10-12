#version 460 core

layout (location = 0) out vec4 oPixel;

layout (location = 0) in vec2 iUvs;
layout (location = 1) in flat uint iBaseColorIndex;

layout (location = 2) uniform sampler2D[16] uTextures;

void main()
{
    oPixel = vec4(texture(uTextures[iBaseColorIndex], iUvs).rgb, 1.0);
}