#pragma once

struct GLFWwindow;

class Application
{
public:
    void Run();
protected:
    virtual void AfterCreatedUiContext();
    virtual void BeforeDestroyUiContext();
    virtual bool Initialize();
    virtual bool Load();
    virtual void Unload();
    virtual void RenderScene();
    virtual void RenderUI();
    virtual void Update();

private:
    GLFWwindow* _windowHandle = nullptr;
    void Render();
};