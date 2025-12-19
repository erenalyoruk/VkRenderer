#include "rendering/rendering_system.hpp"

#include "rendering/components/mesh_renderer.hpp"
#include "rendering/components/transform.hpp"

namespace rendering {
void RenderingSystem::Render(entt::registry& registry, vk::CommandBuffer cmd,
                             BasicPipeline& pipeline) {
  (void)this;

  pipeline.Bind(cmd);
  registry.view<Transform, MeshRenderer>().each([&](const auto& transform,
                                                    const auto& meshRenderer) {
    cmd.bindVertexBuffers(0, meshRenderer.mesh->vertexBuffer.GetBuffer(), {0});
    cmd.bindIndexBuffer(meshRenderer.mesh->indexBuffer.GetBuffer(), 0,
                        vk::IndexType::eUint32);
    glm::mat4 modelMatrix{transform.GetMatrix()};
    cmd.pushConstants(pipeline.GetLayout(), vk::ShaderStageFlagBits::eVertex, 0,
                      sizeof(glm::mat4), &modelMatrix);
    cmd.drawIndexed(meshRenderer.mesh->indexCount, 1, 0, 0, 0);
  });
}
}  // namespace rendering
