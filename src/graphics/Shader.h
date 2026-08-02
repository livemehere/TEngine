#pragma once

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
    GLuint id = 0;
    mutable std::unordered_map<std::string, GLint> uniformLocationCache;

    GLint getUniformLocation(const char* name) const;

public:
    Shader(const std::string& vsPath, const std::string& fsPath);
    ~Shader() {
        if (id != 0) {
            glDeleteProgram(id);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    GLuint getId() const { return id; }

    void use() const {
        glUseProgram(id);
    }

    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec3(const char *name, const glm::vec3& value) const;
    void setVec4(const char *name, const glm::vec4& value) const;
    void setMat4(const char *name, const glm::mat4& value) const;

    void bindUniformBlock(const char* name, GLuint bindingPoint) const;
};
