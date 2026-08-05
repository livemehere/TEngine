#pragma once

#include <array>

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "../graphics/FrameBuffer.h"

class GBuffer;
class ResourceManager;
class Shader;
struct RenderSettings;

class SSAOProcessor {
    static constexpr std::size_t MaximumKernelSize = 64;

    GLuint vao = 0;
    GLuint noiseTexture = 0;
    const Shader& ssaoShader;
    const Shader& blurShader;
    FrameBuffer ssaoBuffer;
    FrameBuffer blurBuffer;
    std::array<glm::vec3, MaximumKernelSize> kernel{};

public:
    explicit SSAOProcessor(ResourceManager& resourceManager);
    ~SSAOProcessor();

    SSAOProcessor(const SSAOProcessor&) = delete;
    SSAOProcessor& operator=(const SSAOProcessor&) = delete;

    const FrameBuffer* process(
        const GBuffer& gBuffer,
        const RenderSettings& settings,
        RenderExtent extent
    );

    [[nodiscard]] GLuint getRawTextureId() const {
        return ssaoBuffer.getTextureId();
    }

    [[nodiscard]] GLuint getBlurredTextureId() const {
        return blurBuffer.getTextureId();
    }
};
