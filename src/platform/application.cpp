#include "application.h"

#ifdef DEBUG
#include <iostream>
#endif

#include <cstring>
#include <stb/stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "math/types.h"
#include "engine/ui.h"
#include "gldebug.h"

int Application::activeApps = 0;

static u8 mouseButtonWasPressedFlags = 0;
static u8 mouseButtonUpdateFlags = 0;

static u64 keyPressedFlags[6] = { 0 };
static u64 keyUpdateFlags[6] = { 0 };

static f64 prevMouseX = 0.0;
static f64 prevMouseY = 0.0;

static void UpdateInputFlags()
{
    mouseButtonWasPressedFlags = mouseButtonUpdateFlags;
    memcpy(keyPressedFlags, keyUpdateFlags, sizeof(keyPressedFlags));
}

static bool Initialize()
{
    if (!Application::activeApps)
        return glfwInit() == GLFW_TRUE;

    return true;
}

static void CursorPositionCallback(GLFWwindow* window, f64 xpos, f64 ypos)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);

    app->mouseX = (xpos / app->screenWidth)  * app->refScreenWidth;
    app->mouseY = (ypos / app->screenHeight) * app->refScreenHeight;

    app->mousePositionCallback(*app);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);

    app->screenWidth  = width;
    app->screenHeight = height;

    f32 aspectRatio = (f32) width / (f32) height;
    app->refScreenWidth = aspectRatio * app->refScreenHeight;

    app->windowResizeCallback(*app);

    glViewport(0, 0, width, height);

    app->onRender(*app);
    glfwSwapBuffers(window);
}

static void ScrollCallback(GLFWwindow* window, f64 xoffset, f64 yoffset)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);
    app->scrollCallback(*app, xoffset, yoffset);
}

static void CharacterCallback(GLFWwindow* window, u32 codepoint)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);
    app->charCallback(*app, codepoint);
}

static void DropCallback(GLFWwindow* window, s32 count, const char** paths)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);
    app->dropCallback(*app, count, paths);
}

Application::Application(const char* title, int width, int height, bool fullscreen)
{
    if (!Initialize())
        return;

    glfwWindowHint(GLFW_SAMPLES, 4);

#   ifdef DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#   endif

    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    window = glfwCreateWindow(width, height, title, monitor, nullptr);
    if (!window)
        return;

    refScreenWidth = screenWidth = width;
    refScreenHeight = screenHeight = height;

    vsyncOn = false;
    deltaMouseX = deltaMouseY = 0.0;

    glfwMakeContextCurrent(window);
    
    onInit = onUpdate = onRender = windowResizeCallback = mousePositionCallback = [](Application&){};
    scrollCallback = [](Application&, f64, f64){};
    charCallback = [](Application&, u32){};
    dropCallback = [](Application&, s32, const char**){};

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, CursorPositionCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCharCallback(window, CharacterCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetDropCallback(window, DropCallback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        return;

    if (!gladLoadGL())
        return;

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);

#ifdef DEBUG
    // Check if debug output works
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                                0, nullptr, GL_TRUE);

        std::cout << "[OpenGL] Ready to debug..." << std::endl;
    }
#endif

    activeApps++;
}

Application::~Application()
{
    activeApps--;

    if (activeApps <= 0)
        glfwTerminate();
}


void Application::Run()
{
    UI::Init();

    onInit(*this);

    f64 prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        time = glfwGetTime();
        deltaTime = time - prevTime;

        deltaMouseX = mouseX - prevMouseX;
        deltaMouseY = prevMouseY - mouseY;

        prevMouseX = mouseX;
        prevMouseY = mouseY;

        onUpdate(*this);
        onRender(*this);

        glfwSwapBuffers(window);
        glfwPollEvents();

        UpdateInputFlags();

        prevTime = time;
    }

    UI::Shutdown();
}

void Application::Exit()
{
    glfwSetWindowShouldClose(window, 1);
}

bool Application::GetKey(int keyCode)
{
    return glfwGetKey(window, keyCode) == GLFW_PRESS;
}

bool Application::GetKeyDown(int keyCode)
{
    bool isPressed  = GetKey(keyCode);
    int keyIdx      = keyCode / 64;
    int keyFlagBit  = keyCode % 64;
    bool wasPressed = keyPressedFlags[keyIdx] & (1 << keyFlagBit);

    if (isPressed)
    {
        if (!wasPressed)
            keyUpdateFlags[keyIdx] |= 1 << keyFlagBit;
    }
    else
    {
        keyUpdateFlags[keyIdx] &= ~(1 << keyFlagBit);
    }

    return isPressed && !wasPressed;
}

bool Application::GetKeyUp(int keyCode)
{
    bool isPressed  = GetKey(keyCode);
    int keyIdx      = keyCode / 64;
    int keyFlagBit  = keyCode % 64;
    bool wasPressed = keyPressedFlags[keyIdx] & (1 << keyFlagBit);

    if (isPressed)
    {
        if (!wasPressed)
            keyUpdateFlags[keyIdx] |= 1 << keyFlagBit;
    }
    else
    {
        keyUpdateFlags[keyIdx] &= ~(1 << keyFlagBit);
    }

    return !isPressed && wasPressed;
}

bool Application::GetMouseButton(int mouseButton)
{
    return glfwGetMouseButton(window, mouseButton) == GLFW_PRESS;
}

bool Application::GetMouseButtonDown(int mouseButton)
{
    bool isPressed  = GetMouseButton(mouseButton);
    bool wasPressed = mouseButtonWasPressedFlags & (1 << mouseButton);

    if (isPressed)
    {
        if (!wasPressed)
            mouseButtonUpdateFlags |= 1 << mouseButton;
    }
    else
    {
        mouseButtonUpdateFlags &= ~(1 << mouseButton);
    }

    return isPressed && !wasPressed;
}

bool Application::GetMouseButtonUp(int mouseButton)
{
    bool isPressed  = GetMouseButton(mouseButton);
    bool wasPressed = mouseButtonWasPressedFlags & (1 << mouseButton);

    if (isPressed)
    {
        if (!wasPressed)
            mouseButtonUpdateFlags |= 1 << mouseButton;
    }
    else
    {
        mouseButtonUpdateFlags &= ~(1 << mouseButton);
    }

    return !isPressed && wasPressed;
}

void Application::SetWindowTitle(const char title[])
{
    glfwSetWindowTitle(window, title);
}

void Application::SetWindowIcon(const char filepath[])
{
    GLFWimage icon;
    int channels;

    icon.pixels = stbi_load(filepath, &icon.width, &icon.height, &channels, 4);

#   ifdef DEBUG
    if (!icon.pixels)
    {
        std::cout << "Failed to load icon.\n";
        return;
    }
#   endif // DEBUG

    glfwSetWindowIcon(window, 1, &icon);
    stbi_image_free(icon.pixels);
}

void Application::SetReferenceResolution(int resolution)
{
    f32 aspectRatio = (f32) screenWidth / (f32) screenHeight;

    refScreenHeight = resolution;
    refScreenWidth  = aspectRatio * resolution;
}

void Application::SetVsync(bool value)
{
    glfwSwapInterval((int) value);
}

void Application::SetCaptureMouse(bool value)
{
    int val = (value) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(window, GLFW_CURSOR, val);
}

void Application::SetClearColor(const Vector4& color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void Application::SetMaximize(bool value)
{
    if (value)
        glfwMaximizeWindow(window);
    else
        glfwRestoreWindow(window);
}