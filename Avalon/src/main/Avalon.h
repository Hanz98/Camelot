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

#ifndef AVALON_SRC_MAIN_AVALON_H_
#define AVALON_SRC_MAIN_AVALON_H_

#include <Avalon/src/window/Window.h>
#include <pch.h>

#include <iostream>

class Avalon {
  Window m_Window;

 public:
  void test();
};

#endif  // AVALON_SRC_MAIN_AVALON_H_
