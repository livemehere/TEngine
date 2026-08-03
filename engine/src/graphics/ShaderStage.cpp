#include "ShaderStage.h"

#include <format>
#include <stdexcept>

#include "../core/utils.h"

namespace {
    const char *getShaderStageName(GLenum type) {
        switch (type) {
            case GL_VERTEX_SHADER:
                return "VERTEX";
            case GL_GEOMETRY_SHADER:
                return "GEOMETRY";
            case GL_FRAGMENT_SHADER:
                return "FRAGMENT";
            default:
                return "UNKNOWN";
        }
    }
}

ShaderStage::ShaderStage(GLenum type, const std::string &filepath) {
    const std::string sourcePath = utils::asset_path(filepath);
    const std::string sourceString = utils::read_file(sourcePath);
    const char* shaderSource = sourceString.c_str();

    id = glCreateShader(type);
    glShaderSource(id, 1, &shaderSource, nullptr);
    glCompileShader(id);

    GLint success = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength,'\0');

        GLsizei written = 0;
        glGetShaderInfoLog(id, logLength, &written, log.data());
        log.resize(written);

        glDeleteShader(id);

        throw std::runtime_error(std::format(
            "[{} SHADER] {}",
            getShaderStageName(type),
            log
        ));
    }
}
