#include "Texture2D.h"

#include <filesystem>
#include <format>

#include "../thirdparty/stb_image.h"

Texture2D::Texture2D(int width, int height, std::span<const uint8_t> pixels) {
    if (const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4; pixels.size() != expectedSize) {
        throw std::invalid_argument("Invalid RGBA pixel data size");
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

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
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
}

Texture2D::Texture2D(const std::string& filepath) {
    stbi_set_flip_vertically_on_load(true);

    auto path = std::filesystem::path(ASSET_ROOT) / filepath;
    int channels; // ignore
    unsigned char *data = stbi_load(path.c_str(), &width_, &height_, &channels, 4);
    if (!data) {
        throw std::runtime_error(std::format("Texture load failed from '{}'", path.c_str()));
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
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
        GL_RGBA8,
        width_,
        height_,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

