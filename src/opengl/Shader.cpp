#include "Shader.h"

#include <format>
#include <stdexcept>

#include "../utils.h"

Shader::Shader(GLenum type, const std::string &filepath) {
    const std::string sourcePath = utils::asset_path(filepath);
    const std::string sourceString = utils::read_file(sourcePath);
    const char* shaderSource = sourceString.c_str();

    id_ = glCreateShader(type);
    glShaderSource(id_, 1, &shaderSource, nullptr);
    glCompileShader(id_);

    GLint success = GL_FALSE;
    glGetShaderiv(id_, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        std::string shaderTypeStr = type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";

        GLint logLength = 0;
        glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength,'\0');

        GLsizei written = 0;
        glGetShaderInfoLog(id_, logLength, &written, log.data());
        log.resize(written);

        glDeleteShader(id_);

        throw std::runtime_error(std::format("[{} SHADER] {}",shaderTypeStr, log));
    }
}
