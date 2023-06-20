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
#include <set>

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
    if (!image->uri)
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

    if (!MakeShader("./data/shaders/main.vs.glsl", "./data/shaders/main.fs.glsl"))
    {
        return false;
    }

    LoadModel("./data/models/SM_Deccer_Cubes_Textured.gltf");

    return true;
}

void ProjectApplication::Update(float deltaTime)
{
    if (IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        Close();
    }

    _elapsedTime += deltaTime;
}

void ProjectApplication::RenderScene([[maybe_unused]] float deltaTime)
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
    struct BatchData
    {
        std::vector<ObjectData> objects;
        std::vector<MeshIndirectInfo> indirectCommands;
    };
    std::vector<BatchData> objectBatches(_cubes.Commands.size());
    std::vector<std::set<uint32_t>> textureHandles(_cubes.Commands.size());
    for (const auto& mesh : _cubes.Meshes)
    {
        const auto index = mesh.BaseColorTexture / 16;
        objectBatches[index].indirectCommands.emplace_back(MeshIndirectInfo
        {
            mesh.IndexCount,
            1,
            mesh.indexOffset,
            mesh.VertexOffset,
            1
        });
        objectBatches[index].objects.emplace_back(ObjectData
        {
            mesh.TransformIndex,
            mesh.BaseColorTexture % 16,
            mesh.NormalTexture
        });
        textureHandles[index].insert(_cubes.Textures[mesh.BaseColorTexture]);
    }

    glNamedBufferData(
        _cubes.TransformData,
        _cubes.Transforms.size() * sizeof(glm::mat4),
        _cubes.Transforms.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _cubes.TransformData);

    for (uint32_t index = 0; const auto& batch : objectBatches)
    {
        glNamedBufferData(
            _cubes.ObjectData[index],
            batch.objects.size() * sizeof(ObjectData),
            batch.objects.data(),
            GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _cubes.ObjectData[index]);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _cubes.Commands[index]);
        glNamedBufferData(
            _cubes.Commands[index],
            batch.indirectCommands.size() * sizeof(MeshIndirectInfo),
            batch.indirectCommands.data(),
            GL_DYNAMIC_DRAW);

        for (uint32_t offset = 0; const auto texture : textureHandles[index])
        {
            glUniform1i(2 + offset, offset);
            glActiveTexture(GL_TEXTURE0 + offset);
            glBindTexture(GL_TEXTURE_2D, texture);
            offset++;
        }
        glBindVertexArray(_cubes.InputLayout);
        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            batch.indirectCommands.size(),
            sizeof(MeshIndirectInfo));
        index++;
    }
}

void ProjectApplication::RenderUI(float deltaTime)
{
    ImGui::Begin("Window");
    {
        ImGui::TextUnformatted("Hello World!");
        ImGui::Text("Time in seconds since startup: %f", _elapsedTime);
        ImGui::Text("The delta time between frames: %f", deltaTime);
        ImGui::End();
    }

    ImGui::ShowDemoWindow();
}

bool ProjectApplication::MakeShader(std::string_view vertexShaderFilePath, std::string_view fragmentShaderFilePath)
{
    int success = false;
    char log[1024] = {};
    const auto vertexShaderSource = Slurp(vertexShaderFilePath);
    const char* vertexShaderSourcePtr = vertexShaderSource.c_str();
    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 1024, nullptr, log);
        spdlog::error(log);
        return false;
    }

    const auto fragmentShaderSource = Slurp(fragmentShaderFilePath);
    const char* fragmentShaderSourcePtr = fragmentShaderSource.c_str();
    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, log);
        spdlog::error(log);
        return false;
    }

    _shaderProgram = glCreateProgram();
    glAttachShader(_shaderProgram, vertexShader);
    glAttachShader(_shaderProgram, fragmentShader);
    glLinkProgram(_shaderProgram);
    glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(_shaderProgram, 1024, nullptr, log);
        spdlog::error(log);

        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

