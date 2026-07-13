#pragma once

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

        if (!file.is_open()) {
            throw std::runtime_error(std::format("Failed to open file : {}", filepath));
        }

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    inline GLuint compile_shader(const GLenum type, const std::string& filepath) {
        const std::string sourcePath = asset_path(filepath);
        const std::string sourceString = read_file(sourcePath);
        const char* shaderSource = sourceString.c_str();

        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &shaderSource, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) {
            return shader;
        }

        std::string shaderTypeStr = type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";
        // failed case

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength,'\0');

        GLsizei written = 0;
        glGetShaderInfoLog(shader, logLength, &written, log.data());
        log.resize(written);

        glDeleteShader(shader);

        throw std::runtime_error(std::format("[{} SHADER] {}",shaderTypeStr, log));
    }

    inline GLuint create_shader_program(const std::string& vsPath, const std::string& fsPath) {
        // TODO: convert shader to RAII to cleanup each stage.
        // if vs compile success and fs failed then vs doesn't cleanup it self.
        GLuint vs = compile_shader(GL_VERTEX_SHADER,vsPath);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER,fsPath);

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        glDeleteShader(vs);
        glDeleteShader(fs);

        if (success == GL_TRUE) {
            return program;
        }

        // failed case
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength,'\0');

        GLsizei written = 0;
        glGetProgramInfoLog(program, logLength, &written, log.data());
        log.resize(written);

        throw std::runtime_error(std::format("[SHADER_PROGRAM] {}", log));
    }

}