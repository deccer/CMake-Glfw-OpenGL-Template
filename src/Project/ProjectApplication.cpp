#include <Project/ProjectApplication.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include <spdlog/spdlog.h>

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
    _shader = std::make_unique<Shader>("../../../shaders/main.vert", "../../../shaders/main.frag");
    _frog = std::make_unique<Model>("../../../models/frog/frog.gltf");

    return true;
}

void ProjectApplication::RenderScene()
{
    const auto projection = glm::perspective(glm::radians(80.0f), 1920.0f / 1080.0f, 0.1f, 256.0f);
    const auto view = glm::lookAt(
        glm::vec3(2 * std::sin(glfwGetTime() / 4), 2, 2 * std::cos(glfwGetTime() / 4)),
        glm::vec3(0, 0, -2),
        glm::vec3(0, 1, 0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _shader->Bind();
    _shader->Set(0, projection);
    _shader->Set(1, view);
    _frog->Draw();
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