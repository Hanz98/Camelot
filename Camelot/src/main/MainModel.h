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

#ifndef CAMELOT_SRC_MAIN_MAINMODEL_H_
#define CAMELOT_SRC_MAIN_MAINMODEL_H_

#include <Avalon/src/window/Window.h>
#include <Camelot/API/main/ICamelot.h>

#include <iostream>

class MainModel : public ICamelot {
 private:
  Window m_window;

 public:
  void test();
};

#endif  // CAMELOT_SRC_MAIN_MAINMODEL_H_
