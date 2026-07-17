#pragma once

#include <string>
#include <glad/glad.h>

class ShaderProgram {
private:
    GLuint id_ = 0;
public:
    ShaderProgram(const std::string& vsPath, const std::string& fsPath);
    ~ShaderProgram() {
        if (id_ != 0) {
            glDeleteProgram(id_);
        }
    }

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    void use() const {
        glUseProgram(id_);
    }

    GLuint getId() const { return id_; }
};
