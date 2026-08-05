#include "Font.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

Font::Font(const std::filesystem::path &path, const unsigned int pixelHeight) {
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        throw std::runtime_error("Failed to initialize FreeType");
    }

    FT_Face face = nullptr;
    const std::string pathString = path.string();
    if (FT_New_Face(library, pathString.c_str(), 0, &face) != 0) {
        FT_Done_FreeType(library);
        throw std::runtime_error(
            std::format("Failed to load font: {}", pathString)
        );
    }

    if (FT_Set_Pixel_Sizes(face, 0, pixelHeight) != 0) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        throw std::runtime_error(
            std::format("Failed to set font size: {}", pathString)
        );
    }

    int maxWidth = 0;
    int maxHeight = 0;
    for (unsigned int character = FirstCharacter;
         character <= LastCharacter;
         ++character) {
        if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0) {
            continue;
        }
        maxWidth = std::max(maxWidth, static_cast<int>(face->glyph->bitmap.width));
        maxHeight = std::max(maxHeight, static_cast<int>(face->glyph->bitmap.rows));
    }

    constexpr int columns = 16;
    constexpr int padding = 2;
    constexpr int characterCount = LastCharacter - FirstCharacter + 1;
    constexpr int rows = (characterCount + columns - 1) / columns;
    const int cellWidth = maxWidth + padding * 2;
    const int cellHeight = maxHeight + padding * 2;
    const int atlasWidth = cellWidth * columns;
    const int atlasHeight = cellHeight * rows;
    std::vector<unsigned char> atlasPixels(
        static_cast<std::size_t>(atlasWidth) * atlasHeight,
        0
    );

    for (unsigned int character = FirstCharacter;
         character <= LastCharacter;
         ++character) {
        if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0) {
            continue;
        }

        const FT_GlyphSlot slot = face->glyph;
        const FT_Bitmap &bitmap = slot->bitmap;
        const int index = static_cast<int>(character - FirstCharacter);
        const int cellX = (index % columns) * cellWidth + padding;
        const int cellY = (index / columns) * cellHeight + padding;

        for (unsigned int sourceY = 0; sourceY < bitmap.rows; ++sourceY) {
            const unsigned int targetY = bitmap.rows - sourceY - 1;
            const unsigned char *source = bitmap.buffer +
                    static_cast<std::ptrdiff_t>(sourceY) * bitmap.pitch;
            unsigned char *target = atlasPixels.data() +
                    static_cast<std::size_t>(cellY + targetY) * atlasWidth +
                    cellX;
            std::copy_n(source, bitmap.width, target);
        }

        glyphs[character] = {
            .size = {
                static_cast<int>(bitmap.width),
                static_cast<int>(bitmap.rows)
            },
            .bearing = {slot->bitmap_left, slot->bitmap_top},
            .advance = static_cast<std::uint32_t>(slot->advance.x),
            .uvRect = {
                static_cast<float>(cellX) / static_cast<float>(atlasWidth),
                static_cast<float>(cellY) / static_cast<float>(atlasHeight),
                static_cast<float>(cellX + bitmap.width) /
                    static_cast<float>(atlasWidth),
                static_cast<float>(cellY + bitmap.rows) /
                    static_cast<float>(atlasHeight)
            }
        };
    }

    lineHeight = static_cast<int>(face->size->metrics.height >> 6);

    GLint previousUnpackAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        atlasWidth,
        atlasHeight,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        atlasPixels.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

Font::~Font() {
    if (atlasTexture != 0) {
        glDeleteTextures(1, &atlasTexture);
    }
}

const Glyph &Font::getGlyph(const unsigned char character) const {
    if (character < FirstCharacter || character > LastCharacter) {
        return glyphs[static_cast<unsigned char>('?')];
    }
    return glyphs[character];
}

float Font::measureWidth(const char *text, const float scale) const {
    float currentWidth = 0.0f;
    float maximumWidth = 0.0f;
    for (const unsigned char *cursor =
             reinterpret_cast<const unsigned char *>(text);
         *cursor != '\0';
         ++cursor) {
        if (*cursor == '\n') {
            maximumWidth = std::max(maximumWidth, currentWidth);
            currentWidth = 0.0f;
            continue;
        }
        currentWidth += static_cast<float>(getGlyph(*cursor).advance >> 6) * scale;
    }
    return std::max(maximumWidth, currentWidth);
}
