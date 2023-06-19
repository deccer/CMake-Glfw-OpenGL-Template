#pragma once
#include <cstdint>

struct GLFWwindow;

class Application
{
public:
    void Run();

protected:
    void Close();
    bool IsKeyPressed(int32_t key);
    
    double GetDeltaTime();

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

    //elapsed time between frames in seconds, used to allow framerate independent timing calculations
    double deltaTime;

};