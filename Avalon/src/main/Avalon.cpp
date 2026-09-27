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

#include "Avalon/src/main/Avalon.h"

#include <chrono>
#include <iostream>
#include <memory>

#include "Avalon/pch.h"

namespace avalon {

Avalon::Avalon()
    : m_window(nullptr),
      m_instance(nullptr),
      m_surfaceManager(nullptr),
      m_device(nullptr),
      m_allocator(nullptr),
      m_swapchainModel(nullptr),
      m_renderer(nullptr) {}

Avalon::~Avalon() { cleanUp(); }

void Avalon::cleanUp() {
  // Reverse creation order: everything that lives on the device goes first.
  if (m_device) {
    m_device->waitIdle();
  }
  if (m_window) {
    m_window->setInputHooks({});  // the controller is about to go away
  }
  m_cameraController.reset();
  if (m_renderer) {
    m_renderer->setUi(nullptr);
  }
  m_ui.reset();
  m_renderer.reset();
  if (m_swapchainModel) {
    m_swapchainModel->cleanUp();
    m_swapchainModel = nullptr;
  }
  if (m_allocator != nullptr && m_allocator->allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator->allocator);
    m_allocator->allocator = VK_NULL_HANDLE;
  }
  m_allocator = nullptr;
  if (m_device) {
    m_device->cleanUp();
    m_device = nullptr;
  }
  if (m_surfaceManager) {
    m_surfaceManager->cleanUp();
    m_surfaceManager = nullptr;
  }
  if (m_instance) {
    m_instance->cleanUp();
    m_instance = nullptr;
  }
  if (m_window) {
    m_window->cleanUp();
    m_window = nullptr;
  }
}

void Avalon::init() {
  try {
    m_window = std::make_shared<Window>();  // TO DO: Make window size and title
                                            // read from settings
    m_instance = std::make_shared<Instance>();
    m_surfaceManager = std::make_shared<SurfaceManager>(m_instance, m_window);
    m_device =
        std::make_shared<Device>(m_instance, m_surfaceManager->getSurface());
    initVma();
    m_swapchainModel =
        std::make_shared<SwapchainModel>(m_device, m_window, m_allocator);
    m_renderer = std::make_unique<Renderer>(m_device, m_window,
                                            m_swapchainModel, m_allocator);
    m_cameraController =
        std::make_unique<CameraController>(&m_renderer->getCamera());
    m_cameraController->attach(*m_window);
    UiInitInfo uiInfo;
    uiInfo.instance = m_instance;
    uiInfo.device = m_device;
    uiInfo.window = m_window;
    uiInfo.renderPass = m_renderer->getRenderPass().getRenderPass();
    uiInfo.samples = m_renderer->getRenderPass().getSamples();
    uiInfo.minImageCount = Renderer::kFramesInFlight;
    uiInfo.imageCount = m_swapchainModel->getImageCount();
    m_ui = std::make_unique<UiContext>(uiInfo);
    m_renderer->setUi(m_ui.get());
    // The 3D view must not react to clicks and wheel events the UI consumes.
    m_cameraController->setInputBlocked(
        [this]() { return m_ui != nullptr && m_ui->wantsMouse(); });
    m_lastFrameStart = std::chrono::steady_clock::now();
  } catch (const std::exception& e) {
    cleanUp();
    spdlog::error("Failed to initialize Avalon. Error: {}", e.what());
    throw std::runtime_error("Failed to initialize Avalon.");
  }
}

bool Avalon::isInitialized() const {
  return m_device != nullptr && m_swapchainModel != nullptr &&
         m_renderer != nullptr;
}

bool Avalon::shouldClose() const {
  return m_window == nullptr || m_window->shouldClose();
}

bool Avalon::frame() {
  if (!isInitialized()) {
    spdlog::error("Avalon::frame() called before init().");
    throw std::runtime_error("Avalon::frame() called before init().");
  }
  const auto now = std::chrono::steady_clock::now();
  m_lastFrameSeconds =
      std::chrono::duration<double>(now - m_lastFrameStart).count();
  m_lastFrameStart = now;

  m_window->pollEvents();
  if (m_window->shouldClose()) {
    return false;
  }
  if (m_ui) {
    m_ui->newFrame();
    if (m_uiCallback) {
      m_uiCallback();
    }
  }
  if (!m_renderer->drawFrame()) {
    if (m_ui) {
      m_ui->discardFrame();
    }
    // Minimised or mid-recreate: avoid a busy loop.
    m_window->waitEvents();
  }
  return true;
}

void Avalon::test() { std::cout << "Hello World from Avalon!" << '\n'; }

void Avalon::initVma() {
  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = 0;
  if (m_device->isExtensionEnabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
    allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  }
  allocatorCreateInfo.vulkanApiVersion = kVulkanApiVersion;
  allocatorCreateInfo.physicalDevice = m_device->getPhysicalDevice();
  allocatorCreateInfo.device = m_device->getDevice();
  allocatorCreateInfo.instance = m_instance->getInstance();
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  m_allocator = std::make_shared<VmaAllocatorWrapper>();
  m_allocator->allocator = VK_NULL_HANDLE;
  VkResult res =
      vmaCreateAllocator(&allocatorCreateInfo, &m_allocator->allocator);
  if (res != VK_SUCCESS) {
    spdlog::error("Failed to create VMA allocator. VkResult: {}",
                  static_cast<int>(res));
    throw std::runtime_error("Failed to create VMA allocator.");
  }
}

}  // namespace avalon
