#pragma once

#include <bit>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#include "rhi/factory.hpp"
#include "rhi/shader.hpp"

namespace rhi {
/**
 * @brief Load SPIR-V bytecode from a file.
 * @param path Path to .spv file
 * @return SPIR-V bytecode or nullopt on failure
 */
inline std::optional<std::vector<uint32_t>> LoadSPIRV(
    const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return std::nullopt;
  }

  auto size = file.tellg();
  if (size <= 0 || size % sizeof(uint32_t) != 0) {
    return std::nullopt;
  }

  std::vector<uint32_t> spirv(static_cast<size_t>(size) / sizeof(uint32_t));
  file.seekg(0);
  file.read(std::bit_cast<char*>(spirv.data()), size);

  return spirv;
}

/**
 * @brief Load a shader from a SPIR-V file.
 * @param factory The RHI factory
 * @param path Path to .spv file
 * @param stage Shader stage
 * @return Shader or nullptr on failure
 */
inline std::unique_ptr<Shader> CreateShaderFromFile(
    Factory& factory, const std::filesystem::path& path, ShaderStage stage) {
  auto spirv = LoadSPIRV(path);
  if (!spirv) {
    return nullptr;
  }
  return factory.CreateShader(stage, *spirv);
}

/**
 * @brief Infer shader stage from file extension.
 * @param path Path to shader file
 * @return Shader stage or nullopt if unknown
 */
inline std::optional<ShaderStage> InferShaderStage(
    const std::filesystem::path& path) {
  std::string filename = path.stem().string();  // e.g., "simple.vert"

  if (filename.ends_with(".vert")) {
    return ShaderStage::Vertex;
  }
  if (filename.ends_with(".frag")) {
    return ShaderStage::Fragment;
  }
  if (filename.ends_with(".comp")) {
    return ShaderStage::Compute;
  }

  // Check extension of original file (before .spv)
  std::string ext = path.stem().extension().string();
  if (ext == ".vert") {
    return ShaderStage::Vertex;
  }
  if (ext == ".frag") {
    return ShaderStage::Fragment;
  }
  if (ext == ".comp") {
    return ShaderStage::Compute;
  }

  return std::nullopt;
}

/**
 * @brief Load a shader from file, inferring stage from filename.
 * @param factory The RHI factory
 * @param path Path to .spv file (e.g., "shader.vert.spv")
 * @return Shader or nullptr on failure
 */
inline std::unique_ptr<Shader> CreateShaderFromFile(
    Factory& factory, const std::filesystem::path& path) {
  auto stage = InferShaderStage(path);
  if (!stage) {
    return nullptr;
  }
  return CreateShaderFromFile(factory, path, *stage);
}
}  // namespace rhi