void ProjectApplication::LoadModel(std::string_view file)
{
    // Read GLTF
    cgltf_options options = {};
    cgltf_data* model = nullptr;
    cgltf_parse_file(&options, file.data(), &model);
    cgltf_load_buffers(&options, model, file.data());

    fs::path path(file.data());
    const auto basePath = path.parent_path();
    std::unordered_map<std::string, size_t> textureIds;
    _cubes.Textures.reserve(model->materials_count);
    const uint32_t maxBatches = model->materials_count / 16 + 1;
    for (uint32_t i = 0; i < model->materials_count; ++i)
    {
        const auto& material = model->materials[i];
        if (material.has_pbr_metallic_roughness && material.pbr_metallic_roughness.base_color_texture.texture != nullptr)
        {
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
            _cubes.Textures.emplace_back(texture);
            textureIds[texturePath] = _cubes.Textures.size() - 1;
        }
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
                            std::memcpy(&ptr->Position, positionPtr + v, sizeof(glm::vec3));
                        }
                        if (normalPtr)
                        {
                            std::memcpy(&ptr->Normal, normalPtr + v, sizeof(glm::vec3));
                        }
                        if (uvPtr)
                        {
                            std::memcpy(&ptr->Uv, uvPtr + v, sizeof(glm::vec2));
                        }
                        if (tangentPtr)
                        {
                            std::memcpy(&ptr->Tangent, tangentPtr + v, sizeof(glm::vec4));
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
                        case cgltf_component_type_r_8u:
                        {
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
                std::string baseColorURI = "";
                if (primitive.material->pbr_metallic_roughness.base_color_texture.texture != nullptr)
                {
                    baseColorURI = FindTexturePath(basePath, primitive.material->pbr_metallic_roughness.base_color_texture.texture->image);
                }
                else
                {
                    baseColorURI = "";
                }
                const auto indexCount = indices.size();
                meshCreateInfos.emplace_back(MeshCreateInfo
                {
                    std::move(vertices),
                    std::move(indices),
                    transformIndex++,
                    baseColorURI == "" ? 0 :(uint32_t)textureIds[baseColorURI],
                    0,
                    vertexOffset,
                    indexOffset,
                    _cubes.VertexBuffer,
                    _cubes.IndexBuffer
                });
                cgltf_node_transform_world(node, glm::value_ptr(_cubes.Transforms.emplace_back()));
                vertexOffset += vertexCount * sizeof(Vertex);
                indexOffset += indexCount * sizeof(uint32_t);
            }
            for (uint32_t j = 0; j < node->children_count; ++j)
            {
                nodes.push(node->children[j]);
            }
        }
    }
    _cubes.Commands.resize(maxBatches);
    _cubes.ObjectData.resize(maxBatches);

    // Allocate GL buffers
    glCreateVertexArrays(1, &_cubes.InputLayout);
    glCreateBuffers(1, &_cubes.VertexBuffer);
    glCreateBuffers(1, &_cubes.IndexBuffer);
    glCreateBuffers(1, &_cubes.TransformData);
    glGenBuffers(_cubes.Commands.size(), _cubes.Commands.data());
    glCreateBuffers(_cubes.ObjectData.size(), _cubes.ObjectData.data());

    size_t vertexSize = 0;
    size_t indexSize = 0;
    for (const auto& meshCreateInfo : meshCreateInfos)
    {
        vertexSize += meshCreateInfo.Vertices.size() * sizeof(Vertex);
        indexSize += meshCreateInfo.Indices.size() * sizeof(uint32_t);
    }

    glNamedBufferStorage(_cubes.VertexBuffer, vertexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(_cubes.IndexBuffer, indexSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glVertexArrayVertexBuffer(_cubes.InputLayout, 0, _cubes.VertexBuffer, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(_cubes.InputLayout, _cubes.IndexBuffer);

    glEnableVertexArrayAttrib(_cubes.InputLayout, 0);
    glEnableVertexArrayAttrib(_cubes.InputLayout, 1);
    glEnableVertexArrayAttrib(_cubes.InputLayout, 2);
    glEnableVertexArrayAttrib(_cubes.InputLayout, 3);

    glVertexArrayAttribFormat(_cubes.InputLayout, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position));
    glVertexArrayAttribFormat(_cubes.InputLayout, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
    glVertexArrayAttribFormat(_cubes.InputLayout, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, Uv));
    glVertexArrayAttribFormat(_cubes.InputLayout, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent));

    glVertexArrayAttribBinding(_cubes.InputLayout, 0, 0);
    glVertexArrayAttribBinding(_cubes.InputLayout, 1, 0);
    glVertexArrayAttribBinding(_cubes.InputLayout, 2, 0);
    glVertexArrayAttribBinding(_cubes.InputLayout, 3, 0);

    for (auto& info : meshCreateInfos)
    {
        info.VertexBuffer = _cubes.VertexBuffer;
        info.IndexBuffer = _cubes.IndexBuffer;
        glNamedBufferSubData(
            info.VertexBuffer,
            info.VertexOffset,
            info.Vertices.size() * sizeof(Vertex),
            info.Vertices.data());
        glNamedBufferSubData(
            info.IndexBuffer,
            info.IndexOffset,
            info.Indices.size() * sizeof(uint32_t),
            info.Indices.data());
        _cubes.Meshes.emplace_back(Mesh
        {
            (uint32_t)info.Indices.size(),
            (int32_t)(info.VertexOffset / sizeof(Vertex)),
            (uint32_t)(info.IndexOffset / sizeof(uint32_t)),
            info.TransformIndex,
            info.BaseColorTexture,
            info.NormalTexture
        });
    }
}
