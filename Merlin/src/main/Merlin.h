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

#ifndef MERLIN_SRC_MAIN_MERLIN_H_
#define MERLIN_SRC_MAIN_MERLIN_H_

#include <iostream>
#include "IModel.h"

namespace Merlin {

class Model : public IModel {
 public:
  void test() override;
};

}  // namespace Merlin

// Legacy alias for backward compatibility - place outside namespace
class Merlin : public Merlin::Model {
public:
  // Inherit all constructors and methods
  using Merlin::Model::Model;
};

#endif  // MERLIN_SRC_MAIN_MERLIN_H_
