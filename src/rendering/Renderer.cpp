#include "Renderer.h"

/*layout (std140) uniform ExampleBlock
{
                     // base alignment  // aligned offset
    float value;     // 4               // 0
    vec3 vector;     // 16              // 16  (offset must be multiple of 16 so 4->16)
    mat4 matrix;     // 16              // 32  (column 0)
                     // 16              // 48  (column 1)
                     // 16              // 64  (column 2)
                     // 16              // 80  (column 3)
    float values[3]; // 16              // 96  (values[0])
                     // 16              // 112 (values[1])
                     // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;     // 4               // 148
}; */

Renderer::Renderer() {
   /* camera */
   glGenBuffers(1, &cameraUBO);
   glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUCameraData), nullptr, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, UniformBinding::Camera, cameraUBO);
   glBindBuffer(GL_UNIFORM_BUFFER,0);

   /* lights */
   glGenBuffers(1, &lightsUBO);
   glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(GPULightingData), nullptr, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, UniformBinding::Lights, lightsUBO);
   glBindBuffer(GL_UNIFORM_BUFFER,0);
}

void Renderer::updateCameraBuffer(Scene& scene, const WindowSize& windowSize) {
   glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
   const GPUCameraData data{
      .viewMatrix = scene.camera.getViewMatrix(),
      .projectionMatrix = scene.camera.getProjectionMatrix(windowSize),
      .position = glm::vec4(scene.camera.transform.position, 1.0f)
   };
   glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPUCameraData), &data);
   glBindBuffer(GL_UNIFORM_BUFFER,0);
}

void Renderer::updateLightsBuffer(Scene& scene) {
   glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
   GPULightingData data{};
   data.ambientLightColorIntensity = glm::vec4(scene.ambientLight.color, scene.ambientLight.intensity);
   data.lightCounts = glm::ivec4(
      0,
      static_cast<int>(scene.pointLights.size()),
      0,
      0
   );
   for (int i=0; i<scene.pointLights.size(); i++) {
      const PointLight& source = scene.pointLights[i];
      data.pointLights[i].colorIntensity = glm::vec4(source.color, source.intensity);
      data.pointLights[i].positionRange = glm::vec4(source.position, source.range);
   }

   glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPULightingData), &data);
   glBindBuffer(GL_UNIFORM_BUFFER,0);
}

void Renderer::beginFrame(Scene& scene, const WindowSize& windowSize) {
   updateCameraBuffer(scene, windowSize);
   updateLightsBuffer(scene);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_PROGRAM_POINT_SIZE);

   // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // render as wireframe
   // glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   // Back-face culling (enable for one-sided meshes)
   // NOTE: keep disabled when both sides of a surface must be visible.
   glEnable(GL_CULL_FACE);
   glFrontFace(GL_CCW);
   glCullFace(GL_BACK);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Renderer::render(const Scene &scene) {

   for (auto& meshObject : scene.meshObjects) {
      meshRenderer.render(meshObject);
   }
}

void Renderer::endFrame() {
}
