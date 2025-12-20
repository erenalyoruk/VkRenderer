#include "resource/model_loader.hpp"

#include <bit>
#include <cstring>

#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "logger.hpp"

namespace resource {
struct ModelLoader::Impl {
  rhi::Factory& factory;
  tinygltf::TinyGLTF loader;

  explicit Impl(rhi::Factory& f) : factory{f} {}

  std::optional<Model> LoadGLTF(const std::filesystem::path& path) {
    tinygltf::Model gltfModel;
    std::string err{};
    std::string warn{};

    bool success = false;
    if (path.extension() == ".glb") {
      success =
          loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path.string());
    } else {
      success =
          loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path.string());
    }

    if (!warn.empty()) {
      LOG_WARNING("glTF warning: {}", warn);
    }
    if (!err.empty()) {
      LOG_ERROR("glTF error: {}", err);
    }
    if (!success) {
      return std::nullopt;
    }

    Model model;
    model.name = path.stem().string();
    model.sourcePath = path.string();

    // Load textures
    LoadTextures(gltfModel, model);

    // Load materials
    LoadMaterials(gltfModel, model);

    // Load meshes
    LoadMeshes(gltfModel, model);

    // Load nodes
    LoadNodes(gltfModel, model);

    // Load lights (KHR_lights_punctual extension)
    LoadLights(gltfModel, model);

    // Load cameras
    LoadCameras(gltfModel, model);

    // Set root nodes
    if (!gltfModel.scenes.empty()) {
      int sceneIndex = gltfModel.defaultScene >= 0 ? gltfModel.defaultScene : 0;
      const auto& scene = gltfModel.scenes[sceneIndex];
      for (int nodeIndex : scene.nodes) {
        model.rootNodes.push_back(static_cast<uint32_t>(nodeIndex));
      }
    }

