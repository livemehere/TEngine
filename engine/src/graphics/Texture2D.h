#pragma once

#include <cstdint>
#include <span>
#include <string>

#include <glad/glad.h>

/* RGBA support only for simplicity */
class Texture2D {
   GLuint id = 0;
   int width = 0;
   int height = 0;
public:
   Texture2D(int width, int height, std::span<const uint8_t> pixels);
   Texture2D(const std::string& filepath);
   Texture2D(std::span<const std::uint8_t> encodedData);

   ~Texture2D() {
      if (id != 0) {
        glDeleteTextures(1, &id);
      }
   }

   Texture2D(const Texture2D&) = delete;
   Texture2D&operator=(const Texture2D&) = delete;

   void bind(GLuint slot = 0) const {
      glActiveTexture(GL_TEXTURE0 + slot);
      glBindTexture(GL_TEXTURE_2D, id);
  }

   static void unBind(GLuint slot = 0) {
      glActiveTexture(GL_TEXTURE0 + slot);
      glBindTexture(GL_TEXTURE_2D, 0);
  }
};
