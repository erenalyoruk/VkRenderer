#pragma once

#include <cstdint>

namespace gpu {
struct Material {
  uint32_t diffuseIndex;
  uint32_t normalIndex;
};
}  // namespace gpu