    return model;
  }

  void LoadTextures(const tinygltf::Model& gltf, Model& model) {
    for (const auto& gltfTexture : gltf.textures) {
      if (gltfTexture.source < 0) {
        continue;
      }

      const auto& image = gltf.images[gltfTexture.source];

      TextureResource tex;
      tex.name = image.name.empty()
                     ? "texture_" + std::to_string(model.textures.size())
                     : image.name;
      tex.width = static_cast<uint32_t>(image.width);
      tex.height = static_cast<uint32_t>(image.height);

      // Create RHI texture
      rhi::Format format = rhi::Format::R8G8B8A8Unorm;

      tex.texture = factory.CreateTexture(tex.width, tex.height, format,
                                          rhi::TextureUsage::Sampled);

      // Upload data
      if (!image.image.empty()) {
        std::span<const std::byte> data{
            std::bit_cast<const std::byte*>(image.image.data()),
            image.image.size()};
        tex.texture->Upload(data);
      }

      model.textures.push_back(std::move(tex));
    }
  }

  void LoadMaterials(const tinygltf::Model& gltf, Model& model) {
    (void)this;

    for (const auto& gltfMat : gltf.materials) {
      Material mat;
      mat.name = gltfMat.name;

      // PBR Metallic Roughness
      const auto& pbr = gltfMat.pbrMetallicRoughness;
      mat.baseColorFactor =
          glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
                    pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
      mat.metallicFactor = static_cast<float>(pbr.metallicFactor);
      mat.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

      if (pbr.baseColorTexture.index >= 0) {
        mat.baseColorTexture = pbr.baseColorTexture.index;
      }
      if (pbr.metallicRoughnessTexture.index >= 0) {
        mat.metallicRoughnessTexture = pbr.metallicRoughnessTexture.index;
      }

      // Normal map
      if (gltfMat.normalTexture.index >= 0) {
        mat.normalTexture = gltfMat.normalTexture.index;
      }

      // Occlusion
      if (gltfMat.occlusionTexture.index >= 0) {
        mat.occlusionTexture = gltfMat.occlusionTexture.index;
      }

      // Emissive
      if (gltfMat.emissiveTexture.index >= 0) {
        mat.emissiveTexture = gltfMat.emissiveTexture.index;
      }
      mat.emissiveFactor =
          glm::vec3(gltfMat.emissiveFactor[0], gltfMat.emissiveFactor[1],
                    gltfMat.emissiveFactor[2]);

      // Alpha mode
      if (gltfMat.alphaMode == "MASK") {
        mat.alphaMode = Material::AlphaMode::Mask;
        mat.alphaCutoff = static_cast<float>(gltfMat.alphaCutoff);
      } else if (gltfMat.alphaMode == "BLEND") {
        mat.alphaMode = Material::AlphaMode::Blend;
      }

      mat.doubleSided = gltfMat.doubleSided;

      model.materials.push_back(std::move(mat));
    }

    // Add default material if none exist
    if (model.materials.empty()) {
      Material defaultMat;
      defaultMat.name = "default";
      model.materials.push_back(defaultMat);
    }
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void LoadMeshes(const tinygltf::Model& gltf, Model& model) {
    for (const auto& gltfMesh : gltf.meshes) {
      Mesh mesh;
      mesh.name = gltfMesh.name;

      std::vector<ecs::Vertex> vertices;
      std::vector<uint32_t> indices;

      glm::vec3 minBounds{std::numeric_limits<float>::max()};
      glm::vec3 maxBounds{std::numeric_limits<float>::lowest()};

      for (const auto& primitive : gltfMesh.primitives) {
        if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
          LOG_WARNING("Skipping non-triangle primitive in mesh: {}", mesh.name);
          continue;
        }

        MeshPrimitive prim;
        prim.vertexOffset = static_cast<uint32_t>(vertices.size());
        prim.indexOffset = static_cast<uint32_t>(indices.size());
        prim.materialIndex = primitive.material;

        size_t vertexCount = 0;

        // Get position accessor info (required)
        const tinygltf::Accessor* posAccessor = nullptr;
        const uint8_t* posData = nullptr;
        size_t posStride = 0;

        if (auto it = primitive.attributes.find("POSITION");
            it != primitive.attributes.end()) {
          posAccessor = &gltf.accessors[it->second];
          const auto& bufferView = gltf.bufferViews[posAccessor->bufferView];
          const auto& buffer = gltf.buffers[bufferView.buffer];
          posData = buffer.data.data() + bufferView.byteOffset +
                    posAccessor->byteOffset;
          posStride = (bufferView.byteStride != 0U) ? bufferView.byteStride
                                                    : sizeof(float) * 3;
          vertexCount = posAccessor->count;

          // Update bounds
          if (posAccessor->minValues.size() >= 3) {
            minBounds =
                glm::min(minBounds, glm::vec3(posAccessor->minValues[0],
                                              posAccessor->minValues[1],
                                              posAccessor->minValues[2]));
          }
          if (posAccessor->maxValues.size() >= 3) {
            maxBounds =
                glm::max(maxBounds, glm::vec3(posAccessor->maxValues[0],
                                              posAccessor->maxValues[1],
                                              posAccessor->maxValues[2]));
          }
        } else {
          LOG_WARNING("Mesh primitive missing POSITION attribute: {}",
                      mesh.name);
          continue;
        }

        // Normal
        const uint8_t* normData = nullptr;
        size_t normStride = 0;
        if (auto it = primitive.attributes.find("NORMAL");
            it != primitive.attributes.end()) {
          const auto& accessor = gltf.accessors[it->second];
          const auto& bufferView = gltf.bufferViews[accessor.bufferView];
          const auto& buffer = gltf.buffers[bufferView.buffer];
          normData =
              buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
          normStride = (bufferView.byteStride != 0U) ? bufferView.byteStride
                                                     : sizeof(float) * 3;
        }

        // Texcoord
        const uint8_t* texData = nullptr;
        size_t texStride = 0;
        if (auto it = primitive.attributes.find("TEXCOORD_0");
            it != primitive.attributes.end()) {
          const auto& accessor = gltf.accessors[it->second];
          const auto& bufferView = gltf.bufferViews[accessor.bufferView];
          const auto& buffer = gltf.buffers[bufferView.buffer];
          texData =
              buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
          texStride = (bufferView.byteStride != 0U) ? bufferView.byteStride
                                                    : sizeof(float) * 2;
        }

        // Color (handle different types)
        const uint8_t* colorData = nullptr;
        size_t colorStride = 0;
        int colorComponentType = 0;
        int colorType = 0;  // VEC3 or VEC4
        if (auto it = primitive.attributes.find("COLOR_0");
            it != primitive.attributes.end()) {
          const auto& accessor = gltf.accessors[it->second];
          const auto& bufferView = gltf.bufferViews[accessor.bufferView];
          const auto& buffer = gltf.buffers[bufferView.buffer];
          colorData =
              buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
          colorComponentType = accessor.componentType;
          colorType = accessor.type;  // TINYGLTF_TYPE_VEC3 or VEC4

          size_t componentSize = 4;  // float
          if (colorComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            componentSize = 1;
          } else if (colorComponentType ==
                     TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            componentSize = 2;
          }
          size_t numComponents = (colorType == TINYGLTF_TYPE_VEC3) ? 3 : 4;
          colorStride = (bufferView.byteStride != 0U)
                            ? bufferView.byteStride
                            : componentSize * numComponents;
        }

        // Build vertices
        for (size_t i = 0; i < vertexCount; ++i) {
          ecs::Vertex v{};

          // Position
          const auto* pos =
              std::bit_cast<const float*>(posData + (i * posStride));
          v.position = glm::vec3(pos[0], pos[1], pos[2]);

          // Normal
          if (normData != nullptr) {
            const auto* norm =
                std::bit_cast<const float*>(normData + (i * normStride));
            v.normal = glm::vec3(norm[0], norm[1], norm[2]);
          } else {
            v.normal = glm::vec3(0.0F, 1.0F, 0.0F);  // Default up normal
          }

          // Texcoord
          if (texData != nullptr) {
            const auto* tex =
                std::bit_cast<const float*>(texData + (i * texStride));
            v.texCoord = glm::vec2(tex[0], tex[1]);
          }

          // Color
          if (colorData != nullptr) {
            const uint8_t* c = colorData + (i * colorStride);
            if (colorComponentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
              const auto* cf = std::bit_cast<const float*>(c);
              if (colorType == TINYGLTF_TYPE_VEC4) {
                v.color = glm::vec4(cf[0], cf[1], cf[2], cf[3]);
              } else {
                v.color = glm::vec4(cf[0], cf[1], cf[2], 1.0F);
              }
            } else if (colorComponentType ==
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
              if (colorType == TINYGLTF_TYPE_VEC4) {
                v.color = glm::vec4(static_cast<float>(c[0]) / 255.0F,
                                    static_cast<float>(c[1]) / 255.0F,
                                    static_cast<float>(c[2]) / 255.0F,
                                    static_cast<float>(c[3]) / 255.0F);
              } else {
                v.color = glm::vec4(static_cast<float>(c[0]) / 255.0F,
                                    static_cast<float>(c[1]) / 255.0F,
                                    static_cast<float>(c[2]) / 255.0F, 1.0F);
              }
            } else if (colorComponentType ==
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
              const auto* cs = std::bit_cast<const uint16_t*>(c);
              if (colorType == TINYGLTF_TYPE_VEC4) {
                v.color = glm::vec4(static_cast<float>(cs[0]) / 65535.0F,
                                    static_cast<float>(cs[1]) / 65535.0F,
                                    static_cast<float>(cs[2]) / 65535.0F,
                                    static_cast<float>(cs[3]) / 65535.0F);
              } else {
                v.color = glm::vec4(static_cast<float>(cs[0]) / 65535.0F,
                                    static_cast<float>(cs[1]) / 65535.0F,
                                    static_cast<float>(cs[2]) / 65535.0F, 1.0F);
              }
            }
          } else {
            v.color = glm::vec4(1.0F);
          }

          vertices.push_back(v);
        }

        prim.vertexCount = static_cast<uint32_t>(vertexCount);

        // Indices
        if (primitive.indices >= 0) {
          const auto& accessor = gltf.accessors[primitive.indices];
          const auto& bufferView = gltf.bufferViews[accessor.bufferView];
          const auto& buffer = gltf.buffers[bufferView.buffer];
          const uint8_t* data =
              buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

          prim.indexCount = static_cast<uint32_t>(accessor.count);

          for (size_t i = 0; i < accessor.count; ++i) {
            uint32_t index = 0;
            switch (accessor.componentType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                index = data[i];
                break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                index = std::bit_cast<const uint16_t*>(data)[i];
                break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                index = std::bit_cast<const uint32_t*>(data)[i];
                break;
              default:
                LOG_WARNING("Unsupported index component type in mesh: {}",
                            mesh.name);
                break;
            }

            indices.push_back(index);
          }
        }

        mesh.primitives.push_back(prim);
      }

      // Create GPU buffers (CPU-visible for simplicity)
      if (!vertices.empty()) {
        size_t vertexSize = vertices.size() * sizeof(ecs::Vertex);
        mesh.vertexBuffer = factory.CreateBuffer(
            vertexSize, rhi::BufferUsage::Vertex, rhi::MemoryUsage::CPUToGPU);
        void* mapped = mesh.vertexBuffer->Map();
        std::memcpy(mapped, vertices.data(), vertexSize);
        mesh.vertexBuffer->Unmap();
      }

      if (!indices.empty()) {
        size_t indexSize = indices.size() * sizeof(uint32_t);
        mesh.indexBuffer = factory.CreateBuffer(
            indexSize, rhi::BufferUsage::Index, rhi::MemoryUsage::CPUToGPU);
        void* mapped = mesh.indexBuffer->Map();
        std::memcpy(mapped, indices.data(), indexSize);
        mesh.indexBuffer->Unmap();
      }

      mesh.bounds.min = minBounds;
      mesh.bounds.max = maxBounds;

      model.meshes.push_back(std::move(mesh));
    }
  }

  void LoadNodes(const tinygltf::Model& gltf, Model& model) {
    (void)this;

    for (const auto& gltfNode : gltf.nodes) {
      SceneNode node;
      node.name = gltfNode.name;

      // Transform
      if (!gltfNode.translation.empty()) {
        node.translation =
            glm::vec3(gltfNode.translation[0], gltfNode.translation[1],
                      gltfNode.translation[2]);
      }
      if (!gltfNode.rotation.empty()) {
        node.rotation =
            glm::quat(static_cast<float>(gltfNode.rotation[3]),   // w
                      static_cast<float>(gltfNode.rotation[0]),   // x
                      static_cast<float>(gltfNode.rotation[1]),   // y
                      static_cast<float>(gltfNode.rotation[2]));  // z
      }
      if (!gltfNode.scale.empty()) {
        node.scale =
            glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
      }

      // Matrix (overrides TRS if present)
      if (!gltfNode.matrix.empty()) {
        glm::mat4 matrix{glm::make_mat4(gltfNode.matrix.data())};

        // Decompose matrix to TRS
        node.translation = glm::vec3(matrix[3]);
        node.scale = glm::vec3(glm::length(glm::vec3(matrix[0])),
                               glm::length(glm::vec3(matrix[1])),
                               glm::length(glm::vec3(matrix[2])));
        glm::mat3 rotMat(glm::vec3(matrix[0]) / node.scale.x,
                         glm::vec3(matrix[1]) / node.scale.y,
                         glm::vec3(matrix[2]) / node.scale.z);
        node.rotation = glm::quat_cast(rotMat);
      }

      node.meshIndex = gltfNode.mesh;
      node.cameraIndex = gltfNode.camera;

      for (int child : gltfNode.children) {
        node.children.push_back(static_cast<uint32_t>(child));
      }

      model.nodes.push_back(std::move(node));
    }
  }

  void LoadLights(const tinygltf::Model& gltf, Model& model) {
    (void)this;

    // KHR_lights_punctual extension
    if (auto it = gltf.extensions.find("KHR_lights_punctual");
        it != gltf.extensions.end()) {
      if (it->second.Has("lights") && it->second.Get("lights").IsArray()) {
        const auto& lights = it->second.Get("lights");
        for (size_t i = 0; i < lights.ArrayLen(); ++i) {
          const auto& gltfLight = lights.Get(static_cast<int>(i));

          Light light;
          if (gltfLight.Has("name")) {
            light.name = gltfLight.Get("name").Get<std::string>();
          }

          if (gltfLight.Has("type")) {
            std::string type = gltfLight.Get("type").Get<std::string>();
            if (type == "directional") {
              light.type = Light::Type::Directional;

            } else if (type == "point") {
              light.type = Light::Type::Point;

            } else if (type == "spot") {
              light.type = Light::Type::Spot;
            }
          }

          if (gltfLight.Has("color") && gltfLight.Get("color").IsArray()) {
            const auto& c = gltfLight.Get("color");
            light.color = glm::vec3(c.Get(0).GetNumberAsDouble(),
                                    c.Get(1).GetNumberAsDouble(),
                                    c.Get(2).GetNumberAsDouble());
          }

          if (gltfLight.Has("intensity")) {
            light.intensity = static_cast<float>(
                gltfLight.Get("intensity").GetNumberAsDouble());
          }

          if (gltfLight.Has("range")) {
            light.range =
                static_cast<float>(gltfLight.Get("range").GetNumberAsDouble());
          }

          model.lights.push_back(std::move(light));
        }
      }
    }
  }

  void LoadCameras(const tinygltf::Model& gltf, Model& model) {
    (void)this;

    for (const auto& gltfCamera : gltf.cameras) {
      CameraData cam;
      cam.name = gltfCamera.name;
      cam.perspective = (gltfCamera.type == "perspective");

      if (cam.perspective) {
        cam.yfov = static_cast<float>(gltfCamera.perspective.yfov);
        cam.aspectRatio =
            static_cast<float>(gltfCamera.perspective.aspectRatio);
        cam.znear = static_cast<float>(gltfCamera.perspective.znear);
        cam.zfar = static_cast<float>(gltfCamera.perspective.zfar);
      }

      model.cameras.push_back(std::move(cam));
    }
  }
};

ModelLoader::ModelLoader(rhi::Factory& factory)
    : impl_{std::make_unique<Impl>(factory)} {}

ModelLoader::~ModelLoader() = default;

std::optional<Model> ModelLoader::Load(const std::filesystem::path& path) {
  return impl_->LoadGLTF(path);
}

}  // namespace resource
