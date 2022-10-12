#pragma once

#include <Project.Library/Application.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <string_view>
#include <vector>
#include <memory>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};

struct MeshIndirectInfo
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t baseVertex;
    uint32_t baseInstance;
};

struct MeshCreateInfo
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t transformIndex;
    uint32_t baseColorTexture;
    uint32_t normalTexture;
    size_t vertexOffset;
    size_t indexOffset;
    uint32_t vbo;
    uint32_t ibo;
};

struct Mesh
{
    uint32_t indexCount = 0;
    int32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    // NOT OpenGL handles, just indices
    uint32_t transformIndex = 0;
    uint32_t baseColorTexture = 0;
    uint32_t normalTexture = 0;
};

struct Model
{
    std::vector<Mesh> meshes;
    std::vector<uint32_t> textures;
    std::vector<glm::mat4> transforms;
    uint32_t vao;
    uint32_t vbo;
    uint32_t ibo;
    std::vector<uint32_t> cmds;
    std::vector<uint32_t> objectData;
    uint32_t transformData;
};

class ProjectApplication : public Application
{
protected:
    void AfterCreatedUiContext() override;
    void BeforeDestroyUiContext() override;
    bool Load() override;
    void RenderScene() override;
    void RenderUI() override;

    void MakeShader(std::string_view vertex, std::string_view fragment);
    void LoadModel(std::string_view file);

private:
    Model _frog;
    uint32_t _shaderProgram;
};