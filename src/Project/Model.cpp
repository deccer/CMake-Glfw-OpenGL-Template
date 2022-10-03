#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Project/Model.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <vector>
#include <queue>

GLuint64 (*APIENTRY glGetTextureHandleARB)(GLuint texture);
void (*APIENTRY glMakeTextureHandleResidentARB)(GLuint64 handle);

namespace fs = std::filesystem;

static std::string FindTexturePath(const fs::path& basePath, const cgltf_image* image)
{
    std::string texturePath;
    if (texturePath.empty())
    {
        auto newPath = basePath / image->name;
        if (!newPath.has_extension())
        {
            if (std::strcmp(image->mime_type, "image/png") == 0)
            {
                newPath.replace_extension("png");
            }
            else if (std::strcmp(image->mime_type, "image/jpg") == 0)
            {
                newPath.replace_extension("jpg");
            }
        }
        texturePath = newPath.generic_string();
    }
    else
    {
        texturePath = (basePath / image->uri).generic_string();
    }
    return texturePath;
}

Model::Model(std::string_view file)
{
    if (!glGetTextureHandleARB)
    {
        glGetTextureHandleARB = (GLuint64(*)(GLuint))glfwGetProcAddress("glGetTextureHandleARB");
    }
    if (!glMakeTextureHandleResidentARB)
    {
        glMakeTextureHandleResidentARB = (void(*)(GLuint64))glfwGetProcAddress("glMakeTextureHandleResidentARB");
    }

    // Read GLTF
    cgltf_options options = {};
    cgltf_data* model = nullptr;
    cgltf_parse_file(&options, file.data(), &model);
    cgltf_load_buffers(&options, model, file.data());

    fs::path path(file.data());
    const auto basePath = path.parent_path();
    std::unordered_map<std::string, size_t> textureIds;
    _textures.reserve(model->materials_count);
    for (uint32_t i = 0; i < model->materials_count; ++i)
    {
        const auto& material = model->materials[i];
        const auto* image = material.pbr_metallic_roughness.base_color_texture.texture->image;
        const auto texturePath = FindTexturePath(basePath, image);
        if (textureIds.contains(texturePath))
        {
            continue;
        }
        uint32_t texture;
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);

        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = STBI_rgb_alpha;
        const auto* textureData = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        const auto levels = (uint32_t)std::floor(std::log2(std::max(width, height)));
        glTextureStorage2D(texture, levels, GL_RGBA8, width, height);
        glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
        glGenerateTextureMipmap(texture);
        stbi_image_free((void*)textureData);
        _textures.emplace_back(texture);
        glMakeTextureHandleResidentARB(_textureHandles.emplace_back(glGetTextureHandleARB(texture)));
        textureIds[texturePath] = _textures.size() - 1;
    }

    uint32_t transformIndex = 0;
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    std::vector<MeshCreateInfo> meshCreateInfos;
    meshCreateInfos.reserve(1024);
    for (uint32_t i = 0; i < model->scene->nodes_count; ++i)
    {
        std::queue<cgltf_node*> nodes;
        nodes.push(model->scene->nodes[i]);
        while (!nodes.empty())
        {
            const auto* node = nodes.front();
            nodes.pop();
            if (!node->mesh)
            {
                for (uint32_t j = 0; j < node->children_count; ++j)
                {
                    nodes.push(node->children[j]);
                }
                continue;
            }
            for (uint32_t j = 0; j < node->mesh->primitives_count; ++j)
            {
                const auto& primitive = node->mesh->primitives[j];
                const glm::vec3* positionPtr = nullptr;
                const glm::vec3* normalPtr = nullptr;
                const glm::vec2* uvPtr = nullptr;
                const glm::vec4* tangentPtr = nullptr;
                uint64_t vertexCount = 0;
                // Vertices
                for (uint32_t k = 0; k < primitive.attributes_count; ++k)
                {
                    const auto& attribute = primitive.attributes[k];
                    const auto* accessor = attribute.data;
                    const auto* view = accessor->buffer_view;
                    const auto* dataPtr = (const char*)view->buffer->data;
                    switch (attribute.type)
                    {
                        case cgltf_attribute_type_position:
                            vertexCount = accessor->count;
                            positionPtr = (const glm::vec3*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_normal:
                            normalPtr = (const glm::vec3*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_texcoord:
                            uvPtr = (const glm::vec2*)(dataPtr + view->offset + accessor->offset);
                            break;

                        case cgltf_attribute_type_tangent:
                            tangentPtr = (const glm::vec4*)(dataPtr + view->offset + accessor->offset);
                            break;

                        default: break;
                    }
                }
                std::vector<Vertex> vertices;
                vertices.resize(vertexCount);
                {
                    auto* ptr = vertices.data();
                    for (uint32_t v = 0; v < vertexCount; ++v, ++ptr)
                    {
                        if (positionPtr)
                        {
                            std::memcpy(&ptr->position, positionPtr + v, sizeof(glm::vec3));
                        }
                        if (normalPtr)
                        {
                            std::memcpy(&ptr->normal, normalPtr + v, sizeof(glm::vec3));
                        }
                        if (uvPtr)
                        {
                            std::memcpy(&ptr->uv, uvPtr + v, sizeof(glm::vec2));
                        }
                        if (tangentPtr)
                        {
                            std::memcpy(&ptr->tangent, tangentPtr + v, sizeof(glm::vec4));
                        }
                    }
                }

                std::vector<uint32_t> indices;
                {
                    const auto* accessor = primitive.indices;
                    const auto* view = accessor->buffer_view;
                    const char* dataPtr = (const char*)view->buffer->data;
                    indices.reserve(accessor->count);
                    switch (accessor->component_type)
                    {
                        case cgltf_component_type_r_8:
                        case cgltf_component_type_r_8u: {
                            const auto* ptr = (const uint8_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        } break;

                        case cgltf_component_type_r_16:
                        case cgltf_component_type_r_16u:
                        {
                            const auto* ptr = (const uint16_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        } break;

                        case cgltf_component_type_r_32f:
                        case cgltf_component_type_r_32u:
                        {
                            const auto* ptr = (const uint32_t*)(dataPtr + view->offset + accessor->offset);
                            std::copy(ptr, ptr + accessor->count, std::back_inserter(indices));
                        } break;

                        default: break;
                    }
                }
                const auto baseColorURI = FindTexturePath(basePath, primitive.material->pbr_metallic_roughness.base_color_texture.texture->image);
                const auto indexCount = indices.size();
                meshCreateInfos.emplace_back(MeshCreateInfo
                {
                    std::move(vertices),
                    std::move(indices),
                    transformIndex++,
                    (uint32_t)textureIds[baseColorURI],
                    0,
                    vertexOffset,
                    indexOffset,
                    _vbo,
                    _ibo
                });
                cgltf_node_transform_world(node, glm::value_ptr(_transforms.emplace_back()));
                vertexOffset += vertexCount * sizeof(Vertex);
                indexOffset += indexCount * sizeof(uint32_t);
            }
            for (uint32_t j = 0; j < node->children_count; ++j)
            {
                nodes.push(node->children[j]);
            }
        }
    }
    // Allocate GL buffers
    glCreateVertexArrays(1, &_vao);
    glCreateBuffers(1, &_vbo);
    glCreateBuffers(1, &_ibo);
    glCreateBuffers(1, &_objectData);
    glCreateBuffers(1, &_transformData);
    glCreateBuffers(1, &_textureData);
    glGenBuffers(1, &_cmds);

    size_t vertexSize = 0;
    size_t indexSize = 0;
    for (const auto& info : meshCreateInfos)
    {
        vertexSize += info.vertices.size() * sizeof(Vertex);
        indexSize += info.indices.size() * sizeof(uint32_t);
    }

    glNamedBufferStorage(_vbo, vertexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(_ibo, indexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(_vao, _ibo);

    glEnableVertexArrayAttrib(_vao, 0);
    glEnableVertexArrayAttrib(_vao, 1);
    glEnableVertexArrayAttrib(_vao, 2);
    glEnableVertexArrayAttrib(_vao, 3);

    glVertexArrayAttribFormat(_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribFormat(_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribFormat(_vao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));

    glVertexArrayAttribBinding(_vao, 0, 0);
    glVertexArrayAttribBinding(_vao, 1, 0);
    glVertexArrayAttribBinding(_vao, 2, 0);
    glVertexArrayAttribBinding(_vao, 3, 0);

    for (auto& info : meshCreateInfos)
    {
        info.vbo = _vbo;
        info.ibo = _ibo;
        _meshes.emplace_back(info);
    }
}

Model::~Model() = default;

void Model::Draw() const
{
    struct ObjectData
    {
        uint32_t transformIndex;
        uint32_t baseColorIndex;
        uint32_t normalIndex;
    };
    std::vector<ObjectData> objectData;
    objectData.reserve(1024);
    std::vector<MeshIndirectInfo> indirectData;
    indirectData.reserve(1024);
    for (const auto& mesh : _meshes)
    {
        indirectData.emplace_back(mesh.Info());
        objectData.emplace_back(ObjectData
        {
            mesh.TransformIndex(),
            mesh.BaseColorTexture(),
            mesh.NormalTexture()
        });
    }
    glNamedBufferData(
        _textureData,
        _textureHandles.size() * sizeof(uint64_t),
        _textureHandles.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, _textureData);

    glNamedBufferData(
        _objectData,
        objectData.size() * sizeof(ObjectData),
        objectData.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _objectData);

    glNamedBufferData(
        _transformData,
        _transforms.size() * sizeof(glm::mat4),
        _transforms.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _transformData);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _cmds);
    glNamedBufferData(
        _cmds,
        indirectData.size() * sizeof(MeshIndirectInfo),
        indirectData.data(),
        GL_DYNAMIC_DRAW);

    glBindVertexArray(_vao);
    glMultiDrawElementsIndirect(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        nullptr,
        indirectData.size(),
        sizeof(MeshIndirectInfo));
}

const std::vector<Mesh>& Model::Meshes() const
{
    return _meshes;
}

const std::vector<glm::mat4>& Model::Transforms() const
{
    return _transforms;
}
