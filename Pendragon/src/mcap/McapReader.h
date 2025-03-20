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
#ifndef PENDRAGON_SRC_MCAP_MCAPREADER_H_
#define PENDRAGON_SRC_MCAP_MCAPREADER_H_

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mcap {
class McapReader;
class LinearMessageView;
};  // namespace mcap

class McapFileReader {
 private:
  mcap::McapReader* m_reader;
  std::shared_ptr<mcap::LinearMessageView> m_messages;

  google::protobuf::SimpleDescriptorDatabase m_protoDb;
  google::protobuf::DescriptorPool m_protoPool;
  google::protobuf::DynamicMessageFactory m_protoFactory;

 public:
  McapFileReader();
  ~McapFileReader();

  void openFile(const std::string& fileName);
  bool readNext(uint64_t timestamp);
};
#endif  // PENDRAGON_SRC_MCAP_MCAPREADER_H_
