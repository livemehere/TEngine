#include "Texture2D.h"

Texture2D::Texture2D(int width, int height, std::span<const uint8_t> pixels) {
    if (const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4; pixels.size() != expectedSize) {
        throw std::invalid_argument("Invalid RGBA pixel data size");
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
}
