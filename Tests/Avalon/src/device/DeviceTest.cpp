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
#include <Avalon/src/device/Device.h>
#include <Avalon/src/device/Instance.h>
#include <Avalon/src/window/Surface.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

TEST(DeviceTest, Initialization) {
  Device device;
  std::shared_ptr<Instance> instance;
  std::shared_ptr<Surface> surface;
  instance->init();

  device.pickPhysicalDevice(instance, surface);
  EXPECT_TRUE(true);
}
