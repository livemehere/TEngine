#include "Shader.h"

#include <format>
#include <initializer_list>
#include <stdexcept>
#include <glm/gtc/type_ptr.inl>

#include "ShaderStage.h"

namespace {
    GLuint linkShaderProgram(std::initializer_list<GLuint> shaderStages) {
        const GLuint program = glCreateProgram();
        for (const GLuint shaderStage : shaderStages) {
            glAttachShader(program, shaderStage);
        }

        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success != GL_FALSE) {
            return program;
        }

        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, '\0');

        GLsizei written = 0;
        glGetProgramInfoLog(program, logLength, &written, log.data());
        log.resize(written);
        glDeleteProgram(program);

        throw std::runtime_error(std::format("[SHADER_PROGRAM] {}", log));
    }

    void validate_location(const char* name, GLint loc) {
        if (loc == -1) {
            throw std::runtime_error(
                std::format(
                    "Uniform '{}' not found",
                    name
                    )
                );
        }
    }
}

Shader::Shader(const std::string &vsPath, const std::string &fsPath) {
    const ShaderStage vs(GL_VERTEX_SHADER,vsPath);
    const ShaderStage fs(GL_FRAGMENT_SHADER,fsPath);

    id = linkShaderProgram({vs.getId(), fs.getId()});

    glDetachShader(id, vs.getId());
    glDetachShader(id, fs.getId());
}

Shader::Shader(
    const std::string &vsPath,
    const std::string &gsPath,
    const std::string &fsPath
) {
    const ShaderStage vs(GL_VERTEX_SHADER, vsPath);
    const ShaderStage gs(GL_GEOMETRY_SHADER, gsPath);
    const ShaderStage fs(GL_FRAGMENT_SHADER, fsPath);

    id = linkShaderProgram({vs.getId(), gs.getId(), fs.getId()});

    glDetachShader(id, vs.getId());
    glDetachShader(id, gs.getId());
    glDetachShader(id, fs.getId());
}

void Shader::setInt(const char *name, const int value) const {
    const GLint loc = getUniformLocation(name);
    glUniform1i(loc, value);
}

void Shader::setFloat(const char *name, float value) const {
    const GLint loc = getUniformLocation(name);
    glUniform1f(loc, value);
}

void Shader::setVec3(const char *name, const glm::vec3 &value) const {
    const GLint loc = getUniformLocation(name);
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::setVec4(const char *name, const glm::vec4& value) const {
    const GLint loc = getUniformLocation(name);
    glUniform4fv(loc, 1, glm::value_ptr(value));
}

void Shader::setMat4(const char *name, const glm::mat4& value) const {
    const GLint loc = getUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

GLint Shader::getUniformLocation(const char *name) const {
    if (const auto it = uniformLocationCache.find(name); it != uniformLocationCache.end()) {
        return it->second;
    }

    const GLint location = glGetUniformLocation(id, name);
    validate_location(name, location);
    uniformLocationCache.emplace(name, location);
    return location;
}

void Shader::bindUniformBlock(const char *name, const GLuint bindingPoint) const {
    const GLuint blockIndex = glGetUniformBlockIndex(id, name);
    if (blockIndex == GL_INVALID_INDEX) {
        throw std::runtime_error(std::format("Uniform block '{}' not found", name));
    }
    glUniformBlockBinding(id, blockIndex, bindingPoint);
}
