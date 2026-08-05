#include "TextRenderer.h"

#include <algorithm>
#include <cstddef>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/mat4x4.hpp>

#include "TextComponents.h"
#include "../../graphics/Font.h"
#include "../../graphics/Shader.h"
#include "../../resources/ResourceManager.h"
#include "../../scene/Scene.h"
#include "../../scene/TransformComponent.h"

TextRenderer::TextRenderer(ResourceManager &resources)
    : font(resources.getDefaultFont()),
      shader(resources.getTextShader()) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, texCoord))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void *>(offsetof(Vertex, color))
    );
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    shader.use();
    shader.setInt("uFontAtlas", 0);
    glUseProgram(previousProgram);
}

TextRenderer::~TextRenderer() {
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

void TextRenderer::appendText(
    std::vector<Vertex> &vertices,
    const char *text,
    const glm::vec3 &origin,
    const glm::vec3 &right,
    const glm::vec3 &up,
    const float scale,
    const glm::vec4 &color,
    const bool centered
) const {
    const auto measureLine = [&](const unsigned char *begin) {
        float width = 0.0f;
        for (const unsigned char *cursor = begin;
             *cursor != '\0' && *cursor != '\n';
             ++cursor) {
            width += static_cast<float>(font.getGlyph(*cursor).advance >> 6) *
                    scale;
        }
        return width;
    };
    const auto *firstCharacter =
            reinterpret_cast<const unsigned char *>(text);
    float cursorX = centered ? -measureLine(firstCharacter) * 0.5f : 0.0f;
    float cursorY = 0.0f;

    for (const unsigned char *cursor = firstCharacter;
         *cursor != '\0';
         ++cursor) {
        if (*cursor == '\n') {
            cursorX = centered ? -measureLine(cursor + 1) * 0.5f : 0.0f;
            cursorY -= static_cast<float>(font.getLineHeight()) * scale;
            continue;
        }

        const Glyph &glyph = font.getGlyph(*cursor);
        const float left = cursorX + static_cast<float>(glyph.bearing.x) * scale;
        const float bottom = cursorY -
                static_cast<float>(glyph.size.y - glyph.bearing.y) * scale;
        const float rightEdge = left + static_cast<float>(glyph.size.x) * scale;
        const float top = bottom + static_cast<float>(glyph.size.y) * scale;

        const glm::vec3 bottomLeft = origin + right * left + up * bottom;
        const glm::vec3 bottomRight = origin + right * rightEdge + up * bottom;
        const glm::vec3 topLeft = origin + right * left + up * top;
        const glm::vec3 topRight = origin + right * rightEdge + up * top;
        const float u0 = glyph.uvRect.x;
        const float v0 = glyph.uvRect.y;
        const float u1 = glyph.uvRect.z;
        const float v1 = glyph.uvRect.w;

        vertices.insert(vertices.end(), {
            {bottomLeft, {u0, v0}, color},
            {bottomRight, {u1, v0}, color},
            {topRight, {u1, v1}, color},
            {bottomLeft, {u0, v0}, color},
            {topRight, {u1, v1}, color},
            {topLeft, {u0, v1}, color}
        });

        cursorX += static_cast<float>(glyph.advance >> 6) * scale;
    }
}

bool TextRenderer::draw(
    const std::vector<Vertex> &vertices,
    const glm::mat4 &viewProjection,
    const bool depthTest
) {
    if (vertices.empty()) {
        return false;
    }

    const GLsizeiptr requiredBytes = static_cast<GLsizeiptr>(
        vertices.size() * sizeof(Vertex)
    );
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (static_cast<std::size_t>(requiredBytes) > bufferCapacity) {
        glBufferData(GL_ARRAY_BUFFER, requiredBytes, vertices.data(), GL_DYNAMIC_DRAW);
        bufferCapacity = static_cast<std::size_t>(requiredBytes);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, requiredBytes, vertices.data());
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shader.use();
    shader.setMat4("uViewProjection", viewProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.getAtlasTexture());
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    return true;
}

bool TextRenderer::renderWorld(
    const Scene &scene,
    const glm::mat4 &view,
    const glm::mat4 &projection
) {
    const glm::mat4 inverseView = glm::inverse(view);
    const glm::vec3 cameraRight = glm::normalize(glm::vec3(inverseView[0]));
    const glm::vec3 cameraUp = glm::normalize(glm::vec3(inverseView[1]));
    std::vector<Vertex> depthTested;
    std::vector<Vertex> alwaysVisible;

    scene.each<TransformComponent, WorldTextComponent>(
        [&](const Entity &entity,
            const TransformComponent &,
            const WorldTextComponent &text) {
            if (!text.enabled || text.text.empty() || text.scale <= 0.0f) {
                return;
            }
            const glm::mat4 worldMatrix = scene.getWorldMatrix(entity);
            const glm::vec3 origin = glm::vec3(
                worldMatrix * glm::vec4(text.localOffset, 1.0f)
            );
            std::vector<Vertex> &target = text.depthTest
                                              ? depthTested
                                              : alwaysVisible;
            appendText(
                target,
                text.text.c_str(),
                origin,
                cameraRight,
                cameraUp,
                text.scale,
                text.color,
                text.centered
            );
        }
    );

    const glm::mat4 viewProjection = projection * view;
    bool drew = draw(depthTested, viewProjection, true);
    drew = draw(alwaysVisible, viewProjection, false) || drew;
    return drew;
}

bool TextRenderer::renderCanvas(const Scene &scene, const RenderExtent extent) {
    std::vector<Vertex> vertices;
    scene.each<CanvasTextComponent>(
        [&](const CanvasTextComponent &text) {
            if (!text.enabled || text.text.empty() || text.scale <= 0.0f) {
                return;
            }
            appendText(
                vertices,
                text.text.c_str(),
                {text.position.x, text.position.y, 0.0f},
                {1.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
                text.scale,
                text.color,
                false
            );
        }
    );

    const glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(extent.width),
        0.0f,
        static_cast<float>(extent.height)
    );
    return draw(vertices, projection, false);
}
