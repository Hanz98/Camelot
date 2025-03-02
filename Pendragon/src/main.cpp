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

#include <Camelot/src/main/MainModel.h>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Hello world from Pendragon!" << std::endl;
  MainModel mainTest;
  // mainTest.test();
#ifndef _WIN32
  usleep(10000000);
#endif
  return 0;
}
