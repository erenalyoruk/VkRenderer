#pragma once

#include <filesystem>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace vulkan {
std::vector<char> ReadFile(const std::filesystem::path& path);

vk::UniqueShaderModule CreateShaderModule(vk::Device device,
                                          const std::vector<char>& code);

vk::UniqueShaderModule LoadShader(vk::Device device,
                                  const std::filesystem::path& path);
}  // namespace vulkan
