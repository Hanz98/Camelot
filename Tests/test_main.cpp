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

#include <gtest/gtest.h>

#include <cstdlib>
#include <iostream>

namespace {

// When built with CAMELOT_TESTS_USE_MOCK_ICD, point the Vulkan loader at the
// bundled mock driver before any instance is created. Variables already set
// in the environment win, so a developer can still run the same binary on a
// real GPU with VK_DRIVER_FILES.
void configureMockIcd() {
#ifdef CAMELOT_MOCK_ICD_JSON
  const char* json = CAMELOT_MOCK_ICD_JSON;
  // VK_DRIVER_FILES is the current loader variable, VK_ICD_FILENAMES the
  // legacy one; both point at the mock driver so only it is loaded.
  setenv("VK_DRIVER_FILES", json, 0);
  setenv("VK_ICD_FILENAMES", json, 0);
  // Read by tests that must skip checks the null driver cannot satisfy.
  setenv("CAMELOT_TESTS_USE_MOCK_ICD", "1", 0);
  std::cout << "Using mock Vulkan ICD: " << std::getenv("VK_DRIVER_FILES")
            << '\n';
#endif
}

}  // namespace

int main(int argc, char** argv) {
  configureMockIcd();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
