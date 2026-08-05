#include "GpuProfiler.h"

#include <numeric>
#include <utility>

double GpuTimings::total() const {
    return std::accumulate(milliseconds.begin(), milliseconds.end(), 0.0);
}

GpuProfiler::Scope::Scope(GpuProfiler &profiler, const GpuPass pass)
    : profiler(profiler.beginPass(pass) ? &profiler : nullptr),
      pass(pass) {}

GpuProfiler::Scope::~Scope() {
    if (profiler) {
        profiler->endPass(pass);
    }
}

GpuProfiler::Scope::Scope(Scope &&other) noexcept
    : profiler(std::exchange(other.profiler, nullptr)),
      pass(other.pass) {}

GpuProfiler::GpuProfiler() {
    supported = glBeginQuery != nullptr &&
                glEndQuery != nullptr &&
                glGetQueryObjectui64v != nullptr;
    if (!supported) {
        return;
    }

    for (auto &passQueries : queries) {
        for (QueryPair &pair : passQueries) {
            glGenQueries(1, &pair.query);
        }
    }
}

GpuProfiler::~GpuProfiler() {
    if (!supported) {
        return;
    }
    for (auto &passQueries : queries) {
        for (QueryPair &pair : passQueries) {
            glDeleteQueries(1, &pair.query);
        }
    }
}

void GpuProfiler::beginFrame() {
    if (!supported) {
        return;
    }

    profiledPassThisFrame = (profiledPassThisFrame + 1) % PassCount;
    for (QueryPair &pair : queries[profiledPassThisFrame]) {
        if (!pair.pending) {
            continue;
        }

        GLint available = GL_FALSE;
        glGetQueryObjectiv(
            pair.query,
            GL_QUERY_RESULT_AVAILABLE,
            &available
        );
        if (available != GL_TRUE) {
            continue;
        }

        GLuint64 elapsedNanoseconds = 0;
        glGetQueryObjectui64v(
            pair.query,
            GL_QUERY_RESULT,
            &elapsedNanoseconds
        );
        const double milliseconds =
                static_cast<double>(elapsedNanoseconds) / 1'000'000.0;
        double &smoothed = timings.milliseconds[profiledPassThisFrame];
        smoothed = smoothed <= 0.0
                       ? milliseconds
                       : smoothed * 0.9 + milliseconds * 0.1;
        pair.pending = false;
    }

    currentFrameSlot = (currentFrameSlot + 1) % BufferedFrames;
    activePasses.fill(false);
    queryActive = false;
}

bool GpuProfiler::beginPass(const GpuPass pass) {
    if (!supported || queryActive) {
        return false;
    }
    const std::size_t passIndex = static_cast<std::size_t>(pass);
    if (passIndex != profiledPassThisFrame) {
        return false;
    }
    QueryPair &pair = queries[passIndex][currentFrameSlot];
    if (pair.pending || activePasses[passIndex]) {
        return false;
    }

    glBeginQuery(GL_TIME_ELAPSED, pair.query);
    activePasses[passIndex] = true;
    queryActive = true;
    return true;
}

void GpuProfiler::endPass(const GpuPass pass) {
    const std::size_t passIndex = static_cast<std::size_t>(pass);
    if (!supported || !activePasses[passIndex]) {
        return;
    }

    QueryPair &pair = queries[passIndex][currentFrameSlot];
    glEndQuery(GL_TIME_ELAPSED);
    pair.pending = true;
    activePasses[passIndex] = false;
    queryActive = false;
}
