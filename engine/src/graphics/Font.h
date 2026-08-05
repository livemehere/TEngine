#pragma once

#include <array>
#include <cstdint>
#include <filesystem>

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

struct Glyph {
    glm::ivec2 size{0};
    glm::ivec2 bearing{0};
    std::uint32_t advance = 0;
    glm::vec4 uvRect{0.0f};
};

class Font {
    static constexpr unsigned int FirstCharacter = 32;
    static constexpr unsigned int LastCharacter = 126;

    GLuint atlasTexture = 0;
    std::array<Glyph, 128> glyphs{};
    int lineHeight = 0;

public:
    explicit Font(const std::filesystem::path &path, unsigned int pixelHeight = 48);
    ~Font();

    Font(const Font &) = delete;
    Font &operator=(const Font &) = delete;

    [[nodiscard]] const Glyph &getGlyph(unsigned char character) const;
    [[nodiscard]] float measureWidth(const char *text, float scale) const;
    [[nodiscard]] int getLineHeight() const { return lineHeight; }
    [[nodiscard]] GLuint getAtlasTexture() const { return atlasTexture; }
};
