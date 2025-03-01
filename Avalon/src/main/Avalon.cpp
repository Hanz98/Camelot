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
#include <volk.h>

#include <iostream>

Avalon::Avalon() : m_window(), m_device(), m_instance() {}

Avalon::~Avalon() { cleanUp(); }

void Avalon::cleanUp() {
  m_window.cleanUp();
  m_instance.cleanUp();
  m_device.cleanUp();
}

void Avalon::init() {
  if (volkInitialize() != VK_SUCCESS) {
    spdlog::error("Failed to initialize volk.");
    throw std::runtime_error("Failed to initialize volk.");
  }
  try {
    m_window.init(
        400, 400,
        "Avalon");  // TO DO: Make window size and title read from settings
    m_instance.init();
    //    m_device.PickPhysicalDevice(m_instance, m_window.getSurface());
  } catch (const std::exception& e) {
    spdlog::error("Failed to initialize Avalon. Error: {}", e.what());
    throw std::runtime_error("Failed to initialize Avalon.");
  }
  //  m_device.PickPhysicalDevice(m_instance, m_window.getSurface());
}

void Avalon::test() { std::cout << "Hello World from Avalon!" << std::endl; }
