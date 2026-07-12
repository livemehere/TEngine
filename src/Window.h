#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class Window {
private:
    GLFWwindow* window = nullptr;

public:
    Window() = default;
    ~Window();

    void init();
    void create_window(int w, int h, const std::string& title, bool vsync = true);

    bool should_close() const;
    void before_update() const;
    void update() const;
};
