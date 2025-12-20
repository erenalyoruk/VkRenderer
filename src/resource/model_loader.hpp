#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "resource/types.hpp"
#include "rhi/factory.hpp"

namespace resource {

class ModelLoader {
 public:
  explicit ModelLoader(rhi::Factory& factory);
  ~ModelLoader();

  /**
   * @brief Load a glTF model from file.
   *
   * @param path Path to .gltf or .glb file
   * @return Loaded model or nullopt on failure
   */
  [[nodiscard]] std::optional<Model> Load(const std::filesystem::path& path);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace resource
