#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
    GLuint id_ = 0;
public:
    Shader(const std::string& vsPath, const std::string& fsPath);
    ~Shader() {
        if (id_ != 0) {
            glDeleteProgram(id_);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    GLuint getId() const { return id_; }

    void use() const {
        glUseProgram(id_);
    }

    void setInt(const char* name, int value) const;
    void setVec4(const char *name, glm::vec4 value) const;
    void setMat4(const char *name, glm::mat4 value) const;
};
