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
#include "McapReader.h"

#include <spdlog/spdlog.h>

#include <exception>
#include <memory>
#include <string>

#define MCAP_IMPLEMENTATION

#include <mcap/reader.hpp>

McapFileReader::McapFileReader()
    : m_reader(nullptr),
      m_protoPool(&m_protoDb),
      m_protoFactory(&m_protoPool) {}

McapFileReader::~McapFileReader() {
  if (m_reader != nullptr) {
    delete m_reader;
    m_reader = nullptr;
  }
}
void McapFileReader::openFile(const std::string& fileName) {
  const auto res = m_reader->open(fileName);
  if (res.ok() == false) {
    spdlog::error("Failed to open file: {}", fileName);
    throw std::runtime_error("Failed to open file");
  }
  m_messages =
      std::make_shared<mcap::LinearMessageView>(m_reader->readMessages());
}

bool McapFileReader::readNext(uint64_t timestamp) {
  if (m_messages == nullptr) {
    return false;
  }

  for (auto message = m_messages->begin(); message != m_messages->end();
       ++message) {
    if (message->schema->encoding != "protobuf") {
      continue;
    }
    if (message->channel->messageEncoding != "protobuf") {
      spdlog::warn("Expected channel with ID {} to have protobuf encoding",
                   message->channel->id);
      m_reader->close();
      throw std::runtime_error("Expected protobuf encoding");
    }

    const google::protobuf::Descriptor* descriptor =
        m_protoPool.FindMessageTypeByName(message->schema->name);
    if (descriptor == nullptr) {
      /*
                  if (!LoadSchema(it->schema, &protoDb)) {
                      reader.close();
                      return 1;
                  }
      */

      descriptor = m_protoPool.FindMessageTypeByName(message->schema->name);
      if (descriptor == nullptr) {
        spdlog::error("Failed to load schema: {}", message->schema->name);
        throw std::runtime_error("Failed to load schema");
      }
    }

    auto protoMessage = std::unique_ptr<google::protobuf::Message>(
        m_protoFactory.GetPrototype(descriptor)->New());
    if (!protoMessage->ParseFromArray(
            message->message.data,
            static_cast<int>(message->message.dataSize))) {
      spdlog::error("Failed to parse message");
      throw std::runtime_error("Failed to parse message");
    }

    if (message->timestamp >= timestamp) {
      return true;
    }
  }

  return false;
}
