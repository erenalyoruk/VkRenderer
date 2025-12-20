#pragma once

#include <cstdint>

namespace rhi {
/**
 * @brief Pixel and buffer formats
 */
enum class Format : uint8_t {
  Undefined,
  // 8-bit formats
  R8Unorm,
  R8G8Unorm,
  R8G8B8Unorm,
  R8G8B8A8Unorm,
  R8G8B8A8Srgb,
  B8G8R8A8Unorm,
  B8G8R8A8Srgb,
  // 16-bit formats
  R16Sfloat,
  R16G16Sfloat,
  R16G16B16A16Sfloat,
  // 32-bit formats
  R32Sfloat,
  R32G32Sfloat,
  R32G32B32Sfloat,
  R32G32B32A32Sfloat,
  // Depth formats
  D16Unorm,
  D32Sfloat,
  D24UnormS8Uint,
  D32SfloatS8Uint,
};

/**
 * @brief Access flags for memory barriers
 */
enum class AccessFlags : uint8_t {
  None = 0,
  ShaderRead = 1 << 0,
  ShaderWrite = 1 << 1,
  IndirectCommandRead = 1 << 2,
  TransferRead = 1 << 3,
  TransferWrite = 1 << 4,
};

inline AccessFlags operator|(AccessFlags a, AccessFlags b) {
  return static_cast<AccessFlags>(static_cast<uint8_t>(a) |
                                  static_cast<uint8_t>(b));
}

inline AccessFlags operator&(AccessFlags a, AccessFlags b) {
  return static_cast<AccessFlags>(static_cast<uint8_t>(a) &
                                  static_cast<uint8_t>(b));
}

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
