#pragma once

#include <cstdint>

namespace vulkan {
struct Material {
  uint32_t diffuseIndex;
  uint32_t normalIndex;
};
}  // namespace vulkan
