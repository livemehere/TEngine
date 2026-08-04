#include "PostProcessor.h"

#include <stdexcept>

#include "../graphics/FrameBuffer.h"
#include "../graphics/Shader.h"
#include "../resources/ResourceManager.h"
#include "RenderSettings.h"

namespace {
    void restoreCapability(const GLenum capability, const GLboolean wasEnabled) {
        if (wasEnabled == GL_TRUE) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }
}

PostProcessor::PostProcessor(ResourceManager &resourceManager) : shader(resourceManager.getPostProcessShader()) {
    glGenVertexArrays(1, &vao);

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    shader.use();
    shader.setInt("uSceneTexture", 0);
    glUseProgram(previousProgram);
}

PostProcessor::~PostProcessor() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

void PostProcessor::render(
    const FrameBuffer& source,
    FrameBuffer& destination,
    const RenderSettings& settings
) {
    if (&source == &destination) {
        throw std::invalid_argument("Post-process source and destination must be different framebuffers");
    }

    destination.bind();

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasStencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask = GL_TRUE;
    GLint previousProgram = 0;
    GLint previousVertexArray = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousTexture2D = 0;

    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    const bool applyHdr =
            settings.hdrEnabled &&
            settings.debugView == DebugViewMode::Shaded;
    shader.setInt("uHdrEnabled", applyHdr ? 1 : 0);
    shader.setInt(
        "uToneMappingMode",
        static_cast<int>(settings.toneMapping)
    );
    shader.setFloat("uExposure", settings.exposure);
    shader.setInt("uGammaCorrectionEnabled", settings.gammaCorrection ? 1 : 0);
    shader.setFloat("uGamma", settings.gamma);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source.getTextureId());

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, previousTexture2D);
    glActiveTexture(previousActiveTexture);
    glBindVertexArray(previousVertexArray);
    glUseProgram(previousProgram);
    glDepthMask(previousDepthMask);
    restoreCapability(GL_DEPTH_TEST, wasDepthTestEnabled);
    restoreCapability(GL_STENCIL_TEST, wasStencilTestEnabled);
    restoreCapability(GL_CULL_FACE, wasCullFaceEnabled);
    restoreCapability(GL_BLEND, wasBlendEnabled);
}
