// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_READ_PARTITION_H_
#define GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_READ_PARTITION_H_

#include "google/cloud/spanner/connection.h"
#include "google/cloud/spanner/keys.h"
#include "google/cloud/spanner/version.h"
#include "google/cloud/status_or.h"
#include <google/spanner/v1/spanner.pb.h>
#include <string>
#include <vector>

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {

class ReadPartition;

/**
 * Serializes an instance of `ReadPartition` for transmission to another
 * process.
 *
 * @param read_partition - instance to be serialized.
 *
 * @par Example
 *
 * TODO(#388): use snippet in lieu of code/endcode for all examples.
 * @code
 * std::vector<spanner::ReadPartition> partitions =
 *   spanner_client.PartitionRead(...);
 * for (auto const& partition : partitions) {
 *   auto serialized_partition = spanner::SerializeReadPartition(partition);
 *   if (serialized_partition.ok()) {
 *     SendToRemoteMachine(*serialized_partition);
 *   }
 * }
 * @endcode
 */
StatusOr<std::string> SerializeReadPartition(
    ReadPartition const& read_partition);

/**
 * Deserializes the provided string into a `ReadPartition`, if able.
 *
 * Returned `Status` should be checked to determine if deserialization was
 * successful.
 *
 * @param serialized_read_partition - string representation to be deserialized.
 *
 * @par Example
 *
 * @code
 * std::string serialized_partition = ReceiveFromRemoteMachine();
 * StatusOr<spanner::ReadPartition> partition =
 *   spanner::DeserializeReadPartition(serialized_partition);
 * if (partition.ok()) {
 *   auto rows = spanner_client.Read(*partition);
 * }
 * @endcode
 */
StatusOr<ReadPartition> DeserializeReadPartition(
    std::string const& serialized_read_partition);

// Internal implementation details that callers should not use.
namespace internal {

ReadPartition MakeReadPartition(std::string transaction_id,
                                std::string session_id,
                                std::string partition_token,
                                std::string table_name, KeySet key_set,
                                std::vector<std::string> column_names,
                                ReadOptions read_options = {});

// TODO(#409): possibly change to pass by value when issue resolved.
Connection::ReadParams MakeReadParams(ReadPartition const& read_partition);

}  // namespace internal

/**
 * The `ReadPartition` class is a regular type that represents a single
 * slice of a parallel Read operation.
 *
 * Instances of `ReadPartition` are created by `Client::PartitionRead`. Once
 * created, `ReadPartition` objects can be serialized, transmitted to separate
 * process, and used to read data in parallel using `Client::Read`.
 */
class ReadPartition {
 public:
  /**
   * Constructs an instance of `ReadPartition` that does not specify any table
   * or columns to be read.
   */
  ReadPartition() = default;

  /// @name Copy and move.
  ///@{
  ReadPartition(ReadPartition const&) = default;
  ReadPartition(ReadPartition&&) = default;
  ReadPartition& operator=(ReadPartition const&) = default;
  ReadPartition& operator=(ReadPartition&&) = default;
  ///@}

  std::string TableName() const { return proto_.table(); }
  std::vector<std::string> ColumnNames() const {
    auto const& columns = proto_.columns();
    return std::vector<std::string>(columns.begin(), columns.end());
  }
  google::cloud::spanner::ReadOptions ReadOptions() const {
    google::cloud::spanner::ReadOptions options;
    options.index_name = proto_.index();
    options.limit = proto_.limit();
    return options;
  }

  /// @name Equality
  ///@{
  friend bool operator==(ReadPartition const& lhs, ReadPartition const& rhs);
  friend bool operator!=(ReadPartition const& lhs, ReadPartition const& rhs);
  ///@}

 private:
  friend class ReadPartitionTester;
  friend ReadPartition internal::MakeReadPartition(
      std::string transaction_id, std::string session_id,
      std::string partition_token, std::string table_name, KeySet key_set,
      std::vector<std::string> column_names,
      google::cloud::spanner::ReadOptions read_options);
  friend Connection::ReadParams internal::MakeReadParams(
      ReadPartition const& read_partition);
  friend StatusOr<std::string> SerializeReadPartition(
      ReadPartition const& read_partition);
  friend StatusOr<ReadPartition> DeserializeReadPartition(
      std::string const& serialized_read_partition);

  ReadPartition(std::string transaction_id, std::string session_id,
                std::string partition_token, std::string table_name,
                google::cloud::spanner::KeySet key_set,
                std::vector<std::string> column_names,
                google::cloud::spanner::ReadOptions read_options = {});

  // Accessor methods for use by friends.
  std::string PartitionToken() const { return proto_.partition_token(); }
  std::string SessionId() const { return proto_.session(); }
  std::string TransactionId() const { return proto_.transaction().id(); }
  google::spanner::v1::KeySet KeySet() const { return proto_.key_set(); }

  google::spanner::v1::ReadRequest proto_;
};

}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_READ_PARTITION_H_
