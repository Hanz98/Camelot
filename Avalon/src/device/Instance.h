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

#ifndef AVALON_SRC_DEVICE_INSTANCE_H_
#define AVALON_SRC_DEVICE_INSTANCE_H_

#include <pch.h>

#include "VkBootstrap.h"
#include "VkBootstrapDispatch.h"

class Instance {
 private:
  vkb::Instance m_instance;

 public:
  Instance();
  Instance(Instance&& other);
  Instance(const Instance& other) = delete;
  Instance& operator=(Instance&& other);
  Instance& operator=(const Instance& other) = delete;

  ~Instance();

  void cleanUp();

  void init();
  inline VkInstance& getInstance() { return m_instance.instance; }
};

#endif  // AVALON_SRC_DEVICE_INSTANCE_H_
