#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Project/ProjectApplication.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <vector>
#include <queue>

static GLuint64 (*APIENTRY glGetTextureHandleARB)(GLuint texture);
static void (*APIENTRY glMakeTextureHandleResidentARB)(GLuint64 handle);


static std::string Slurp(std::string_view path)
{
    std::ifstream file(path.data(), std::ios::ate);
    std::string result(file.tellg(), '\0');
    file.seekg(0);
    file.read((char*)result.data(), result.size());
    return result;
}

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

void ProjectApplication::AfterCreatedUiContext()
{
}

void ProjectApplication::BeforeDestroyUiContext()
{
}

bool ProjectApplication::Load()
{
    if (!Application::Load())
    {
        spdlog::error("App: Unable to load");
        return false;
    }
    MakeShader("../../../shaders/main.vert", "../../../shaders/main.frag");
    LoadModel("../../../models/frog/frog.gltf");

    return true;
}

void ProjectApplication::RenderScene()
{
    const auto projection = glm::perspective(glm::radians(80.0f), 1920.0f / 1080.0f, 0.1f, 256.0f);
    const auto view = glm::lookAt(
        glm::vec3(3 * std::cos(glfwGetTime() / 4), 2, -3 * std::sin(glfwGetTime() / 4)),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(_shaderProgram);
    glUniformMatrix4fv(0, 1, false, glm::value_ptr(projection));
    glUniformMatrix4fv(1, 1, false, glm::value_ptr(view));

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
    for (const auto& mesh : _frog.meshes)
    {
        indirectData.emplace_back(MeshIndirectInfo
        {
            mesh.indexCount,
            1,
            mesh.indexOffset,
            mesh.vertexOffset,
            1
        });
        objectData.emplace_back(ObjectData
        {
            mesh.transformIndex,
            mesh.baseColorTexture,
            mesh.normalTexture
        });
    }
    glNamedBufferData(
        _frog.textureData,
        _frog.textureHandles.size() * sizeof(uint64_t),
        _frog.textureHandles.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, _frog.textureData);

    glNamedBufferData(
        _frog.objectData,
        objectData.size() * sizeof(ObjectData),
        objectData.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _frog.objectData);

    glNamedBufferData(
        _frog.transformData,
        _frog.transforms.size() * sizeof(glm::mat4),
        _frog.transforms.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _frog.transformData);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _frog.cmds);
    glNamedBufferData(
        _frog.cmds,
        indirectData.size() * sizeof(MeshIndirectInfo),
        indirectData.data(),
        GL_DYNAMIC_DRAW);

    glBindVertexArray(_frog.vao);
    glMultiDrawElementsIndirect(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        nullptr,
        indirectData.size(),
        sizeof(MeshIndirectInfo));
}

void ProjectApplication::RenderUI()
{
    ImGui::Begin("Window");
    {
        ImGui::TextUnformatted("Hello World!");
        ImGui::End();
    }

    ImGui::ShowDemoWindow();
}

void ProjectApplication::MakeShader(std::string_view vertex, std::string_view fragment) {
    int success = false;
    char log[1024] = {};
    const auto vertexShaderSource = Slurp(vertex);
    const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 1024, nullptr, log);
        std::printf("%s\n", log);
    }

    const auto fragmentShaderSource = Slurp(fragment);
    const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();
    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, log);
        std::printf("%s\b", log);
    }

    _shaderProgram = glCreateProgram();
    glAttachShader(_shaderProgram, vertexShader);
    glAttachShader(_shaderProgram, fragmentShader);
    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(_shaderProgram, 1024, nullptr, log);
        std::printf("%s\b", log);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void ProjectApplication::LoadModel(std::string_view file) {
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
    _frog.textures.reserve(model->materials_count);
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
        _frog.textures.emplace_back(texture);
        glMakeTextureHandleResidentARB(_frog.textureHandles.emplace_back(glGetTextureHandleARB(texture)));
        textureIds[texturePath] = _frog.textures.size() - 1;
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
                    _frog.vbo,
                    _frog.ibo
                });
                cgltf_node_transform_world(node, glm::value_ptr(_frog.transforms.emplace_back()));
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
    glCreateVertexArrays(1, &_frog.vao);
    glCreateBuffers(1, &_frog.vbo);
    glCreateBuffers(1, &_frog.ibo);
    glCreateBuffers(1, &_frog.objectData);
    glCreateBuffers(1, &_frog.transformData);
    glCreateBuffers(1, &_frog.textureData);
    glGenBuffers(1, &_frog.cmds);

    size_t vertexSize = 0;
    size_t indexSize = 0;
    for (const auto& info : meshCreateInfos)
    {
        vertexSize += info.vertices.size() * sizeof(Vertex);
        indexSize += info.indices.size() * sizeof(uint32_t);
    }

    glNamedBufferStorage(_frog.vbo, vertexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(_frog.ibo, indexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayVertexBuffer(_frog.vao, 0, _frog.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(_frog.vao, _frog.ibo);

    glEnableVertexArrayAttrib(_frog.vao, 0);
    glEnableVertexArrayAttrib(_frog.vao, 1);
    glEnableVertexArrayAttrib(_frog.vao, 2);
    glEnableVertexArrayAttrib(_frog.vao, 3);

    glVertexArrayAttribFormat(_frog.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(_frog.vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribFormat(_frog.vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribFormat(_frog.vao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));

    glVertexArrayAttribBinding(_frog.vao, 0, 0);
    glVertexArrayAttribBinding(_frog.vao, 1, 0);
    glVertexArrayAttribBinding(_frog.vao, 2, 0);
    glVertexArrayAttribBinding(_frog.vao, 3, 0);

    for (auto& info : meshCreateInfos)
    {
        info.vbo = _frog.vbo;
        info.ibo = _frog.ibo;
        glNamedBufferSubData(
            info.vbo,
            info.vertexOffset,
            info.vertices.size() * sizeof(Vertex),
            info.vertices.data());
        glNamedBufferSubData(
            info.ibo,
            info.indexOffset,
            info.indices.size() * sizeof(uint32_t),
            info.indices.data());
        _frog.meshes.emplace_back(Mesh
        {
            (uint32_t)info.indices.size(),
            (int32_t)(info.vertexOffset / sizeof(Vertex)),
            (uint32_t)(info.indexOffset / sizeof(uint32_t)),
            info.transformIndex,
            info.baseColorTexture,
            info.normalTexture
        });
    }
}
