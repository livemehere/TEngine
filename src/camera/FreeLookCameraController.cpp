#include "FreeLookCameraController.h"

void FreeLookCameraController::update(Transform &transform, Input &input, float dt) {
    auto& mouseState = input.getMouseState();
    // cursor lock control
    if (mouseState.rightBtnDown) {
        input.setCursorLockState(true);
        // view rotation
        transform.rotation.y -= mouseState.deltaX * hSensitivity;
        transform.rotation.x -= mouseState.deltaY * vSensitivity;
        transform.rotation.x = glm::clamp(transform.rotation.x, -89.0f, 89.0f);
    } else {
        input.setCursorLockState(false);
    }

    if (!mouseState.lock) return;

    glm::vec2 moveInput{0.0f};
    if (input.isKeyDown(Key::A)) {
        moveInput.x -= 1.0f;
    }
    if (input.isKeyDown(Key::D)) {
        moveInput.x += 1.0f;
    }
    if (input.isKeyDown(Key::W)) {
        moveInput.y += 1.0f;
    }
    if (input.isKeyDown(Key::S)) {
        moveInput.y -= 1.0f;
    }
    if (glm::dot(moveInput, moveInput) > 0.0f) {
        moveInput = glm::normalize(moveInput);
    }

    float applySpeed = input.isKeyDown(Key::LeftShift) ? sprintSpeed : speed;

    transform.position += transform.getForward() * moveInput.y * applySpeed * dt;
    transform.position += transform.getRight() * moveInput.x * applySpeed * dt;

    // up, down
    if (input.isKeyDown(Key::E)) {
        transform.position += glm::vec3(0.0f, 1.0f,0.0f) * applySpeed * dt;
    }

    if (input.isKeyDown(Key::Q)) {
        transform.position += glm::vec3(0.0f, -1.0f,0.0f) * applySpeed * dt;
    }
}
