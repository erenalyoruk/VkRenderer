#include "resource/resource_manager.hpp"

#include "logger.hpp"

namespace resource {
ResourceManager::ResourceManager(rhi::Factory& factory)
    : factory_{factory}, modelLoader_{factory} {}

Model* ResourceManager::LoadModel(const std::filesystem::path& path) {
  std::string key = path.string();

  // Check cache
  if (auto it = models_.find(key); it != models_.end()) {
    return it->second.get();
  }

  // Load
  auto model = modelLoader_.Load(path);
  if (!model) {
    LOG_ERROR("Failed to load model: {}", path.string());
    return nullptr;
  }

  auto* ptr = models_.emplace(key, std::make_unique<Model>(std::move(*model)))
                  .first->second.get();
  LOG_INFO("Loaded model: {} ({} meshes, {} materials, {} textures)",
           path.string(), ptr->meshes.size(), ptr->materials.size(),
           ptr->textures.size());
  return ptr;
}

Model* ResourceManager::GetModel(const std::string& name) {
  if (auto it = models_.find(name); it != models_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void ResourceManager::Clear() { models_.clear(); }
}  // namespace resource
