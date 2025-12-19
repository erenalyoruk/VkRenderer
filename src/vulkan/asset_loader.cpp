#include "vulkan/asset_loader.hpp"

#include <bit>
#include <cstdint>
#include <span>
#include <vector>

#include <tiny_gltf.h>

#include "logger.hpp"

namespace vulkan {
AssetLoader::AssetLoader(Context& context) : context_{context} {}

Mesh AssetLoader::LoadMesh(const std::string& filepath) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool ret{loader.LoadASCIIFromFile(&model, &err, &warn, filepath)};
  if (!warn.empty()) {
    LOG_WARNING("TinyGLTF warning: {}", warn);
  }

  if (!err.empty()) {
    LOG_ERROR("TinyGLTF error: {}", err);
  }

  if (!ret) {
    LOG_CRITICAL("Failed to load glTF file: {}", filepath);
    throw std::runtime_error("Failed to load glTF file: " + filepath);
  }

  auto& gltfMesh{model.meshes[0]};
  auto& primitive{gltfMesh.primitives[0]};

  std::vector<uint32_t> indices{};
  {
    int indexAccessor{primitive.indices};
    auto& accessor{model.accessors[indexAccessor]};
    auto& bufferView{model.bufferViews[accessor.bufferView]};
    auto& buffer{model.buffers[bufferView.buffer]};
    size_t offset{accessor.byteOffset + bufferView.byteOffset};
    size_t count{accessor.count};

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      const uint32_t* data{
          std::bit_cast<const uint32_t*>(&buffer.data[offset])};
      std::span<const uint32_t> span{data, count};
      indices.assign(span.begin(), span.end());
    } else if (accessor.componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      const uint16_t* data{
          std::bit_cast<const uint16_t*>(&buffer.data[offset])};
      std::span<const uint16_t> span{data, count};
      indices.reserve(count);
      for (uint16_t index : span) {
        indices.push_back(static_cast<uint32_t>(index));
      }
    }
  }

  std::vector<Vertex> vertices{};
  {
    int posAccessor{primitive.attributes["POSITION"]};
    auto& posAcc{model.accessors[posAccessor]};
    auto& posView{model.bufferViews[posAcc.bufferView]};
    auto& posBuffer{model.buffers[posView.buffer]};
    size_t posOffset{posAcc.byteOffset + posView.byteOffset};
    const float* posData{
        std::bit_cast<const float*>(&posBuffer.data[posOffset])};

    int normAccessor{primitive.attributes["NORMAL"]};
    auto& normAcc{model.accessors[normAccessor]};
    auto& normView{model.bufferViews[normAcc.bufferView]};
    auto& normBuffer{model.buffers[normView.buffer]};
    size_t normOffset{normAcc.byteOffset + normView.byteOffset};
    const float* normData{
        std::bit_cast<const float*>(&normBuffer.data[normOffset])};

    int texAccessor{primitive.attributes["TEXCOORD_0"]};
    auto& texAcc{model.accessors[texAccessor]};
    auto& texView{model.bufferViews[texAcc.bufferView]};
    auto& texBuffer{model.buffers[texView.buffer]};
    size_t texOffset{texAcc.byteOffset + texView.byteOffset};
    const float* texData{
        std::bit_cast<const float*>(&texBuffer.data[texOffset])};

    std::span<const float> posSpan{posData, posAcc.count * 3};
    std::span<const float> normSpan{normData, normAcc.count * 3};
    std::span<const float> texSpan{texData, texAcc.count * 2};

    size_t vertexCount{posAcc.count};
    vertices.reserve(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
      Vertex vertex{};
      vertex.position = {posSpan[i * 3], posSpan[(i * 3) + 1],
                         posSpan[(i * 3) + 2]};
      vertex.normal = {normSpan[i * 3], normSpan[(i * 3) + 1],
                       normSpan[(i * 3) + 2]};
      vertex.texCoord = {texSpan[i * 2], texSpan[(i * 2) + 1]};
      vertices.push_back(vertex);
    }
  }

  Buffer vertexBuffer{Buffer::Create(
      context_.GetAllocator(), vertices.size() * sizeof(Vertex),
      vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_AUTO,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
          VMA_ALLOCATION_CREATE_MAPPED_BIT)};
  vertexBuffer.Upload(std::span<const Vertex>{vertices});

  Buffer indexBuffer{Buffer::Create(
      context_.GetAllocator(), indices.size() * sizeof(uint32_t),
      vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_AUTO,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
          VMA_ALLOCATION_CREATE_MAPPED_BIT)};
  indexBuffer.Upload(std::span<const uint32_t>{indices});

  return {.vertexBuffer = std::move(vertexBuffer),
          .indexBuffer = std::move(indexBuffer),
          .indexCount = static_cast<uint32_t>(indices.size())};
}
}  // namespace vulkan
