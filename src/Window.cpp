#include "Window.h"
#include <iostream>
#include <format>

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
        exit(-1);
    }
    glfwSetErrorCallback(error_callback);
}

void Window::create_window(int w, int h, const std::string &title, bool vsync) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwDestroyWindow(window);
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSwapInterval(vsync ? 1 : 0);

    // imgui
    imgui_init(window);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(window);
}

void Window::before_update() const {
    glfwPollEvents();
    imgui_new_frame();
}

void Window::update() const {
    glClear(GL_COLOR_BUFFER_BIT);
    imgui_render();
    glfwSwapBuffers(window);
}

