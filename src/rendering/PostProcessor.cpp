#include "PostProcessor.h"

PostProcessor::PostProcessor(ResourceManager &resourceManager) : shader(resourceManager.getPostProcessShader()) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
}

PostProcessor::~PostProcessor() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

void PostProcessor::render(GLuint textureId) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    shader.setInt("uSceneTexture", 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
}
