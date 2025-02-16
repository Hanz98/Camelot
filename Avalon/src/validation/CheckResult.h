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

#ifndef AVALON_SRC_VALIDATION_CHECKRESULT_H_
#define AVALON_SRC_VALIDATION_CHECKRESULT_H_

#include <pch.h>

#include <exception>
#include <sstream>
#define VK_CHECK_RESULT(f)                                            \
  {                                                                   \
    VkResult res = (f);                                               \
    if (res != VK_SUCCESS) {                                          \
      std::stringstream msg;                                          \
      msg << "Function call " << #f << " returned error code " << res \
          << " in file " << __FILE__ << " at line " << __LINE__;      \
      throw std::runtime_error(msg.str());                            \
    }                                                                 \
  }
#endif  // AVALON_SRC_VALIDATION_CHECKRESULT_H_
