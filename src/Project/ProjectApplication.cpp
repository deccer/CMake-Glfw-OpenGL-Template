#include <Project/ProjectApplication.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
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

    return true;
}

void ProjectApplication::RenderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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