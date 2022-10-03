#pragma once

#include <Project/Mesh.hpp>

#include <string_view>

class Model
{
public:
    Model(std::string_view path);
    ~Model();

    void Draw() const;

    const std::vector<Mesh>& Meshes() const;
    const std::vector<glm::mat4>& Transforms() const;

private:
    std::vector<Mesh> _meshes;
    std::vector<uint32_t> _textures;
    std::vector<uint64_t> _textureHandles;
    std::vector<glm::mat4> _transforms;
    uint32_t _vao;
    uint32_t _vbo;
    uint32_t _ibo;
    uint32_t _cmds;
    uint32_t _objectData;
    uint32_t _transformData;
    uint32_t _textureData;
};