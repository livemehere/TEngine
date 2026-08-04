#include "Texture2D.h"

#include <filesystem>
#include <format>
#include <limits>
#include <stdexcept>

#include "../thirdparty/stb_image.h"

namespace {
    GLenum getInternalFormat(const TextureColorSpace colorSpace) {
        return colorSpace == TextureColorSpace::SRGB
                   ? GL_SRGB8_ALPHA8
                   : GL_RGBA8;
    }
}

Texture2D::Texture2D(
    int width,
    int height,
    std::span<const uint8_t> pixels,
    const TextureColorSpace colorSpace
)
    : width(width), height(height) {
    if (const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        pixels.size() != expectedSize) {
        throw std::invalid_argument("Invalid RGBA pixel data size");
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     glTexParameteri(
    //     GL_TEXTURE_2D,
    //     GL_TEXTURE_WRAP_S,
    //     GL_REPEAT
    // );
    //
    //     glTexParameteri(
    //         GL_TEXTURE_2D,
    //         GL_TEXTURE_WRAP_T,
    //         GL_REPEAT
    //     );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        getInternalFormat(colorSpace),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
}

Texture2D::Texture2D(
    const std::string &filepath,
    const TextureColorSpace colorSpace
) {
    stbi_set_flip_vertically_on_load(true);

    int channels; // ignore
    unsigned char *data = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
    if (!data) {
        throw std::runtime_error(std::format("Texture load failed from '{}'", filepath.c_str()));
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     glTexParameteri(
    //     GL_TEXTURE_2D,
    //     GL_TEXTURE_WRAP_S,
    //     GL_REPEAT
    // );
    //
    //     glTexParameteri(
    //         GL_TEXTURE_2D,
    //         GL_TEXTURE_WRAP_T,
    //         GL_REPEAT
    //     );
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        getInternalFormat(colorSpace),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

Texture2D::Texture2D(
    std::span<const std::uint8_t> encodedData,
    const TextureColorSpace colorSpace
) {
    if (encodedData.empty()) {
        throw std::invalid_argument("Encoded texture data is empty");
    }
    if (encodedData.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Encoded texture data is too large");
    }

    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    unsigned char *data = stbi_load_from_memory(
        encodedData.data(),
        static_cast<int>(encodedData.size()),
        &width,
        &height,
        &channels,
        4
    );
    if (!data) {
        throw std::runtime_error("Failed to decode embedded texture");
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        getInternalFormat(colorSpace),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}
