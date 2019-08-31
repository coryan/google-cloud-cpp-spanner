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

#include "google/cloud/spanner/internal/tuple_utils.h"
#include <gmock/gmock.h>
#include <string>
#include <vector>

namespace ns1 {

#if __cplusplus == 201703L
struct NamedStructViaMembers {
  std::int64_t id;
  std::string first_name;
  std::string last_name;

  template <std::size_t N>
  constexpr char const* get_element_name() const {
    if constexpr (N == 0) {
      return "id";
    }
    if constexpr (N == 1) {
      return "first_name";
    }
    if constexpr (N == 2) {
      return "last_name";
    }
    static_assert(N <= 2);
  }

  template <std::size_t N>
  constexpr auto const& get() const& {
    if constexpr (N == 0) {
      return id;
    }
    if constexpr (N == 1) {
      return first_name;
    }
    if constexpr (N == 2) {
      return last_name;
    }
    static_assert(N <= 2);
  }

  template <std::size_t N>
  constexpr auto&& get() && {
    if constexpr (N == 0) {
      return id;
    }
    if constexpr (N == 1) {
      return std::move(first_name);
    }
    if constexpr (N == 2) {
      return std::move(last_name);
    }
    static_assert(N <= 2);
  }

  template <std::size_t N>
  constexpr auto& get() & {
    if constexpr (N == 0) {
      return id;
    }
    if constexpr (N == 1) {
      return first_name;
    }
    if constexpr (N == 2) {
      return last_name;
    }
    static_assert(N <= 2);
  }
};
#endif  // __cplusplus == 201703L

struct NamedStructViaAdl {
  std::int64_t id;
  std::string first_name;
  std::string last_name;
};

template <std::size_t N>
char const* GetElementName(NamedStructViaAdl const&) {
  static char const* names[] = {
      "id",
      "first_name",
      "last_name",
  };
  static_assert(N < sizeof(names) / sizeof(names[0]),
                "get_field_name index out of range");
  return names[N];
}

template <std::size_t N>
auto GetElement(NamedStructViaAdl const& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name));
}

template <std::size_t N>
auto GetElement(NamedStructViaAdl&& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name));
}

template <std::size_t N>
auto GetElement(NamedStructViaAdl& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name));
}
}  // namespace ns1

