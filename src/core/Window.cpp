#include "../core/Window.h"
#include <iostream>
#include <format>

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {
    void error_callback(int error, const char* desc) {
        std::cout << std::format("Error: {}", desc) << std::endl;
    }

    void imgui_init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplOpenGL3_Init(nullptr); // auto detect
        ImGui_ImplGlfw_InitForOpenGL(window, true);
    }

    void imgui_new_frame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void imgui_render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::init() {
    if (!glfwInit()) {
        std::cout << "glfw init failed" << std::endl;
        throw std::runtime_error("Failed to init GLFW");
    }
    glfwSetErrorCallback(error_callback);
}

void Window::create_window(int w, int h, const std::string &title, bool vsync) {
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    this->w = w;
    this->h = h;

    window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwDestroyWindow(window);
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    glfwSwapInterval(vsync ? 1 : 0);

    // imgui
    imgui_init(window);

    // OpenGL setup
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

}

bool Window::should_close() const {
    return glfwWindowShouldClose(window);
}

void Window::pollEvents() {
    // TODO: move to renderer.beginFrame()
    // and beginFrame control chunk, batching.., renderer.endFrame() will flush it.
    // update window size
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    glViewport(0,0, fb_w, fb_h);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwPollEvents();
    imgui_new_frame();
}

void Window::update() const {
    imgui_render();
    glfwSwapBuffers(window);
}

WindowSize Window::get_size() const {
    return {
        w,
        h,
        fb_w,
        fb_h
    };
}
