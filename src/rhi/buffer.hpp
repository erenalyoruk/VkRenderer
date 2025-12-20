#pragma once

#include <span>

#include "rhi/types.hpp"

namespace rhi {
/**
 * @brief Usage flags for buffers and textures
 */
enum class BufferUsage : uint8_t {
  Vertex = 1 << 0,       // Vertex buffer
  Index = 1 << 1,        // Index buffer
  Uniform = 1 << 2,      // Uniform buffer
  Storage = 1 << 3,      // Storage buffer
  TransferSrc = 1 << 4,  // Source for transfer operations
  TransferDst = 1 << 5,  // Destination for transfer operations
};

constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) {
  return static_cast<BufferUsage>(static_cast<uint8_t>(a) &
                                  static_cast<uint8_t>(b));
}

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
  return static_cast<BufferUsage>(static_cast<uint8_t>(a) |
                                  static_cast<uint8_t>(b));
}

/**
 * @brief Abstract base class for RHI buffer objects.
 */
class Buffer {
 public:
  virtual ~Buffer() = default;

  /**
   * @brief Maps the buffer memory for CPU access.
   *
   * @return void* Pointer to the mapped memory.
   */
  virtual void* Map() = 0;

  /**
   * @brief Unmaps the buffer memory.
   */
  virtual void Unmap() = 0;

  /**
   * @brief Uploads data to the buffer.
   *
   * @param data Data to upload.
   * @param offset Offset in the buffer to start uploading to.
   */
  virtual void Upload(std::span<const std::byte> data, Size offset = 0) = 0;

  /**
   * @brief Gets the size of the buffer.
   *
   * @return Size Size of the buffer in bytes.
   */
  [[nodiscard]] virtual Size GetSize() const = 0;

  /**
   * @brief Gets the device address of the buffer.
   *
   * @return Address Device address of the buffer.
   */
  [[nodiscard]] virtual Address GetDeviceAddress() const = 0;
};
}  // namespace rhi
