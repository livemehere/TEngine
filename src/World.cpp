#include "common.h"
#include "World.h"

#include <imgui.h>
#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>

#include "utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

World::World(Window& window, Input& input) : window(window), input(input) {
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);

    // shader
    program = utils::create_shader_program("shaders/basic.vert", "shaders/basic.frag");
    glUseProgram(program);
    modelLocation = glGetUniformLocation(program, "uModel");
    viewLocation = glGetUniformLocation(program, "uView");
    projectionLocation = glGetUniformLocation(program, "uProjection");


    // texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        textureW,
        textureH,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixel.data()
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // disable back-face
    // NOTE: if two size object needed, turn of culling
    // glEnable(GL_CULL_FACE);
    // glFrontFace(GL_CCW);
    // glCullFace(GL_BACK);

}

World::~World() {
    glDeleteProgram(program);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void World::update() {

    auto size = window.get_size();


    // for 2D
    // transform.position.x = size.fb_w/2;
    // transform.position.y = size.fb_h/2;
    // transform.scale.x = 200.0f;
    // transform.scale.y = 200.0f;

    // for 3D
    transform.position.z = -1.0f;

    /* MODEL */
    model = glm::mat4(1.0f);

    // position
    model = glm::translate(model, transform.position);

    // rotation

    model = glm::rotate(model,glm::radians(transform.rotation.x),glm::vec3(1.0f,0.0f,0.0f));
    model = glm::rotate(model,glm::radians(transform.rotation.y),glm::vec3(0.0f,1.0f,0.0f));
    model = glm::rotate(model,glm::radians(transform.rotation.z),glm::vec3(0.0f,0.0f,1.0f));

    // scale
    model = glm::scale(model, transform.scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

    /* VIEW */
    view = glm::mat4(1.0f);
    view = glm::translate(view, viewTransform.position);

    /* Euler */
    // view = glm::rotate(view,glm::radians(viewTransform.rotation.x),glm::vec3(1.0f,0.0f,0.0f));
    // view = glm::rotate(view,glm::radians(viewTransform.rotation.y),glm::vec3(0.0f,1.0f,0.0f));
    // view = glm::rotate(view,glm::radians(viewTransform.rotation.z),glm::vec3(0.0f,0.0f,1.0f));

    /* Quaternion */
    glm::vec3 eulerAngles(viewTransform.rotation);
    glm::quat viewQuat = glm::quat(glm::radians(eulerAngles));
    glm::mat4 viewQuatMat = glm::mat4_cast(viewQuat);
    view = view * viewQuatMat;


    view = glm::inverse(view);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

    /* PROJECTION */
   // projection = glm::ortho(0.0f,(float)size.fb_w,0.0f,(float)size.fb_h);
    projection = glm::perspective(glm::radians(45.0f),(float)size.fb_w/ (float)size.fb_h, 0.1f, 1000.0f);

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));


    /* INPUT HANDLES */
    auto native_window = window.get();

    static float lastFrameTime = 0;
    float currentFrameTime = (float)glfwGetTime();
    float deltaTime = (currentFrameTime - lastFrameTime);
    lastFrameTime = currentFrameTime;


    // view rotation
    static float hSensitivity = 0.05f;
    static float vSensitivity = 0.05f;
    auto& mouseState = input.getMouseState();
    viewTransform.rotation.y -= mouseState.deltaX * hSensitivity;
    viewTransform.rotation.x -= mouseState.deltaY * vSensitivity;


    // view movement
    static float moveSpeed = 2.0f;
    glm::vec3 inputVector(0.0f, 0.0f, 0.0f);
    if (glfwGetKey(native_window, GLFW_KEY_A) == GLFW_PRESS) {
       inputVector.x -= 1.0f;
    }
    if (glfwGetKey(native_window, GLFW_KEY_D) == GLFW_PRESS) {
        inputVector.x +=1.0f;
    }
    if (glfwGetKey(native_window, GLFW_KEY_W) == GLFW_PRESS) {
        inputVector.y +=1.0f;
    }
    if (glfwGetKey(native_window, GLFW_KEY_S) == GLFW_PRESS) {
        inputVector.y -=1.0f;
    }

    /* UP, DOWN */
    if (glfwGetKey(native_window, GLFW_KEY_E) == GLFW_PRESS) {
        viewTransform.position += glm::vec3(0.0f, 1.0f,0.0f) * moveSpeed * deltaTime;
    }

    if (glfwGetKey(native_window, GLFW_KEY_Q) == GLFW_PRESS) {
        viewTransform.position += glm::vec3(0.0f, -1.0f,0.0f) * moveSpeed * deltaTime;
    }

    if (glm::dot(inputVector, inputVector) > 0.0f) {
        inputVector = glm::normalize(inputVector);
    }

    viewForward = viewQuat * glm::vec3(0.0f, 0.0f, -1.0f);
    viewRight = viewQuat * glm::vec3(1.0f, 0.0f, 0.0f);

    viewTransform.position += viewForward * inputVector.y * moveSpeed * deltaTime;
    viewTransform.position += viewRight * inputVector.x * moveSpeed * deltaTime;

}

void World::render() {
    glUseProgram(program);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(),GL_UNSIGNED_INT,0);

    ImGui::Begin("Camera");
    ImGui::DragFloat3("position", glm::value_ptr(viewTransform.position), 1.0f);
    ImGui::DragFloat3("rotation", glm::value_ptr(viewTransform.rotation), 1.0f);
    ImGui::End();
}
