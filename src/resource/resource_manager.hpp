#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>

#include "resource/model_loader.hpp"
#include "resource/types.hpp"
#include "rhi/factory.hpp"

namespace resource {

class ResourceManager {
 public:
  explicit ResourceManager(rhi::Factory& factory);

  /**
   * @brief Load a model from file. Returns cached version if already loaded.
   */
  [[nodiscard]] Model* LoadModel(const std::filesystem::path& path);

  /**
   * @brief Get a previously loaded model.
   */
  [[nodiscard]] Model* GetModel(const std::string& name);

  /**
   * @brief Unload all resources.
   */
  void Clear();

 private:
  rhi::Factory& factory_;
  ModelLoader modelLoader_;
  std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};

}  // namespace resource
