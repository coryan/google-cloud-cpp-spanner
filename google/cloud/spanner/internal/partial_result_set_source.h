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

#ifndef GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_PARTIAL_RESULT_SET_SOURCE_H_
#define GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_PARTIAL_RESULT_SET_SOURCE_H_

#include "google/cloud/spanner/internal/partial_result_set_reader.h"
#include "google/cloud/spanner/result_set.h"
#include "google/cloud/spanner/value.h"
#include "google/cloud/optional.h"
#include "google/cloud/status.h"
#include "google/cloud/status_or.h"
#include <google/spanner/v1/spanner.grpc.pb.h>
#include <google/spanner/v1/spanner.pb.h>
#include <grpcpp/grpcpp.h>
#include <memory>

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {
namespace internal {

/**
 * This class serves as a bridge between the gRPC `PartialResultSet` streaming
 * reader and the spanner `ResultSet`, which is used to iterate over the rows
 * returned from a read operation.
 */
class PartialResultSetSource : public internal::ResultSetSource {
 public:
  /// Factory method to create a PartialResultSetSource.
  static StatusOr<std::unique_ptr<PartialResultSetSource>> Create(
      std::unique_ptr<PartialResultSetReader> reader);
  ~PartialResultSetSource() override;

  StatusOr<optional<Value>> NextValue() override;
  optional<google::spanner::v1::ResultSetMetadata> Metadata() override {
    if (last_result_.has_metadata()) {
      return last_result_.metadata();
    }
    return {};
  }

  std::int64_t RowsModified() const override {
    if (last_result_.stats().row_count_case() ==
        google::spanner::v1::ResultSetStats::kRowCountLowerBound) {
      return last_result_.stats().row_count_lower_bound();
    } else {
      return last_result_.stats().row_count_exact();
    }
  }

  optional<std::unordered_map<std::string, std::string>> QueryStats()
      const override {
    if (last_result_.has_stats() && last_result_.stats().has_query_stats()) {
      std::unordered_map<std::string, std::string> query_stats;
      return query_stats;
    }
    return {};
  }

  optional<QueryPlan> QueryExecutionPlan() const override {
    if (last_result_.has_stats() && last_result_.stats().has_query_plan()) {
      return last_result_.stats().query_plan();
    }
    return {};
  }

  optional<google::spanner::v1::ResultSetStats> Stats() override {
    if (last_result_.has_stats()) {
      return last_result_.stats();
    }
    return {};
  }

 private:
  explicit PartialResultSetSource(
      std::unique_ptr<PartialResultSetReader> reader)
      : reader_(std::move(reader)) {}

  Status ReadFromStream();

  std::unique_ptr<PartialResultSetReader> reader_;
  google::spanner::v1::PartialResultSet last_result_;
  optional<google::protobuf::Value> partial_chunked_value_;
  bool finished_ = false;
  int next_value_index_ = 0;
  int next_value_type_index_ = 0;
};

}  // namespace internal
}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_PARTIAL_RESULT_SET_SOURCE_H_
