#include "BloomProcessor.h"

#include <algorithm>
#include <stdexcept>

#include "../graphics/Shader.h"
#include "../resources/ResourceManager.h"
#include "RenderSettings.h"
#include "GpuProfiler.h"

namespace {
    void restoreCapability(const GLenum capability, const GLboolean wasEnabled) {
        if (wasEnabled == GL_TRUE) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }

    void drawFullscreenTriangle(const GLuint vao) {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    RenderExtent scaledExtent(
        const RenderExtent extent,
        const float scale
    ) {
        const float clampedScale = std::clamp(scale, 0.125f, 1.0f);
        return {
            std::max(1, static_cast<int>(extent.width * clampedScale)),
            std::max(1, static_cast<int>(extent.height * clampedScale))
        };
    }
}

BloomProcessor::BloomProcessor(
    ResourceManager& resourceManager,
    GpuProfiler &gpuProfiler
)
    : extractShader(resourceManager.getBloomExtractShader()),
      blurShader(resourceManager.getGaussianBlurShader()),
      brightBuffer({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA16F
      }),
      blurBufferA({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA16F
      }),
      blurBufferB({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::RGBA16F
      }),
      gpuProfiler(gpuProfiler) {
    glGenVertexArrays(1, &vao);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

    extractShader.use();
    extractShader.setInt("uSceneTexture", 0);
    blurShader.use();
    blurShader.setInt("uImage", 0);

    glUseProgram(previousProgram);
}

BloomProcessor::~BloomProcessor() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

const FrameBuffer* BloomProcessor::process(
    const FrameBuffer& source,
    const RenderSettings& settings
) {
    if (!settings.bloomEnabled || settings.debugView != DebugViewMode::Shaded) {
        return nullptr;
    }
    if (source.isMultisampled()) {
        throw std::invalid_argument("Bloom source must be single-sampled");
    }
    auto gpuTiming = gpuProfiler.profile(GpuPass::Bloom);

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasStencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask = GL_TRUE;
    GLint previousFramebuffer = 0;
    GLint previousViewport[4] = {};
    GLint previousProgram = 0;
    GLint previousVertexArray = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTexture2D = 0;

    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);

    const RenderExtent extent = scaledExtent(
        source.getExtent(),
        settings.bloomResolutionScale
    );
    brightBuffer.resize(extent);
    blurBufferA.resize(extent);
    blurBufferB.resize(extent);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    brightBuffer.bind();
    glClear(GL_COLOR_BUFFER_BIT);
    extractShader.use();
    extractShader.setFloat("uThreshold", settings.bloomThreshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source.getTextureId());
    drawFullscreenTriangle(vao);

    const FrameBuffer* current = &brightBuffer;
    const int blurPasses = std::clamp(settings.bloomBlurPasses, 0, 64);
    blurShader.use();
    for (int pass = 0; pass < blurPasses; ++pass) {
        FrameBuffer& target = pass % 2 == 0
            ? blurBufferA
            : blurBufferB;

        target.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        blurShader.setInt("uHorizontal", pass % 2 == 0 ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, current->getTextureId());
        drawFullscreenTriangle(vao);
        current = &target;
    }

    glBindTexture(GL_TEXTURE_2D, previousTexture2D);
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
    restoreCapability(GL_DEPTH_TEST, wasDepthTestEnabled);
    restoreCapability(GL_STENCIL_TEST, wasStencilTestEnabled);
    restoreCapability(GL_CULL_FACE, wasCullFaceEnabled);
    restoreCapability(GL_BLEND, wasBlendEnabled);

    return current;
}
