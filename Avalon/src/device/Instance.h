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

#include <VkBootstrap.h>
#include <VkBootstrapDispatch.h>
#include <pch.h>
#include "InstanceIModel.h"

namespace Core {

class Model : public IModel {
 private:
  vkb::Instance m_instance;

 public:
  Model();
  Model(Model&& other);
  Model(const Model& other) = delete;
  Model& operator=(Model&& other);
  Model& operator=(const Model& other) = delete;

  ~Model() override;

  void cleanUp() override;

  void init() override;
  inline const vkb::Instance& getVkbInstance() const override { return m_instance; }
  inline VkInstance& getInstance() override { return m_instance.instance; }
};

}  // namespace Core

// Legacy alias for backward compatibility - place outside namespace
class Instance : public Core::Model {
public:
  // Inherit all constructors and methods
  using Core::Model::Model;
};

#endif  // AVALON_SRC_DEVICE_INSTANCE_H_
