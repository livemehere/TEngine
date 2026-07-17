#pragma once

#include <string>
#include <glad/glad.h>

class Shader {
private:
    GLuint id_ = 0;
public:
    Shader(GLenum type, const std::string &filepath);
    ~Shader() {
        if (id_ != 0) {
            glDeleteShader(id_);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    GLuint getId() const { return id_; }
};
