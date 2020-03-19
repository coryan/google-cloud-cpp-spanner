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

#include "google/cloud/spanner/client.h"
#include "google/cloud/spanner/database.h"
#include "google/cloud/spanner/database_admin_client.h"
#include "google/cloud/spanner/mutations.h"
#include "google/cloud/spanner/testing/database_environment.h"
#include "google/cloud/internal/getenv.h"
#include "google/cloud/internal/random.h"
#include "google/cloud/testing_util/assert_ok.h"
#include "google/cloud/testing_util/init_google_mock.h"
#include <gmock/gmock.h>

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {
namespace {

using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

class ValidateSessionRefresh : public ::testing::Test {
 public:
  ValidateSessionRefresh()
      : emulator_(google::cloud::internal::GetEnv("SPANNER_EMULATOR_HOST")
                      .has_value()) {}

  static void SetUpTestSuite() {
    database_ = spanner_testing::DatabaseEnvironment::GetDatabase();
    client_ =
        google::cloud::internal::make_unique<Client>(MakeConnection(database_));
  }

  void SetUp() override {
    auto commit_result = client_->Commit(
        Mutations{MakeDeleteMutation("Singers", KeySet::All())});
    EXPECT_STATUS_OK(commit_result);
  }

  static void InsertTwoSingers() {
    auto commit_result = client_->Commit(Mutations{
        InsertMutationBuilder("Singers", {"SingerId", "FirstName", "LastName"})
            .EmplaceRow(1, "test-fname-1", "test-lname-1")
            .EmplaceRow(2, "test-fname-2", "test-lname-2")
            .Build()});
    ASSERT_STATUS_OK(commit_result);
  }

  static void TearDownTestSuite() { client_ = nullptr; }

 protected:
  bool EmulatorUnimplemented(Status const& status) {
    return emulator_ && status.code() == StatusCode::kUnimplemented;
  }

  bool emulator_;
  static Database database_;
  static std::unique_ptr<Client> client_;
};

Database ValidateSessionRefresh::database_("unused", "unused", "unused");
std::unique_ptr<Client> ValidateSessionRefresh::client_;

TEST_F(ValidateSessionRefresh, InsertAndCommit) {
  auto stub = internal::CreateDefaultSpannerStub(
      ConnectionOptions(grpc::InsecureChannelCredentials())
          .set_endpoint("localhost:1")
          .enable_tracing("rpc"),
      /*channel_id=*/0);
  EXPECT_NE(stub, nullptr);

  auto create_session = [&]() {
    grpc::ClientContext context;
    google::spanner::v1::CreateSessionRequest request;
    request.set_database(database_.FullName());
    return stub->CreateSession(context, request);
  };
  auto poll_session = [&](std::string session_name) {
    grpc::ClientContext context;
    google::spanner::v1::GetSessionRequest request;
    request.set_name(std::move(session_name));
    return stub->GetSession(context, request);
  };

  auto session1 = create_session();
  ASSERT_STATUS_OK(session1);
  auto session2 = create_session();
  ASSERT_STATUS_OK(session2);

  namespace ch = std::chrono;
  auto const kTestDuration = ch::minutes(120);
  auto const kPoll1 = ch::minutes(1);
  auto const kPoll2 = ch::minutes(60);
  auto const kPollPeriod = ch::seconds(10);
  auto const deadline = ch::steady_clock::now() + kTestDuration;

  auto next_poll1 = ch::steady_clock::now() + kPoll1;
  auto next_poll2 = ch::steady_clock::now() + kPoll2;
  for (auto now = ch::steady_clock::now(); now < deadline;
       now = ch::steady_clock::now()) {
    if (now >= next_poll1) {
      next_poll1 = ch::steady_clock::now() + kPoll1;
      std::cout << "Polling Session 1" << std::endl;
      auto r = poll_session(session1->name());
      EXPECT_STATUS_OK(r);
      if (r) {
        std::cout << "Poll successful = " << r->DebugString() << std::endl;
      }
    }
    if (now >= next_poll2) {
      next_poll1 = ch::steady_clock::now() + kPoll1;
      std::cerr << "Polling Session 2" << std::endl;
      auto r = poll_session(session2->name());
      EXPECT_STATUS_OK(r);
      if (r) {
        std::cout << "Poll successful = " << r->DebugString() << std::endl;
      }
    }
    std::this_thread::sleep_for(kPollPeriod);
  }
}

}  // namespace
}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google

int main(int argc, char* argv[]) {
  ::google::cloud::testing_util::InitGoogleMock(argc, argv);
  (void)::testing::AddGlobalTestEnvironment(
      new google::cloud::spanner_testing::DatabaseEnvironment());

  return RUN_ALL_TESTS();
}
