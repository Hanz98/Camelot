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
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>

void basicLogfileSetup() {
  try {
    auto logger = spdlog::basic_logger_mt("Logger", "logs/basic-log.txt");
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  } catch (const spdlog::spdlog_ex &ex) {
    std::cout << "Log init failed: " << ex.what() << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "Hello world from Pendragon!" << std::endl;
  basicLogfileSetup();
  MainModel mainTest;
  mainTest.test();
  return 0;
}
