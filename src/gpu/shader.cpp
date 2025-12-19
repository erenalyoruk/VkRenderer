#include "gpu/shader.hpp"

#include <bit>
#include <fstream>

#include "logger.hpp"

namespace gpu {
std::vector<char> ReadFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    LOG_CRITICAL("Failed to open shader file: {}", path.string());
    throw std::runtime_error("Failed to open shader file: " + path.string());
  }

  std::streampos end{file.tellg()};
  if (end == std::streampos(-1)) {
    LOG_CRITICAL("Failed to determine size of shader file: {}", path.string());
    throw std::runtime_error("Failed to determine size of shader file: " +
                             path.string());
  }

  size_t fileSize{static_cast<size_t>(end)};
  if (fileSize == 0) {
    LOG_CRITICAL("Shader file is empty: {}", path.string());
    throw std::runtime_error("Shader file is empty: " + path.string());
  }

  LOG_DEBUG("Loading shader: {} ({} bytes)", path.string(), fileSize);

  std::vector<char> buffer(fileSize);

  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
  if (!file.good()) {
    LOG_CRITICAL("Failed to read shader file: {}", path.string());
    throw std::runtime_error("Failed to read shader file: " + path.string());
  }

  return buffer;
}

vk::UniqueShaderModule CreateShaderModule(vk::Device device,
                                          const std::vector<char>& code) {
  vk::ShaderModuleCreateInfo createInfo{
      .codeSize = code.size(),
      .pCode = std::bit_cast<const uint32_t*>(code.data()),
  };

  return device.createShaderModuleUnique(createInfo);
}

vk::UniqueShaderModule LoadShader(vk::Device device,
                                  const std::filesystem::path& path) {
  std::vector<char> code{ReadFile(path)};
  return CreateShaderModule(device, code);
}
}  // namespace gpu
