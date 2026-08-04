#pragma once

#include <cstdint>

struct RenderStats {
    std::uint64_t drawCalls = 0;
    std::uint64_t instancedDrawCalls = 0;
    std::uint64_t instanceCount = 0;
    std::uint64_t triangleCount = 0;
    std::uint64_t shadowDrawCalls = 0;
    std::uint64_t shadowTriangleCount = 0;
};
