#pragma once

#include <cstdint>

namespace rhi {
/**
 * @brief Pixel and buffer formats
 */
enum class Format : uint8_t {
  R8G8B8A8Unorm,       // 8-bit unsigned normalized RGBA
  R32G32B32A32Sfloat,  // 32-bit signed float RGBA
  D32Sfloat,           // 32-bit signed float depth
};

/**
 * @brief Memory usage patterns
 */
enum class MemoryUsage : uint8_t {
  GPUOnly,   // Memory only accessible by GPU
  CPUToGPU,  // Memory accessible by CPU and GPU (CPU writes, GPU reads)
  GPUToCPU,  // Memory accessible by GPU and CPU (GPU writes, CPU reads)
};

/**
 * @brief Type aliases for size and address representations
 */
using Size = uint64_t;
using Address = uint64_t;
}  // namespace rhi
