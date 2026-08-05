#include "SSAOProcessor.h"

#include <algorithm>
#include <array>
#include <format>
#include <random>

#include <glm/geometric.hpp>

#include "../graphics/Shader.h"
#include "../resources/ResourceManager.h"
#include "GBuffer.h"
#include "RenderSettings.h"

namespace {
    void restoreCapability(const GLenum capability, const GLboolean enabled) {
        if (enabled == GL_TRUE) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }

    float lerp(const float from, const float to, const float amount) {
        return from + amount * (to - from);
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

SSAOProcessor::SSAOProcessor(ResourceManager& resourceManager)
    : ssaoShader(resourceManager.getSSAOShader()),
      blurShader(resourceManager.getSSAOBlurShader()),
      ssaoBuffer({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::R16F
      }),
      blurBuffer({
          .extent = {1, 1},
          .hasDepthStencil = false,
          .colorFormat = FrameBufferColorFormat::R16F
      }) {
    std::mt19937 generator(1337);
    std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);

    for (std::size_t index = 0; index < kernel.size(); ++index) {
        glm::vec3 sample{
            randomFloat(generator) * 2.0f - 1.0f,
            randomFloat(generator) * 2.0f - 1.0f,
            randomFloat(generator)
        };
        const float lengthSquared = glm::dot(sample, sample);
        if (lengthSquared > 0.000001f) {
            sample *= glm::inversesqrt(lengthSquared);
        } else {
            sample = {0.0f, 0.0f, 1.0f};
        }
        sample *= randomFloat(generator);

        const float normalizedIndex =
                static_cast<float>(index) /
                static_cast<float>(kernel.size());
        const float scale = lerp(
            0.1f,
            1.0f,
            normalizedIndex * normalizedIndex
        );
        kernel[index] = sample * scale;
    }

    std::array<float, 16 * 4> noise{};
    for (std::size_t index = 0; index < 16; ++index) {
        noise[index * 4 + 0] = randomFloat(generator) * 2.0f - 1.0f;
        noise[index * 4 + 1] = randomFloat(generator) * 2.0f - 1.0f;
        noise[index * 4 + 2] = 0.0f;
        noise[index * 4 + 3] = 1.0f;
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        4,
        4,
        0,
        GL_RGBA,
        GL_FLOAT,
        noise.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &vao);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    ssaoShader.use();
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("uNoiseTexture", 2);
    for (std::size_t index = 0; index < kernel.size(); ++index) {
        const std::string uniformName = std::format("uSamples[{}]", index);
        ssaoShader.setVec3(uniformName.c_str(), kernel[index]);
    }
    blurShader.use();
    blurShader.setInt("uSSAOInput", 0);
    glUseProgram(previousProgram);
}

SSAOProcessor::~SSAOProcessor() {
    if (noiseTexture != 0) {
        glDeleteTextures(1, &noiseTexture);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

const FrameBuffer* SSAOProcessor::process(
    const GBuffer& gBuffer,
    const RenderSettings& settings,
    const RenderExtent extent
) {
    const bool needsSSAO =
            settings.debugView == DebugViewMode::Shaded ||
            settings.debugView == DebugViewMode::SSAO ||
            settings.frameBufferDebugView == FrameBufferDebugView::SSAORaw ||
            settings.frameBufferDebugView ==
                FrameBufferDebugView::SSAOBlurred;
    if (!settings.ssaoEnabled || !needsSSAO) {
        return nullptr;
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasStencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask = GL_TRUE;
    GLint previousFramebuffer = 0;
    GLint previousViewport[4]{};
    GLint previousProgram = 0;
    GLint previousVertexArray = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
    std::array<GLint, 3> previousTextures{};

    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    for (GLuint slot = 0; slot < previousTextures.size(); ++slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextures[slot]);
    }

    const RenderExtent renderExtent = scaledExtent(
        extent,
        settings.ssaoResolutionScale
    );
    ssaoBuffer.resize(renderExtent);
    blurBuffer.resize(renderExtent);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    ssaoBuffer.bind();
    ssaoShader.use();
    ssaoShader.setInt(
        "uSampleCount",
        std::clamp(settings.ssaoSampleCount, 1, 64)
    );
    ssaoShader.setFloat(
        "uResolutionScale",
        std::clamp(settings.ssaoResolutionScale, 0.125f, 1.0f)
    );
    ssaoShader.setFloat("uRadius", std::max(settings.ssaoRadius, 0.001f));
    ssaoShader.setFloat("uBias", std::max(settings.ssaoBias, 0.0f));
    ssaoShader.setFloat("uPower", std::max(settings.ssaoPower, 0.01f));
    // The SSAO pass only reads position and normal. Reuse slot 2 for the
    // unused G-buffer attachments before binding the noise texture there.
    gBuffer.bindTextures(0, 1, 2, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    blurBuffer.bind();
    blurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoBuffer.getTextureId());
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    for (GLuint slot = 0; slot < previousTextures.size(); ++slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, previousTextures[slot]);
    }
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

    return &blurBuffer;
}
