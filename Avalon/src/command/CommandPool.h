/*
 * Copyright 2024 Jan Filip
 *
 * Licensed under the MIT License. You may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AVALON_SRC_COMMAND_COMMANDPOOL_H_
#define AVALON_SRC_COMMAND_COMMANDPOOL_H_

#include <Avalon/src/device/Device.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

// Command pool on the graphics queue family. Buffers allocated from it are
// resettable individually (RESET_COMMAND_BUFFER_BIT).
class CommandPool {
 private:
  std::shared_ptr<Device> m_device;
  VkCommandPool m_pool;

 public:
  explicit CommandPool(std::shared_ptr<Device> device);
  CommandPool(const CommandPool&) = delete;
  CommandPool& operator=(const CommandPool&) = delete;
  CommandPool(CommandPool&&) = delete;
  CommandPool& operator=(CommandPool&&) = delete;
  ~CommandPool();

  void cleanUp();

  [[nodiscard]] VkCommandPool getPool() const { return m_pool; }

  // Allocates `count` primary command buffers. They are freed with the pool.
  [[nodiscard]] std::vector<VkCommandBuffer> allocate(uint32_t count) const;
  void free(const std::vector<VkCommandBuffer>& buffers) const;
};

#endif  // AVALON_SRC_COMMAND_COMMANDPOOL_H_
