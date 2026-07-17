#include "Input.h"

#include <GLFW/glfw3.h>

Input::Input(Window &window) : window_(window) {}

void Input::update() {
    GLFWwindow* nativeWindow = window_.get();
    const Size size = window_.get_size();

    /* MOUSE POSITION */
    double x,y;
    glfwGetCursorPos(nativeWindow, &x, &y);
    mouseState_.screenX = (float)x * size.fb_w / size.w;
    mouseState_.screenY = (float)y * size.fb_h / size.h;
    if (firstMouseUpdate_) {
        mouseState_.deltaX = 0.0f;
        mouseState_.deltaY = 0.0f;
        mouseState_.prevScreenX = mouseState_.screenX;
        mouseState_.prevScreenY = mouseState_.screenY;
        firstMouseUpdate_ = false;
    } else {
        mouseState_.deltaX = mouseState_.screenX - mouseState_.prevScreenX;
        mouseState_.deltaY = mouseState_.screenY - mouseState_.prevScreenY;
        mouseState_.prevScreenX = mouseState_.screenX;
        mouseState_.prevScreenY = mouseState_.screenY;
    }

    /* MOUSE BUTTONS */
    const int leftBtnDown = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseState_.leftBtnPressed = leftBtnDown && !mouseState_.leftBtnDown;
    mouseState_.leftBtnDown = leftBtnDown;

    const int rightBtnDown = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseState_.rightBtnPressed = rightBtnDown && !mouseState_.rightBtnDown;
    mouseState_.rightBtnDown = rightBtnDown;


}

void Input::setCursorLockState(bool lock) {
    if (mouseState_.lock == lock) return;
    mouseState_.lock = lock;
    glfwSetInputMode(window_.get(), GLFW_CURSOR, lock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    firstMouseUpdate_ = true;
}
