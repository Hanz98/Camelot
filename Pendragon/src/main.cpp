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
#include <Pendragon/src/mcap/McapReader.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <mcap_file>" << std::endl;
    return 1;
  }

  std::cout << "Hello world from Pendragon!" << std::endl;
  MainModel mainTest;
  mainTest.test();

  McapFileReader reader;
  reader.openFile(argv[1]);
  return 0;
}
