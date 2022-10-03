#pragma once

#include <Project.Library/Application.hpp>

class ProjectApplication : public Application
{
protected:
    void AfterCreatedUiContext() override;
    void BeforeDestroyUiContext() override;
    bool Load() override;
    void RenderScene() override;
    void RenderUI() override;
};