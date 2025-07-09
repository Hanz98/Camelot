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

#ifndef AVALON_SRC_DEVICE_IMODEL_H_
#define AVALON_SRC_DEVICE_IMODEL_H_

#include <VkBootstrap.h>
#include <VkBootstrapDispatch.h>
#include <pch.h>

// Forward declaration for Instance namespace
namespace Instance {
class Model;
}

namespace Device {

class IModel {
public:
  virtual ~IModel() = default;
  virtual void cleanUp() = 0;
  virtual VkDevice& getDevice() = 0;
  virtual VkPhysicalDevice& getPhysicalDevice() = 0;
  virtual void PickPhysicalDevice(const Instance::Model& instance, const VkSurfaceKHR& surface) = 0;
};

}  // namespace Device

#endif  // AVALON_SRC_DEVICE_IMODEL_H_