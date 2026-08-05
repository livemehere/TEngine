#include "FrameBufferDebugRenderer.h"

#include <algorithm>

#include "../graphics/Shader.h"
#include "../resources/ResourceManager.h"

namespace {
void restoreCapability(const GLenum capability, const GLboolean enabled) {
    if (enabled == GL_TRUE) {
        glEnable(capability);
    } else {
        glDisable(capability);
    }
}
}

FrameBufferDebugRenderer::FrameBufferDebugRenderer(
    ResourceManager &resourceManager
)
    : shader(resourceManager.getFrameBufferDebugShader()),
      output({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA8
      }) {
    glGenVertexArrays(1, &vao);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    shader.use();
    shader.setInt("uTexture2D", 0);
    shader.setInt("uCubeTexture", 1);
    glUseProgram(previousProgram);
}

FrameBufferDebugRenderer::~FrameBufferDebugRenderer() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

bool FrameBufferDebugRenderer::render(const FrameBufferDebugInput &input) {
    available = false;
    if (input.mode == FrameBufferDebugView::Off ||
        input.texture2D == 0 ||
        input.cubeTexture == 0 ||
        input.extent.width <= 0 ||
        input.extent.height <= 0) {
        return false;
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasStencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask = GL_TRUE;
    GLboolean previousColorMask[4]{};
    GLint previousFramebuffer = 0;
    GLint previousViewport[4]{};
    GLint previousProgram = 0;
    GLint previousVertexArray = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTexture2D = 0;
    GLint previousCubeTexture = 0;

    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, previousColorMask);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousCubeTexture);

    output.resize(input.extent);
    output.bind();
    constexpr GLfloat clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shader.use();
    shader.setInt("uMode", static_cast<int>(input.mode));
    shader.setInt("uCubeFace", std::clamp(input.cubeFace, 0, 5));
    shader.setInt("uOrthographic", input.orthographic ? 1 : 0);
    shader.setFloat("uCameraNear", input.cameraNear);
    shader.setFloat("uCameraFar", input.cameraFar);
    shader.setFloat("uDepthRangeNear", input.depthRangeNear);
    shader.setFloat("uDepthRangeFar", input.depthRangeFar);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input.texture2D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, input.cubeTexture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousTexture2D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, previousCubeTexture);
    glActiveTexture(previousActiveTexture);
    glBindVertexArray(previousVertexArray);
    glUseProgram(previousProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(
        previousViewport[0],
        previousViewport[1],
        previousViewport[2],
        previousViewport[3]
    );
    glDepthMask(previousDepthMask);
    glColorMask(
        previousColorMask[0],
        previousColorMask[1],
        previousColorMask[2],
        previousColorMask[3]
    );
    restoreCapability(GL_DEPTH_TEST, wasDepthTestEnabled);
    restoreCapability(GL_STENCIL_TEST, wasStencilTestEnabled);
    restoreCapability(GL_CULL_FACE, wasCullFaceEnabled);
    restoreCapability(GL_BLEND, wasBlendEnabled);

    available = true;
    return true;
}