namespace std {
template <>
struct tuple_size<::ns1::NamedStructViaAdl> {
  static constexpr std::size_t value = 3;
};

#if __cplusplus == 201703L
template <>
struct tuple_size<::ns1::NamedStructViaMembers> {
  static constexpr std::size_t value = 3;
};
#endif  // __cplusplus == 201703L
}  // namespace std

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {
TEST(TupleUtils, NumElements) {
  EXPECT_EQ(internal::NumElements<std::vector<int>>::value, 2);
  EXPECT_EQ(internal::NumElements<std::tuple<>>::value, 0);
  EXPECT_EQ(internal::NumElements<std::tuple<int>>::value, 1);
  EXPECT_EQ((internal::NumElements<std::tuple<int, int>>::value), 2);

  // Verify that NumElement works for tuple-like things that specialize
  // `std::tuple_size<>`, this should become common for C++17 as developers use
  // structured bindings
  // Use a local variable because EXPECT_EQ() takes the address, sigh...
  auto avoid_odr_use = internal::NumElements<ns1::NamedStructViaAdl>::value;
  EXPECT_EQ(avoid_odr_use, 3);
}

// Helper functor used to test the `ForEach` function. Uses a templated
// `operator()`.
struct Stringify {
  template <typename T>
  void operator()(T const& t, std::vector<std::string>& out) const {
    out.push_back(std::to_string(t));
  }
};

// Helper functor used to test the `ForEachNamed` function.
struct StringifyNamed {
  template <typename T>
  void operator()(std::string const& field_name, T const& t,
                  std::vector<std::string>& out) const {
    out.push_back(field_name);
    out.push_back(std::to_string(t));
  }
  void operator()(std::string const& field_name, std::string const& t,
                  std::vector<std::string>& out) const {
    out.push_back(field_name);
    out.push_back(t);
  }
};

TEST(TupleUtils, ForEachMultipleTypes) {
  auto tup = std::make_tuple(true, 42);
  std::vector<std::string> v;
  internal::ForEach(tup, Stringify{}, v);
  EXPECT_THAT(v, testing::ElementsAre("1", "42"));
}

TEST(TupleUtils, ForEachMutate) {
  auto add_one = [](int& x) { x += 1; };
  auto tup = std::make_tuple(1, 2, 3);
  internal::ForEach(tup, add_one);
  EXPECT_EQ(tup, std::make_tuple(2, 3, 4));
}

namespace ns {
// A type that looks like a tuple (i.e., a heterogeneous container), but is not
// a tuple. This will verify that `ForEach` works with tuple-like types. In a
// separate namespace to make sure that `ForEach` works with types in another
// namespace.
template <typename... Ts>
struct NotATuple {
  std::tuple<Ts...> data;
};

// Required ADL extension point to make `NotATuple` iterable like a tuple.
template <std::size_t I, typename... Ts>
auto GetElement(NotATuple<Ts...>& nat) -> decltype(std::get<I>(nat.data)) {
  return std::get<I>(nat.data);
}
}  // namespace ns

TEST(TupleUtils, ForEachStruct) {
  auto not_a_tuple = ns::NotATuple<bool, int>{std::make_tuple(true, 42)};
  std::vector<std::string> v;
  internal::ForEach(not_a_tuple, Stringify{}, v);
  EXPECT_THAT(v, testing::ElementsAre("1", "42"));
}

TEST(TupleUtils, GetElementName_ViaAdl) {
  ::ns1::NamedStructViaAdl tested{1, "fname-1", "lname-1"};
  using internal::GetElementName;
  EXPECT_EQ("id", GetElementName<0>(tested));
  EXPECT_EQ("first_name", GetElementName<1>(tested));
  EXPECT_EQ("last_name", GetElementName<2>(tested));
}

TEST(TupleUtils, GetElementName_ViaMembers) {
  ::ns1::NamedStructViaMembers tested{1, "fname-1", "lname-1"};
  using internal::GetElementName;
  EXPECT_EQ("id", GetElementName<0>(tested));
  EXPECT_EQ("first_name", GetElementName<1>(tested));
  EXPECT_EQ("last_name", GetElementName<2>(tested));
}

TEST(TupleUtils, NumElements_ViaAdl) {
  ::ns1::NamedStructViaAdl tested{1, "fname-1", "lname-1"};
  using internal::GetElementName;
  EXPECT_EQ("id", GetElementName<0>(tested));
  EXPECT_EQ("first_name", GetElementName<1>(tested));
  EXPECT_EQ("last_name", GetElementName<2>(tested));
}

TEST(TupleUtils, NumElements_ViaMembers) {
  ::ns1::NamedStructViaMembers tested{1, "fname-1", "lname-1"};
  using internal::GetElementName;
  EXPECT_EQ("id", GetElementName<0>(tested));
  EXPECT_EQ("first_name", GetElementName<1>(tested));
  EXPECT_EQ("last_name", GetElementName<2>(tested));
}

TEST(TupleUtils, GetElement_ViaAdl) {
  ::ns1::NamedStructViaAdl const tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  EXPECT_EQ(1, GetElement<0>(tested));
  EXPECT_EQ("fname-1", GetElement<1>(tested));
  EXPECT_EQ("lname-1", GetElement<2>(tested));
}

TEST(TupleUtils, GetElement_ViaMembers) {
  ::ns1::NamedStructViaMembers const tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  EXPECT_EQ(1, GetElement<0>(tested));
  EXPECT_EQ("fname-1", GetElement<1>(tested));
  EXPECT_EQ("lname-1", GetElement<2>(tested));
}

TEST(TupleUtils, GetElementMove_ViaAdl) {
  ::ns1::NamedStructViaAdl tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  auto actual = GetElement<1>(std::move(tested));
  EXPECT_EQ("fname-1", actual);
}

TEST(TupleUtils, GetElementMove_ViaMembers) {
  ::ns1::NamedStructViaMembers tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  auto actual = GetElement<1>(std::move(tested));
  EXPECT_EQ("fname-1", actual);
}

TEST(TupleUtils, GetElementAssign_ViaAdl) {
  ::ns1::NamedStructViaAdl tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  GetElement<1>(tested) = "updated";
  EXPECT_EQ("updated", tested.first_name);
}

TEST(TupleUtils, GetElementAssign_ViaMembers) {
  ::ns1::NamedStructViaMembers tested{1, "fname-1", "lname-1"};
  using internal::GetElement;
  GetElement<1>(tested) = "updated";
  EXPECT_EQ("updated", tested.first_name);
}

TEST(TupleUtils, ForEachNamed_ViaAdl) {
  ::ns1::NamedStructViaAdl tested{1, "fname-1", "lname-1"};
  std::vector<std::string> v;
  internal::ForEachNamed(tested, StringifyNamed{}, v);
  EXPECT_THAT(v, testing::ElementsAre("id", "1", "first_name", "fname-1",
                                      "last_name", "lname-1"));
}

TEST(TupleUtils, ForEachNamed_ViaMembers) {
  ::ns1::NamedStructViaMembers tested{1, "fname-1", "lname-1"};
  std::vector<std::string> v;
  internal::ForEachNamed(tested, StringifyNamed{}, v);
  EXPECT_THAT(v, testing::ElementsAre("id", "1", "first_name", "fname-1",
                                      "last_name", "lname-1"));
}

namespace ns2 {
struct LacksNumElements {
  std::tuple<int, int, int> values;
};
template <std::size_t I>
int GetElement(LacksNumElements const& v) {
  return std::get<I>(v.values);
}
template <std::size_t I>
int& GetElement(LacksNumElements& v) {
  return std::get<I>(v.values);
}

struct InvalidGetElementName {
  std::tuple<int, int, int> values;
};
template <std::size_t I>
int GetElement(InvalidGetElementName const& v) {
  return std::get<I>(v.values);
}
template <std::size_t I>
int& GetElement(InvalidGetElementName& v) {
  return std::get<I>(v.values);
}
template <std::size_t I>
int GetElementName(InvalidGetElementName&) {
  return -1;
}

}  // namespace ns2

TEST(TupleUtils, HasNumElements) {
  // These should succeed at compile time, we are verifying that the types work.
  static_assert(!internal::HasNumElements<bool>::value,
                "HasNumElements<bool> should be false");
  static_assert(internal::NumElements<std::tuple<int, int, int>>::value == 3,
                "Mismatched NumElements value");
  static_assert(internal::NumElements<std::pair<int, std::string>>::value == 2,
                "Mismatched NumElements value");
  static_assert(internal::HasNumElements<std::pair<int, std::string>>::value,
                "HasNumElements<std::pair<>> should be true");
  static_assert(internal::HasNumElements<std::tuple<int, std::string>>::value,
                "HasNumElements<std::tuple<>> should be true");

  static_assert(!internal::HasNumElements<ns2::LacksNumElements>::value,
                "HasNumElements<LacksNumElements> should be false");
  static_assert(!internal::HasNumElements<ns2::InvalidGetElementName>::value,
                "HasNumElements<LacksNumElements> should be false");

  static_assert(internal::HasNumElements<ns1::NamedStructViaAdl>::value,
                "ns1::NamedStructViaAdl should be a named struct");
  static_assert(internal::HasNumElements<ns1::NamedStructViaMembers>::value,
                "ns1::NamedStructViaMembers should be a named struct");
}

#if 0
TEST(TupleUtils, IsNamedStruct) {
  // These should succeed at compile time, we are verifying that the types work.
  static_assert(internal::IsNamedStruct<bool>::value == false,
                "bool should not be a NamedStruct");
  static_assert(
      internal::IsNamedStruct<std::pair<int, std::string>>::value == false,
      "std::pair<> should not be a NamedStruct");
  static_assert(internal::IsNamedStruct<
                    std::tuple<int, std::string, std::string>>::value == false,
                "std::tuple<> should not be a NamedStruct");

  static_assert(internal::IsNamedStruct<ns1::NamedStructViaAdl>::value,
                "ns1::NamedStructViaAdl should be a named struct");
  static_assert(internal::IsNamedStruct<ns1::NamedStructViaMembers>::value,
                "ns1::NamedStructViaMembers should be a named struct");
}
#endif

}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google
