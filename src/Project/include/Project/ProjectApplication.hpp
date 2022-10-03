#pragma once

#include <Project.Library/Application.hpp>

#include <Project/Shader.hpp>
#include <Project/Model.hpp>

#include <memory>

class ProjectApplication : public Application
{
protected:
    void AfterCreatedUiContext() override;
    void BeforeDestroyUiContext() override;
    bool Load() override;
    void RenderScene() override;
    void RenderUI() override;

private:
    std::unique_ptr<Shader> _shader;
    std::unique_ptr<Model> _frog;
};