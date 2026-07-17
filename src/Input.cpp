#include "Input.h"

#include <GLFW/glfw3.h>

Input::Input(Window &window) : window_(window) {}

void Input::update() {
    GLFWwindow* nativeWindow = window_.get();
    const Size size = window_.get_size();

    /* MOUSE POSITION */
    double x,y;
    glfwGetCursorPos(nativeWindow, &x, &y);
    mouseState_.x = (float)x * size.fb_w / size.w;
    mouseState_.y = (float)y * size.fb_h / size.h;
    if (firstMouseUpdate_) {
        mouseState_.deltaX = 0.0f;
        mouseState_.deltaY = 0.0f;
        mouseState_.prevX = mouseState_.x;
        mouseState_.prevY = mouseState_.y;
        firstMouseUpdate_ = false;
    } else {
        mouseState_.deltaX = mouseState_.x - mouseState_.prevX;
        mouseState_.deltaY = mouseState_.y - mouseState_.prevY;
        mouseState_.prevX = mouseState_.x;
        mouseState_.prevY = mouseState_.y;
    }

    /* MOUSE BUTTONS */
    const int leftBtnDown = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseState_.leftBtnPressed = leftBtnDown && !mouseState_.leftBtnDown;
    mouseState_.leftBtnDown = leftBtnDown;

    const int rightBtnDown = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseState_.rightBtnPressed = rightBtnDown && !mouseState_.rightBtnDown;
    mouseState_.rightBtnDown = rightBtnDown;


}
