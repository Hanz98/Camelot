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

Instance::Instance() : m_instance(VK_NULL_HANDLE) {}

Instance::Instance(Instance&& other) : m_instance(other.m_instance) {
  other.m_instance = VK_NULL_HANDLE;
}

Instance& Instance::operator=(Instance&& other) {
  m_instance = other.m_instance;
  other.m_instance = VK_NULL_HANDLE;
  return *this;
}

Instance::~Instance() { cleanUp(); }

void Instance::cleanUp() {
  if (m_instance) {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }
}

void Instance::init() {}
