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

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>  // NOLINT(build/c++17)
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include "Camelot/src/main/MainModel.h"
#include "Camelot/src/replay/Recording.h"

namespace {

constexpr int kExitOk = 0;
constexpr int kExitUsage = 1;
constexpr int kExitBadRecording = 2;

void basicLogfileSetup() {
  try {
    auto logger = spdlog::basic_logger_mt("Logger", "logs/basic-log.txt");
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "Log init failed: " << ex.what() << '\n';
  }
}

void printUsage(std::string_view program) {
  std::cout << "Usage: " << program << " [--help] [recording.mcap]\n"
            << "\n"
            << "Replays a Foxglove MCAP recording (camera images, point\n"
            << "clouds, scene entities, transforms) with a timeline. Without\n"
            << "a recording the demo scene is shown.\n";
}

// Checks the recording before any window exists so a bad path is a one-line
// error on stderr. Returns the exit code, kExitOk when the file replays.
int probeRecording(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) {
    std::cerr << "Pendragon: " << path.string()
              << ": no such file or not a regular file\n";
    return kExitBadRecording;
  }
  camelot::Recording recording;
  try {
    recording.open(path);
  } catch (const std::exception& error) {
    std::cerr << "Pendragon: " << path.string() << ": " << error.what() << '\n';
    return kExitBadRecording;
  }
  return kExitOk;
}

}  // namespace

int main(int argc, char* argv[]) {
  const std::span<char*> args(argv, static_cast<size_t>(argc));
  const std::string_view program = args.empty() ? "Pendragon" : args[0];
  std::filesystem::path recording;
  for (const char* arg : args.subspan(args.empty() ? 0 : 1)) {
    const std::string_view value(arg);
    if (value == "--help" || value == "-h") {
      printUsage(program);
      return kExitOk;
    }
    if (value.starts_with("-") || !recording.empty()) {
      std::cerr << "Pendragon: unexpected argument '" << value << "'\n";
      printUsage(program);
      return kExitUsage;
    }
    recording = value;
  }
  if (!recording.empty()) {
    const int status = probeRecording(recording);
    if (status != kExitOk) {
      return status;
    }
  }

  std::cout << "Hello world from Pendragon!" << '\n';
  basicLogfileSetup();
  camelot::MainModel model;
  model.run(recording);
  return kExitOk;
}
