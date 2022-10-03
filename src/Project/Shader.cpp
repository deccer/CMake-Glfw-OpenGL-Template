#include <Project/Shader.hpp>

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>

static std::string Slurp(std::string_view path)
{
    std::ifstream file(path.data(), std::ios::ate);
    std::string result(file.tellg(), '\0');
    file.seekg(0);
    file.read((char*)result.data(), result.size());
    return result;
}

Shader::Shader(std::string_view vertex, std::string_view fragment)
{
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
        glGetShaderInfoLog(vertexShader, 1024, NULL, log);
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
        glGetShaderInfoLog(fragmentShader, 1024, NULL, log);
        std::printf("%s\b", log);
    }

    _program = glCreateProgram();
    glAttachShader(_program, vertexShader);
    glAttachShader(_program, fragmentShader);
    glLinkProgram(_program);
    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(_program, 1024, NULL, log);
        std::printf("%s\b", log);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() = default;

void Shader::Bind() const
{
    glUseProgram(_program);
}

void Shader::Set(uint32_t location, const glm::mat4& matrix) const
{
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(matrix));
}
