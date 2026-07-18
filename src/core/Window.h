#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct WindowSize {
    int w;
    int h;
    int fb_w;
    int fb_h;
};

class Window {
private:
    GLFWwindow* window = nullptr;
    int w;
    int h;
    int fb_w;
    int fb_h;

public:
    Window() = default;
    ~Window();

    void init();
    void create_window(int w, int h, const std::string& title, bool vsync = true);

    bool should_close() const;
    void pollEvents();
    void update() const ;

    WindowSize get_size() const;
    GLFWwindow* get() const {
        return window;
    }


};
