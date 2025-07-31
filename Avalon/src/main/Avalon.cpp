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
#include "Avalon.h"

#include <pch.h>

#include <iostream>
#include <memory>

Avalon::Avalon()
    : m_surfaceManager(nullptr),
      m_device(nullptr),
      m_instance(nullptr),
      m_window(nullptr),
      m_allocator(nullptr) {}

Avalon::~Avalon() {
  cleanUp();
  if (m_allocator != nullptr && m_allocator->allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator->allocator);
    m_allocator = nullptr;
  }
}

void Avalon::cleanUp() {
  if (m_device) {
    m_device->cleanUp();
  }
  if (m_surfaceManager) {
    m_surfaceManager->cleanUp();
  }
  if (m_window) {
    m_window->cleanUp();
  }
  if (m_instance != nullptr) {
    m_instance->cleanUp();
  }
  if (m_allocator != nullptr && m_allocator->allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator->allocator);
    m_allocator = nullptr;
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
    m_allocator = std::make_shared<VmaAllocatorWrapper>();
    initVma();
  } catch (const std::exception& e) {
    cleanUp();
    spdlog::error("Failed to initialize Avalon. Error: {}", e.what());
    throw std::runtime_error("Failed to initialize Avalon.");
  }

  cleanUp();
}

void Avalon::test() { std::cout << "Hello World from Avalon!" << std::endl; }

void Avalon::initVma() {
  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorCreateInfo.physicalDevice = m_device->getPhysicalDevice();
  allocatorCreateInfo.device = m_device->getDevice();
  allocatorCreateInfo.instance = m_instance->getInstance();
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vmaCreateAllocator(&allocatorCreateInfo, &m_allocator->allocator);
}
