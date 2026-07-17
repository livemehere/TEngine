#include "FreeLookCameraController.h"

void FreeLookCameraController::update(Camera &camera, Input &input, float dt) {
    auto& mouseState = input.getMouseState();
    // cursor lock control
    if (mouseState.leftBtnPressed) {
        input.setCursorLockState(true);
        return; // return current frame, prevent spark
    } else if (input.isKeyDown(Key::ESC)) {
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

    camera.transform.position += camera.getForward() * moveInput.y * applySpeed * dt;
    camera.transform.position += camera.getRight() * moveInput.x * applySpeed * dt;

    // up, down
    if (input.isKeyDown(Key::E)) {
        camera.transform.position += glm::vec3(0.0f, 1.0f,0.0f) * applySpeed * dt;
    }

    if (input.isKeyDown(Key::Q)) {
        camera.transform.position += glm::vec3(0.0f, -1.0f,0.0f) * applySpeed * dt;
    }


    // view rotation
    camera.transform.rotation.y -= mouseState.deltaX * hSensitivity;
    camera.transform.rotation.x -= mouseState.deltaY * vSensitivity;
    camera.transform.rotation.x = glm::clamp(camera.transform.rotation.x, -89.0f, 89.0f);


}
