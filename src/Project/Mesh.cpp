#include <Project/Mesh.hpp>

#include <glad/glad.h>

Mesh::Mesh(const MeshCreateInfo& info)
{
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
    _indexCount = info.indices.size();
    _vertexOffset = info.vertexOffset / sizeof(Vertex);
    _indexOffset = info.indexOffset / sizeof(uint32_t);
    _transformIndex = info.transformIndex;
    _baseColorTexture = info.baseColorTexture;
    _normalTexture = info.normalTexture;
}

Mesh::~Mesh() = default;

MeshIndirectInfo Mesh::Info() const
{
    return
    {
        _indexCount,
        1,
        _indexOffset,
        _vertexOffset,
        1
    };
}

uint32_t Mesh::TransformIndex() const
{
    return _transformIndex;
}

uint32_t Mesh::BaseColorTexture() const
{
    return _baseColorTexture;
}

uint32_t Mesh::NormalTexture() const
{
    return _normalTexture;
}
