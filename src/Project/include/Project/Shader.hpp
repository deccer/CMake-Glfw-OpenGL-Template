#pragma once

#include <string_view>
#include <cstdint>

#include <glm/mat4x4.hpp>

class Shader
{
public:
    Shader(std::string_view vertex, std::string_view fragment);
    ~Shader();

    void Bind() const;
    void Set(uint32_t location, const glm::mat4& matrix) const;

private:
    uint32_t _program;
};