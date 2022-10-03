#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 o_pixel;

layout (location = 0) in vec2 i_uvs;
layout (location = 1) in flat uint i_base_color_index;

layout (bindless_sampler) uniform;
layout (binding = 2) uniform Textures {
    uniform sampler2D[128] textures;
};

void main() {
    o_pixel = vec4(texture(textures[i_base_color_index], i_uvs).rgb, 1.0);
}