#pragma once

#include <expected>
#include <fstream>
#include <string>
#include <sstream>

namespace utils {

    inline std::filesystem::path asset_path(
        const std::string &filepath) {
        return  std::filesystem::path(ASSET_ROOT) / filepath;
    }

    inline std::string read_file(const std::string& filepath) {
        std::ifstream file(filepath);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    inline GLuint compile_shader(const GLenum type, const std::string& filepath) {
        const std::string source_path = asset_path(filepath);
        const std::string source_string = read_file(source_path);
        const char* shader_source = source_string.c_str();

        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &shader_source, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            return shader;
        }

        std::string shader_type_str = type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";
        // failed case

        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length,'\0');

        GLsizei written = 0;
        glGetShaderInfoLog(shader, log_length, &written, log.data());
        log.resize(written);

        throw std::runtime_error(std::format("[{} SHADER] {}",shader_type_str, log));
    }

}