#include "Texture2D.h"

Texture2D::Texture2D(int width, int height, std::span<const uint8_t> pixels) {

    const size_t expectedSize = width * height * 4;
    if (pixels.size() != expectedSize) {
        throw std::invalid_argument("Invalid RGBA pixel data size");
    }

    glGenTextures(1, &id_);
    bind();

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
