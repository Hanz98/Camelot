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
void Avalon::test() {
  std::cout << "Hello World from Avalon!" << std::endl;
  if (volkInitialize() != VK_SUCCESS) {
    spdlog::error("Failed to initialize volk.");
    throw std::runtime_error("Failed to initialize volk.");
  }
}
