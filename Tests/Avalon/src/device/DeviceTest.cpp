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
#include <Avalon/src/window/Window.h>
#include <VkBootstrap.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>

// Objects are declared in creation order so that the fixture destroys them
// in reverse: device -> surface -> instance -> window.
class DeviceTest : public testing::Test {
 protected:
  std::shared_ptr<Window> window;
  std::shared_ptr<Instance> instance;
  std::shared_ptr<Surface> surface;

  void SetUp() override {
    window = std::make_shared<Window>();
    ASSERT_NE(window->getWindow(), nullptr);
    instance = std::make_shared<Instance>();
    surface = std::make_shared<Surface>(instance, window);
    surface->init();
  }

  void TearDown() override {
    surface.reset();
    instance.reset();
    window.reset();
  }
};

TEST(InstanceTest, ConstructorCreatesInstance) {
  Instance instance;
  EXPECT_NE(instance.getInstance(), VK_NULL_HANDLE);
  EXPECT_NE(instance.getVkbInstance().instance, VK_NULL_HANDLE);

  instance.cleanUp();
  EXPECT_EQ(instance.getInstance(), VK_NULL_HANDLE);
  instance.cleanUp();  // idempotent
  EXPECT_EQ(instance.getInstance(), VK_NULL_HANDLE);
}

TEST(SurfaceTest, RejectsNullInstanceOrWindow) {
  auto window = std::make_shared<Window>();
  auto instance = std::make_shared<Instance>();

  Surface noInstance(nullptr, window);
  EXPECT_THROW(noInstance.init(), std::runtime_error);

  Surface noWindow(instance, nullptr);
  EXPECT_THROW(noWindow.init(), std::runtime_error);
}

TEST_F(DeviceTest, SurfaceIsCreated) {
  EXPECT_NE(surface->getSurface(), VK_NULL_HANDLE);

  surface->cleanUp();
  EXPECT_EQ(surface->getSurface(), VK_NULL_HANDLE);
}

TEST_F(DeviceTest, ConstructorRejectsNullArguments) {
  EXPECT_THROW(Device(nullptr, surface), std::runtime_error);
  EXPECT_THROW(Device(instance, nullptr), std::runtime_error);
}

TEST_F(DeviceTest, ConstructorCreatesDeviceWithQueues) {
  Device device(instance, surface);
  EXPECT_NE(device.getDevice(), VK_NULL_HANDLE);
  EXPECT_NE(device.getPhysicalDevice(), VK_NULL_HANDLE);

  auto graphics = device.getVkbDevice().get_queue(vkb::QueueType::graphics);
  ASSERT_TRUE(graphics.has_value()) << graphics.error().message();
  EXPECT_NE(graphics.value(), VK_NULL_HANDLE);

  auto present = device.getVkbDevice().get_queue(vkb::QueueType::present);
  ASSERT_TRUE(present.has_value()) << present.error().message();
  EXPECT_NE(present.value(), VK_NULL_HANDLE);

  EXPECT_NE(device.getDepthFormat(), VK_FORMAT_UNDEFINED);
  EXPECT_NE(device.getMaxUsableSampleCount(), 0);

  device.cleanUp();
  EXPECT_EQ(device.getDevice(), VK_NULL_HANDLE);
  device.cleanUp();  // idempotent
  EXPECT_EQ(device.getDevice(), VK_NULL_HANDLE);
}

TEST_F(DeviceTest, MoveTransfersOwnership) {
  Device device(instance, surface);
  VkDevice original = device.getDevice();
  ASSERT_NE(original, VK_NULL_HANDLE);

  Device moved(std::move(device));
  EXPECT_EQ(device.getDevice(), VK_NULL_HANDLE);
  EXPECT_EQ(moved.getDevice(), original);

  Device assigned(instance, surface);
  assigned = std::move(moved);
  EXPECT_EQ(moved.getDevice(), VK_NULL_HANDLE);
  EXPECT_EQ(assigned.getDevice(), original);
}
