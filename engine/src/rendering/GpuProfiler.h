#pragma once

#include <array>
#include <cstddef>

#include <glad/glad.h>

enum class GpuPass : std::size_t {
    DirectionalShadow,
    PointShadow,
    Geometry,
    SSAO,
    DeferredLighting,
    Forward,
    Transparent,
    Outline,
    WorldText,
    FrameBufferDebug,
    Bloom,
    PostProcess,
    CanvasText,
    Count
};

struct GpuTimings {
    std::array<double, static_cast<std::size_t>(GpuPass::Count)> milliseconds{};

    [[nodiscard]] double get(GpuPass pass) const {
        return milliseconds[static_cast<std::size_t>(pass)];
    }

    [[nodiscard]] double total() const;
};

class GpuProfiler {
    static constexpr std::size_t BufferedFrames = 4;
    static constexpr std::size_t PassCount =
            static_cast<std::size_t>(GpuPass::Count);

    struct QueryPair {
        GLuint query = 0;
        bool pending = false;
    };

    std::array<std::array<QueryPair, BufferedFrames>, PassCount> queries{};
    std::array<bool, PassCount> activePasses{};
    GpuTimings timings;
    std::size_t currentFrameSlot = 0;
    std::size_t profiledPassThisFrame = 0;
    bool supported = false;
    bool queryActive = false;

    bool beginPass(GpuPass pass);
    void endPass(GpuPass pass);

public:
    class Scope {
        GpuProfiler *profiler = nullptr;
        GpuPass pass = GpuPass::DirectionalShadow;

    public:
        Scope(GpuProfiler &profiler, GpuPass pass);
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
        Scope(Scope &&other) noexcept;
        Scope &operator=(Scope &&) = delete;
    };

    GpuProfiler();
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler &) = delete;
    GpuProfiler &operator=(const GpuProfiler &) = delete;

    void beginFrame();
    [[nodiscard]] Scope profile(GpuPass pass) { return Scope(*this, pass); }
    [[nodiscard]] const GpuTimings &getTimings() const { return timings; }
};
