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

#include "Instance.h"

#include <Avalon/src/validation/CheckResult.h>
#include <volk.h>

#include <Avalon/src/utils/Initializers.hpp>

Instance::Instance() : m_instance() {}

Instance::Instance(Instance&& other) : m_instance(other.m_instance) {
  other.m_instance = {};
}

Instance& Instance::operator=(Instance&& other) {
  m_instance = other.m_instance;
  other.m_instance = {};
  return *this;
}

Instance::~Instance() { cleanUp(); }

void Instance::cleanUp() {
  if (m_instance) {
    vkb::destroy_instance(m_instance);
    m_instance = {};
  }
}

void Instance::init() {
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Camelot")
                      .request_validation_layers()
                      .use_default_debug_messenger()
                      .build();
  if (!inst_ret) {
    spdlog::error("Failed to create Vulkan instance. Error: " +
                  inst_ret.error().message());
    throw std::runtime_error("Failed to create Vulkan instance.");
  }
  vkb::Instance vkb_inst = inst_ret.value();
  volkLoadInstance(vkb_inst.instance);
}
