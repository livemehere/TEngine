#pragma once
#include "Window.h"


struct MouseState {
    float x;
    float y;
    float prevX;
    float prevY;
    float deltaX;
    float deltaY;
    bool leftBtnDown;
    bool rightBtnDown;
    bool leftBtnPressed;
    bool rightBtnPressed;
};

class Input {
    Window& window_;
    MouseState mouseState_{};
    bool firstMouseUpdate_ = true;

public:
    Input(Window &window);
    ~Input() = default;

    void update();
    const MouseState& getMouseState() const {
        return mouseState_;
    }
};
