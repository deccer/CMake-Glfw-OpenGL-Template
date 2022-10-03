#version 460 core

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec2 i_uv;
layout (location = 3) in vec4 i_tangent;

layout (location = 0) out vec2 o_uvs;
layout (location = 1) out flat uint o_base_color_index;

layout (location = 0) uniform mat4 u_projection;
layout (location = 1) uniform mat4 u_view;

struct ObjectData
{
    uint transform_index;
    uint base_color_index;
    uint normal_index;
};

layout (binding = 0) buffer BObjectData
{
    ObjectData[] object_data;
};

layout (binding = 1) buffer BTransforms
{
    mat4[] transforms;
};

void main()
{
    o_uvs = i_uv;
    o_base_color_index = object_data[gl_DrawID].base_color_index;
    gl_Position = u_projection * u_view * transforms[object_data[gl_DrawID].transform_index] * vec4(i_position, 1.0);
}