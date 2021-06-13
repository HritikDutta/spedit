#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "math/types.h"

#define KEY(x) GLFW_KEY_##x
#define MOUSE(x) GLFW_MOUSE_BUTTON_##x

struct Application
{
    Application(const char title[], int width, int height, bool fullscreen = false);
    ~Application();

    void Run();
    void Exit();
    
    bool GetKey(int keyCode);
    bool GetKeyDown(int keyCode);
    bool GetKeyUp(int keyCode);

    bool GetMouseButton(int mouseButton);
    bool GetMouseButtonDown(int mouseButton);
    bool GetMouseButtonUp(int mouseButton);

    void SetWindowTitle(const char title[]);
    void SetReferenceResolution(int resolution);
    void SetVsync(bool value);
    void SetWindowIcon(const char filepath[]);
    void SetCaptureMouse(bool value);
    void SetClearColor(const Vector4& color);
    void SetMaximize(bool value);

    GLFWwindow* window;
    s64 screenWidth, screenHeight;
    s64 refScreenWidth, refScreenHeight;
    bool vsyncOn;

    f64 mouseX, mouseY;
    f64 deltaMouseX, deltaMouseY;
    f64 time, deltaTime;

    void (*onInit)(Application& app);
    void (*onUpdate)(Application& app);
    void (*onRender)(Application& app);

    void (*mousePositionCallback)(Application& app);
    void (*windowResizeCallback)(Application& app);
    void (*scrollCallback)(Application& app, f64 scrollX, f64 scrollY);
    void (*charCallback)(Application& app, u32 codepoint);

    static int activeApps;
};