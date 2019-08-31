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

#include "google/cloud/spanner/value.h"
#include "google/cloud/spanner/internal/base64.h"
#include "google/cloud/spanner/internal/date.h"
#include "google/cloud/spanner/internal/time.h"
#include "google/cloud/spanner/testing/matchers.h"
#include "google/cloud/optional.h"
#include "google/cloud/testing_util/assert_ok.h"
#include <google/protobuf/text_format.h>
#include <gmock/gmock.h>
#include <cmath>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ns {
#if __cplusplus == 201703L
struct NamedStructCxx17 {
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

// Used by google test
bool operator==(NamedStructCxx17 const& lhs, NamedStructCxx17 const& rhs) {
  return std::tie(lhs.id, lhs.first_name, lhs.last_name) ==
         std::tie(rhs.id, rhs.first_name, rhs.last_name);
}
bool operator!=(NamedStructCxx17 const& lhs, NamedStructCxx17 const& rhs) {
  return !(lhs == rhs);
}

// Used by google test
void PrintTo(NamedStructCxx17 const& x, std::ostream* os) {
  *os << "{" << x.id << "," << x.first_name << "," << x.last_name << "}";
}
}  // namespace ns

namespace std {
template <>
class tuple_size<::ns::NamedStructCxx17>
    : public std::integral_constant<std::size_t, 3> {};
}  // namespace std
#endif  // __cplusplus == 201703L

namespace ns {
struct NamedStructCxx11 {
  std::int64_t id;
  std::string first_name;
  std::string last_name;
  google::cloud::spanner::Date birth_date;
};

template <std::size_t N>
char const* GetElementName(NamedStructCxx11 const&) {
  static char const* names[] = {
      "id",
      "FirstName",
      "LastName",
      "BirthDate",
  };
  static_assert(N <= sizeof(names) / sizeof(names[0]),
                "Index out of range in GetFieldName(NamedStructCxx11)");
  return names[N];
}

template <std::size_t N>
auto GetElement(NamedStructCxx11 const& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name,
                                     v.birth_date))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name, v.birth_date));
}

template <std::size_t N>
auto GetElement(NamedStructCxx11&& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name,
                                     v.birth_date))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name, v.birth_date));
}

template <std::size_t N>
auto GetElement(NamedStructCxx11& v)
    -> decltype(std::get<N>(std::tie(v.id, v.first_name, v.last_name,
                                     v.birth_date))) {
  return std::get<N>(std::tie(v.id, v.first_name, v.last_name, v.birth_date));
}

// Used by google test
bool operator==(NamedStructCxx11 const& lhs, NamedStructCxx11 const& rhs) {
  return std::tie(lhs.id, lhs.first_name, lhs.last_name, lhs.birth_date) ==
         std::tie(rhs.id, rhs.first_name, rhs.last_name, rhs.birth_date);
}
bool operator!=(NamedStructCxx11 const& lhs, NamedStructCxx11 const& rhs) {
  return !(lhs == rhs);
}

// Used by google test
void PrintTo(NamedStructCxx11 const& x, std::ostream* os) {
  *os << "{" << x.id << "," << x.first_name << "," << x.last_name << ","
      << x.birth_date.year() << "-" << x.birth_date.month() << "-"
      << x.birth_date.day() << "}";
}

}  // namespace ns

namespace std {
template <>
class tuple_size<::ns::NamedStructCxx11>
    : public std::integral_constant<std::size_t, 4> {};
}  // namespace std

namespace google {
namespace cloud {
// namespace spanner_extensions {
// template <>
// struct NumElements<::ns::NamedStructCxx11>
//    : public std::integral_constant<std::size_t, 4> {};
//}  // namespace spanner_extensions

namespace spanner {
inline namespace SPANNER_CLIENT_NS {
using ::google::cloud::spanner_testing::IsProtoEqual;
using ::google::protobuf::TextFormat;

template <typename T>
void TestBasicSemantics(T init) {
  Value const default_ctor{};
  EXPECT_FALSE(default_ctor.get<T>().ok());

  Value const v{init};

  EXPECT_STATUS_OK(v.get<T>());
  EXPECT_EQ(init, *v.get<T>());

  Value copy = v;
  EXPECT_EQ(copy, v);
  Value const moved = std::move(copy);
  EXPECT_EQ(moved, v);

  // Tests a null Value of type `T`.
  Value const null = MakeNullValue<T>();

  EXPECT_FALSE(null.get<T>().ok());
  EXPECT_STATUS_OK(null.get<optional<T>>());
  EXPECT_EQ(optional<T>{}, *null.get<optional<T>>());

  Value copy_null = null;
  EXPECT_EQ(copy_null, null);
  Value const moved_null = std::move(copy_null);
  EXPECT_EQ(moved_null, null);

  // Round-trip from Value -> Proto(s) -> Value
  auto const protos = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(protos.first, protos.second));

  Value const not_null{optional<T>(init)};
  EXPECT_STATUS_OK(not_null.get<T>());
  EXPECT_EQ(init, *not_null.get<T>());
  EXPECT_STATUS_OK(not_null.get<optional<T>>());
  EXPECT_EQ(init, **not_null.get<optional<T>>());
}

TEST(Value, BasicSemantics) {
  for (auto x : {false, true}) {
    SCOPED_TRACE("Testing: bool " + std::to_string(x));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<bool>(5, x));
    std::vector<optional<bool>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }

  auto const min64 = std::numeric_limits<std::int64_t>::min();
  auto const max64 = std::numeric_limits<std::int64_t>::max();
  for (auto x : std::vector<std::int64_t>{min64, -1, 0, 1, max64}) {
    SCOPED_TRACE("Testing: std::int64_t " + std::to_string(x));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<std::int64_t>(5, x));
    std::vector<optional<std::int64_t>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }

  // Note: We skip testing the NaN case here because NaN always compares not
  // equal, even with itself. So NaN is handled in a separate test.
  auto const inf = std::numeric_limits<double>::infinity();
  for (auto x : {-inf, -1.0, -0.5, 0.0, 0.5, 1.0, inf}) {
    SCOPED_TRACE("Testing: double " + std::to_string(x));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<double>(5, x));
    std::vector<optional<double>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }

  for (auto const& x :
       std::vector<std::string>{"", "f", "foo", "12345678901234567"}) {
    SCOPED_TRACE("Testing: std::string " + std::string(x));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<std::string>(5, x));
    std::vector<optional<std::string>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }

  for (auto const& x : std::vector<Value::Bytes>{
           Value::Bytes{""}, Value::Bytes{"f"}, Value::Bytes{"foo"},
           Value::Bytes{"12345678901234567"}}) {
    SCOPED_TRACE("Testing: Value::Bytes " + std::string(x.data));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<Value::Bytes>(5, x));
    std::vector<optional<Value::Bytes>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }

  for (time_t t : {
           -9223372035LL,   // near the limit of 64-bit/ns system_clock
           -2147483649LL,   // below min 32-bit int
           -2147483648LL,   // min 32-bit int
           -1LL, 0LL, 1LL,  // around the unix epoch
           1561147549LL,    // contemporary
           2147483647LL,    // max 32-bit int
           2147483648LL,    // above max 32-bit int
           9223372036LL     // near the limit of 64-bit/ns system_clock
       }) {
    auto tp = std::chrono::system_clock::from_time_t(t);
    for (auto nanos : {-1, 0, 1}) {
      auto ts = std::chrono::time_point_cast<Timestamp::duration>(tp) +
                std::chrono::nanoseconds(nanos);
      SCOPED_TRACE("Testing: google::cloud::spanner::Timestamp " +
                   internal::TimestampToString(ts));
      TestBasicSemantics(ts);
      std::vector<Timestamp> v(5, ts);
      TestBasicSemantics(v);
      std::vector<optional<Timestamp>> ov(5, ts);
      ov.resize(10);
      TestBasicSemantics(ov);
    }
  }

  for (auto x : {
           Date(1582, 10, 15),  // start of Gregorian calendar
           Date(1677, 9, 21),   // before system_clock limit
           Date(1901, 12, 13),  // around min 32-bit seconds limit
           Date(1970, 1, 1),    // the unix epoch
           Date(2019, 6, 21),   // contemporary
           Date(2038, 1, 19),   // around max 32-bit seconds limit
           Date(2262, 4, 12)    // after system_clock limit
       }) {
    SCOPED_TRACE("Testing: google::cloud::spanner::Date " +
                 internal::DateToString(x));
    TestBasicSemantics(x);
    TestBasicSemantics(std::vector<Date>(5, x));
    std::vector<optional<Date>> v(5, x);
    v.resize(10);
    TestBasicSemantics(v);
  }
}

TEST(Value, DoubleNaN) {
  double const nan = std::nan("NaN");
  Value v{nan};
  EXPECT_TRUE(std::isnan(*v.get<double>()));

  // Since IEEE 754 defines that nan is not equal to itself, then a Value with
  // NaN should not be equal to itself.
  EXPECT_NE(nan, nan);
  EXPECT_NE(v, v);
}

TEST(Value, BytesDecodingError) {
  Value const v(Value::Bytes("some data"));
  auto p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));

  // Now we corrupt the Value proto so that it cannot be decoded.
  p.second.set_string_value("not base64 encoded data");
  Value bad = internal::FromProto(p.first, p.second);
  EXPECT_NE(v, bad);

  // We know the type is Bytes, but we cannot get a value out of it because the
  // base64 decoding will fail.
  StatusOr<Value::Bytes> bytes = bad.get<Value::Bytes>();
  EXPECT_FALSE(bytes.ok());
  EXPECT_THAT(bytes.status().message(), testing::HasSubstr("Invalid base64"));
}

TEST(Value, BytesRelationalOperators) {
  // Note that Value::Bytes inequalities treat the bytes as unsigned, so
  // b1 is always less than b2, without respect to the signedness of char.
  Value::Bytes b1(std::string(1, '\x00'));
  Value::Bytes b2(std::string(1, '\xff'));

  EXPECT_EQ(b1, b1);
  EXPECT_LE(b1, b1);
  EXPECT_GE(b1, b1);

  EXPECT_NE(b1, b2);
  EXPECT_LT(b1, b2);
  EXPECT_LE(b1, b2);
  EXPECT_GE(b2, b1);
  EXPECT_GT(b2, b1);
}

TEST(Value, ConstructionFromLiterals) {
  Value v_int64(42);
  EXPECT_EQ(42, *v_int64.get<std::int64_t>());

  Value v_string("hello");
  EXPECT_EQ("hello", *v_string.get<std::string>());

  std::vector<char const*> vec = {"foo", "bar"};
  Value v_vec(vec);
  EXPECT_STATUS_OK(v_vec.get<std::vector<std::string>>());

  std::tuple<char const*, char const*> tup = std::make_tuple("foo", "bar");
  Value v_tup(tup);
  EXPECT_STATUS_OK((v_tup.get<std::tuple<std::string, std::string>>()));

  auto named_field = std::make_tuple(false, std::make_pair("f1", 42));
  Value v_named_field(named_field);
  EXPECT_STATUS_OK(
      (v_named_field
           .get<std::tuple<bool, std::pair<std::string, std::int64_t>>>()));
}

TEST(Value, MixingTypes) {
  using A = bool;
  using B = std::int64_t;

  Value a(A{});
  EXPECT_TRUE(a.get<A>().ok());
  EXPECT_FALSE(a.get<B>().ok());
  EXPECT_FALSE(a.get<B>().ok());

  Value null_a = MakeNullValue<A>();
  EXPECT_FALSE(null_a.get<A>().ok());
  EXPECT_FALSE(null_a.get<B>().ok());

  EXPECT_NE(null_a, a);

  Value b(B{});
  EXPECT_TRUE(b.get<B>().ok());
  EXPECT_FALSE(b.get<A>().ok());
  EXPECT_FALSE(b.get<A>().ok());

  EXPECT_NE(b, a);
  EXPECT_NE(b, null_a);

  Value null_b = MakeNullValue<B>();
  EXPECT_FALSE(null_b.get<B>().ok());
  EXPECT_FALSE(null_b.get<A>().ok());

  EXPECT_NE(null_b, b);
  EXPECT_NE(null_b, null_a);
  EXPECT_NE(null_b, a);
}

TEST(Value, SpannerArray) {
  using ArrayInt64 = std::vector<std::int64_t>;
  using ArrayDouble = std::vector<double>;

  ArrayInt64 const empty = {};
  Value const ve(empty);
  EXPECT_EQ(ve, ve);
  EXPECT_TRUE(ve.get<ArrayInt64>().ok());
  EXPECT_FALSE(ve.get<ArrayDouble>().ok());
  EXPECT_EQ(empty, *ve.get<ArrayInt64>());

  ArrayInt64 const ai = {1, 2, 3};
  Value const vi(ai);
  EXPECT_EQ(vi, vi);
  EXPECT_TRUE(vi.get<ArrayInt64>().ok());
  EXPECT_FALSE(vi.get<ArrayDouble>().ok());
  EXPECT_EQ(ai, *vi.get<ArrayInt64>());

  ArrayDouble const ad = {1.0, 2.0, 3.0};
  Value const vd(ad);
  EXPECT_EQ(vd, vd);
  EXPECT_NE(vi, vd);
  EXPECT_FALSE(vd.get<ArrayInt64>().ok());
  EXPECT_TRUE(vd.get<ArrayDouble>().ok());
  EXPECT_EQ(ad, *vd.get<ArrayDouble>());

  Value const null_vi = MakeNullValue<ArrayInt64>();
  EXPECT_EQ(null_vi, null_vi);
  EXPECT_NE(null_vi, vi);
  EXPECT_NE(null_vi, vd);
  EXPECT_FALSE(null_vi.get<ArrayInt64>().ok());
  EXPECT_FALSE(null_vi.get<ArrayDouble>().ok());

  Value const null_vd = MakeNullValue<ArrayDouble>();
  EXPECT_EQ(null_vd, null_vd);
  EXPECT_NE(null_vd, null_vi);
  EXPECT_NE(null_vd, vd);
  EXPECT_NE(null_vd, vi);
  EXPECT_FALSE(null_vd.get<ArrayDouble>().ok());
  EXPECT_FALSE(null_vd.get<ArrayInt64>().ok());
}

TEST(Value, SpannerStruct) {
  // Using declarations to shorten the tests, making them more readable.
  using std::int64_t;
  using std::make_pair;
  using std::make_tuple;
  using std::pair;
  using std::string;
  using std::tuple;

  auto tup1 = make_tuple(false, int64_t{123});
  using T1 = decltype(tup1);
  Value v1(tup1);
  EXPECT_STATUS_OK(v1.get<T1>());
  EXPECT_EQ(tup1, *v1.get<T1>());
  EXPECT_EQ(v1, v1);

  // Verify we can extract tuple elements even if they're wrapped in a pair.
  auto const pair0 = v1.get<tuple<pair<string, bool>, int64_t>>();
  EXPECT_STATUS_OK(pair0);
  EXPECT_EQ(std::get<0>(tup1), std::get<0>(*pair0).second);
  EXPECT_EQ(std::get<1>(tup1), std::get<1>(*pair0));
  auto const pair1 = v1.get<tuple<bool, pair<string, int64_t>>>();
  EXPECT_STATUS_OK(pair1);
  EXPECT_EQ(std::get<0>(tup1), std::get<0>(*pair1));
  EXPECT_EQ(std::get<1>(tup1), std::get<1>(*pair1).second);
  auto const pair01 =
      v1.get<tuple<pair<string, bool>, pair<string, int64_t>>>();
  EXPECT_STATUS_OK(pair01);
  EXPECT_EQ(std::get<0>(tup1), std::get<0>(*pair01).second);
  EXPECT_EQ(std::get<1>(tup1), std::get<1>(*pair01).second);

  auto tup2 = make_tuple(false, make_pair(string("f2"), int64_t{123}));
  using T2 = decltype(tup2);
  Value v2(tup2);
  EXPECT_STATUS_OK(v2.get<T2>());
  EXPECT_EQ(tup2, *v2.get<T2>());
  EXPECT_EQ(v2, v2);
  EXPECT_NE(v2, v1);

  // T1 is lacking field names, but otherwise the same as T2.
  EXPECT_EQ(tup1, *v2.get<T1>());
  EXPECT_NE(tup2, *v1.get<T2>());

  auto tup3 = make_tuple(false, make_pair(string("Other"), int64_t{123}));
  using T3 = decltype(tup3);
  Value v3(tup3);
  EXPECT_STATUS_OK(v3.get<T3>());
  EXPECT_EQ(tup3, *v3.get<T3>());
  EXPECT_EQ(v3, v3);
  EXPECT_NE(v3, v2);
  EXPECT_NE(v3, v1);

  static_assert(std::is_same<T2, T3>::value, "Only diff is field name");

  // v1 != v2, yet T2 works with v1 and vice versa
  EXPECT_NE(v1, v2);
  EXPECT_STATUS_OK(v1.get<T2>());
  EXPECT_STATUS_OK(v2.get<T1>());

  Value v_null(optional<T1>{});
  EXPECT_FALSE(v_null.get<optional<T1>>()->has_value());
  EXPECT_FALSE(v_null.get<optional<T2>>()->has_value());

  EXPECT_NE(v1, v_null);
  EXPECT_NE(v2, v_null);

  auto array_struct = std::vector<T3>{
      T3{false, {"age", 1}},
      T3{true, {"age", 2}},
      T3{false, {"age", 3}},
  };
  using T4 = decltype(array_struct);
  Value v4(array_struct);
  EXPECT_TRUE(v4.get<T4>().ok());
  EXPECT_FALSE(v4.get<T3>().ok());
  EXPECT_FALSE(v4.get<T2>().ok());
  EXPECT_FALSE(v4.get<T1>().ok());

  EXPECT_STATUS_OK(v4.get<T4>());
  EXPECT_EQ(array_struct, *v4.get<T4>());

  auto empty = tuple<>{};
  using T5 = decltype(empty);
  Value v5(empty);
  EXPECT_TRUE(v5.get<T5>().ok());
  EXPECT_FALSE(v5.get<T4>().ok());
  EXPECT_EQ(v5, v5);
  EXPECT_NE(v5, v4);

  EXPECT_STATUS_OK(v5.get<T5>());
  EXPECT_EQ(empty, *v5.get<T5>());

  auto crazy = tuple<tuple<std::vector<optional<bool>>>>{};
  using T6 = decltype(crazy);
  Value v6(crazy);
  EXPECT_TRUE(v6.get<T6>().ok());
  EXPECT_FALSE(v6.get<T5>().ok());
  EXPECT_EQ(v6, v6);
  EXPECT_NE(v6, v5);

  EXPECT_STATUS_OK(v6.get<T6>());
  EXPECT_EQ(crazy, *v6.get<T6>());
}

TEST(Value, ProtoConversionBool) {
  for (auto b : {true, false}) {
    Value const v(b);
    auto const p = internal::ToProto(Value(b));
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::BOOL, p.first.code());
    EXPECT_EQ(b, p.second.bool_value());
  }
}

TEST(Value, ProtoConversionInt64) {
  auto const min64 = std::numeric_limits<std::int64_t>::min();
  auto const max64 = std::numeric_limits<std::int64_t>::max();
  for (auto x : std::vector<std::int64_t>{min64, -1, 0, 1, 42, max64}) {
    Value const v(x);
    auto const p = internal::ToProto(v);
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::INT64, p.first.code());
    EXPECT_EQ(std::to_string(x), p.second.string_value());
  }
}

TEST(Value, ProtoConversionFloat64) {
  for (auto x : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
    Value const v(x);
    auto const p = internal::ToProto(v);
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::FLOAT64, p.first.code());
    EXPECT_EQ(x, p.second.number_value());
  }

  // Tests special cases
  auto const inf = std::numeric_limits<double>::infinity();
  Value v(inf);
  auto p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::FLOAT64, p.first.code());
  EXPECT_EQ("Infinity", p.second.string_value());

  v = Value(-inf);
  p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::FLOAT64, p.first.code());
  EXPECT_EQ("-Infinity", p.second.string_value());

  auto const nan = std::nan("NaN");
  v = Value(nan);
  p = internal::ToProto(v);
  // Note: NaN is defined to be not equal to everything, including itself, so
  // we instead ensure that it is not equal with EXPECT_NE.
  EXPECT_NE(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::FLOAT64, p.first.code());
  EXPECT_EQ("NaN", p.second.string_value());
}

TEST(Value, ProtoConversionString) {
  for (auto const& x :
       std::vector<std::string>{"", "f", "foo", "12345678901234567890"}) {
    Value const v(x);
    auto const p = internal::ToProto(v);
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::STRING, p.first.code());
    EXPECT_EQ(x, p.second.string_value());
  }
}

TEST(Value, ProtoConversionBytes) {
  for (auto const& x : std::vector<Value::Bytes>{
           Value::Bytes(""), Value::Bytes("f"), Value::Bytes("foo"),
           Value::Bytes("12345678901234567890")}) {
    Value const v(x);
    auto const p = internal::ToProto(v);
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::BYTES, p.first.code());
    EXPECT_EQ(internal::Base64Encode(x.data), p.second.string_value());
  }
}

TEST(Value, ProtoConversionTimestamp) {
  for (time_t t : {
           -9223372035LL,   // near the limit of 64-bit/ns system_clock
           -2147483649LL,   // below min 32-bit int
           -2147483648LL,   // min 32-bit int
           -1LL, 0LL, 1LL,  // around the unix epoch
           1561147549LL,    // contemporary
           2147483647LL,    // max 32-bit int
           2147483648LL,    // above max 32-bit int
           9223372036LL     // near the limit of 64-bit/ns system_clock
       }) {
    auto tp = std::chrono::system_clock::from_time_t(t);
    for (auto nanos : {-1, 0, 1}) {
      auto ts = std::chrono::time_point_cast<Timestamp::duration>(tp) +
                std::chrono::nanoseconds(nanos);
      Value const v(ts);
      auto const p = internal::ToProto(v);
      EXPECT_EQ(v, internal::FromProto(p.first, p.second));
      EXPECT_EQ(google::spanner::v1::TypeCode::TIMESTAMP, p.first.code());
      EXPECT_EQ(internal::TimestampToString(ts), p.second.string_value());
    }
  }
}

TEST(Value, ProtoConversionDate) {
  for (auto x : {
           Date(1582, 10, 15),  // start of Gregorian calendar
           Date(1677, 9, 21),   // before system_clock limit
           Date(1901, 12, 13),  // around min 32-bit seconds limit
           Date(1970, 1, 1),    // the unix epoch
           Date(2019, 6, 21),   // contemporary
           Date(2038, 1, 19),   // around max 32-bit seconds limit
           Date(2262, 4, 12)    // after system_clock limit
       }) {
    Value const v(x);
    auto const p = internal::ToProto(v);
    EXPECT_EQ(v, internal::FromProto(p.first, p.second));
    EXPECT_EQ(google::spanner::v1::TypeCode::DATE, p.first.code());
    EXPECT_EQ(internal::DateToString(x), p.second.string_value());
  }
}

TEST(Value, ProtoConversionArray) {
  std::vector<std::int64_t> data{1, 2, 3};
  Value const v(data);
  auto const p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::ARRAY, p.first.code());
  EXPECT_EQ(google::spanner::v1::TypeCode::INT64,
            p.first.array_element_type().code());
  EXPECT_EQ("1", p.second.list_value().values(0).string_value());
  EXPECT_EQ("2", p.second.list_value().values(1).string_value());
  EXPECT_EQ("3", p.second.list_value().values(2).string_value());
}

TEST(Value, ProtoConversionStruct) {
  auto data = std::make_tuple(3.14, std::make_pair("foo", 42));
  Value const v(data);
  auto const p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::STRUCT, p.first.code());

  auto const& field0 = p.first.struct_type().fields(0);
  EXPECT_EQ("", field0.name());
  EXPECT_EQ(google::spanner::v1::TypeCode::FLOAT64, field0.type().code());
  EXPECT_EQ(3.14, p.second.list_value().values(0).number_value());

  auto const& field1 = p.first.struct_type().fields(1);
  EXPECT_EQ("foo", field1.name());
  EXPECT_EQ(google::spanner::v1::TypeCode::INT64, field1.type().code());
  EXPECT_EQ("42", p.second.list_value().values(1).string_value());
}

void SetProtoKind(Value& v, google::protobuf::NullValue x) {
  auto p = internal::ToProto(v);
  p.second.set_null_value(x);
  v = internal::FromProto(p.first, p.second);
}

void SetProtoKind(Value& v, double x) {
  auto p = internal::ToProto(v);
  p.second.set_number_value(x);
  v = internal::FromProto(p.first, p.second);
}

void SetProtoKind(Value& v, char const* x) {
  auto p = internal::ToProto(v);
  p.second.set_string_value(x);
  v = internal::FromProto(p.first, p.second);
}

void SetProtoKind(Value& v, bool x) {
  auto p = internal::ToProto(v);
  p.second.set_bool_value(x);
  v = internal::FromProto(p.first, p.second);
}

void ClearProtoKind(Value& v) {
  auto p = internal::ToProto(v);
  p.second.clear_kind();
  v = internal::FromProto(p.first, p.second);
}

TEST(Value, GetBadBool) {
  Value v(true);
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<bool>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<bool>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<bool>().ok());

  SetProtoKind(v, "hello");
  EXPECT_FALSE(v.get<bool>().ok());
}

TEST(Value, GetBadDouble) {
  Value v(0.0);
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<double>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<double>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<double>().ok());

  SetProtoKind(v, "bad string");
  EXPECT_FALSE(v.get<double>().ok());
}

TEST(Value, GetBadString) {
  Value v("hello");
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<std::string>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<std::string>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<std::string>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<std::string>().ok());
}

TEST(Value, GetBadBytes) {
  Value v(Value::Bytes("hello"));
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<Value::Bytes>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<Value::Bytes>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<Value::Bytes>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<Value::Bytes>().ok());
}

TEST(Value, GetBadInt) {
  Value v(42);
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, "");
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<std::int64_t>().ok());

  SetProtoKind(v, "123blah");
  EXPECT_FALSE(v.get<std::int64_t>().ok());
}

TEST(Value, GetBadTimestamp) {
  Value v(Timestamp{});
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<Timestamp>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<Timestamp>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<Timestamp>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<Timestamp>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<Timestamp>().ok());
}

TEST(Value, GetBadDate) {
  Value v(Date{});
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<Date>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<Date>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<Date>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<Date>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<Date>().ok());
}

TEST(Value, GetBadOptional) {
  Value v(optional<double>{});
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<optional<double>>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<optional<double>>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<optional<double>>().ok());
}

TEST(Value, GetBadArray) {
  Value v(std::vector<double>{});
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<std::vector<double>>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<std::vector<double>>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<std::vector<double>>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<std::vector<double>>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<std::vector<double>>().ok());
}

TEST(Value, GetBadStruct) {
  Value v(std::tuple<bool>{});
  ClearProtoKind(v);
  EXPECT_FALSE(v.get<std::tuple<bool>>().ok());

  SetProtoKind(v, google::protobuf::NULL_VALUE);
  EXPECT_FALSE(v.get<std::tuple<bool>>().ok());

  SetProtoKind(v, true);
  EXPECT_FALSE(v.get<std::tuple<bool>>().ok());

  SetProtoKind(v, 0.0);
  EXPECT_FALSE(v.get<std::tuple<bool>>().ok());

  SetProtoKind(v, "blah");
  EXPECT_FALSE(v.get<std::tuple<bool>>().ok());
}

TEST(Value, NamedStructCxx11_ToProto) {
  static_assert(internal::IsNamedStruct<ns::NamedStructCxx11>::value, "a");

  Value v(ns::NamedStructCxx11{1, "Elena", "Campbell", Date(1970, 01, 01)});
  auto const p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::STRUCT, p.first.code());

  google::spanner::v1::Type expected_type;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        code: STRUCT
        struct_type {
          fields {
            name: "id"
            type { code: INT64 }
          }
          fields {
            name: "FirstName"
            type { code: STRING }
          }
          fields {
            name: "LastName"
            type { code: STRING }
          }
          fields {
            name: "BirthDate"
            type { code: DATE }
          }
        })pb", &expected_type));
  EXPECT_THAT(p.first, IsProtoEqual(expected_type));

  google::protobuf::Value expected_value;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        list_value {
          values { string_value: "1" }
          values { string_value: "Elena" }
          values { string_value: "Campbell" }
          values { string_value: "1970-01-01" }
        })pb", &expected_value));
  EXPECT_THAT(p.second, IsProtoEqual(expected_value));
}

TEST(Value, NamedStructCxx11_Array) {
  std::vector<ns::NamedStructCxx11> array{
      {1, "Elena", "Campbell", Date(1970, 01, 01)},
      {2, "Gabriel", "Wright", Date(1980, 02, 02)},
  };
  Value v(array);
  auto extracted = v.get<std::vector<ns::NamedStructCxx11>>();
  EXPECT_STATUS_OK(extracted);
  EXPECT_THAT(*extracted, ::testing::ElementsAreArray(array));

  auto const p = internal::ToProto(v);

  google::spanner::v1::Type expected_type;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        code: ARRAY
        array_element_type {
          code: STRUCT
          struct_type {
            fields {
              name: "id"
              type { code: INT64 }
            }
            fields {
              name: "FirstName"
              type { code: STRING }
            }
            fields {
              name: "LastName"
              type { code: STRING }
            }
            fields {
              name: "BirthDate"
              type { code: DATE }
            }
          }
        })pb", &expected_type));
  EXPECT_THAT(p.first, IsProtoEqual(expected_type));

  google::protobuf::Value expected_value;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        list_value {
          values {
            list_value {
              values { string_value: "1" }
              values { string_value: "Elena" }
              values { string_value: "Campbell" }
              values { string_value: "1970-01-01" }
            }
          }
          values {
            list_value {
              values { string_value: "2" }
              values { string_value: "Gabriel" }
              values { string_value: "Wright" }
              values { string_value: "1980-02-02" }
            }
          }
        })pb", &expected_value));
  EXPECT_THAT(p.second, IsProtoEqual(expected_value));
}

#if __cplusplus == 201703L
TEST(Value, NamedStructCxx17_ToProto) {
  static_assert(internal::IsNamedStruct<ns::NamedStructCxx17>::value, "a");

  Value v(ns::NamedStructCxx17{1, "Elena", "Campbell"});
  auto const p = internal::ToProto(v);
  EXPECT_EQ(v, internal::FromProto(p.first, p.second));
  EXPECT_EQ(google::spanner::v1::TypeCode::STRUCT, p.first.code());

  google::spanner::v1::Type expected_type;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        code: STRUCT
        struct_type {
          fields {
            name: "id"
            type { code: INT64 }
          }
          fields {
            name: "first_name"
            type { code: STRING }
          }
          fields {
            name: "last_name"
            type { code: STRING }
          }
        })pb", &expected_type));
  EXPECT_THAT(p.first, IsProtoEqual(expected_type));

  google::protobuf::Value expected_value;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        list_value {
          values { string_value: "1" }
          values { string_value: "Elena" }
          values { string_value: "Campbell" }
        })pb", &expected_value));
  EXPECT_THAT(p.second, IsProtoEqual(expected_value));
}

TEST(Value, NamedStructCxx17_Array) {
  std::vector<ns::NamedStructCxx17> array{
      {1, "Elena", "Campbell"},
      {2, "Gabriel", "Wright"},
  };
  Value v(array);
  auto extracted = v.get<std::vector<ns::NamedStructCxx17>>();
  EXPECT_STATUS_OK(extracted);
  EXPECT_THAT(*extracted, ::testing::ElementsAreArray(array));

  auto const p = internal::ToProto(v);
  google::spanner::v1::Type expected_type;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        code: ARRAY
        array_element_type {
          code: STRUCT
          struct_type {
            fields {
              name: "id"
              type { code: INT64 }
            }
            fields {
              name: "first_name"
              type { code: STRING }
            }
            fields {
              name: "last_name"
              type { code: STRING }
            }
          }
        })pb", &expected_type));
  EXPECT_THAT(p.first, IsProtoEqual(expected_type));

  google::protobuf::Value expected_value;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        list_value {
          values {
            list_value {
              values { string_value: "1" }
              values { string_value: "Elena" }
              values { string_value: "Campbell" }
            }
          }
          values {
            list_value {
              values { string_value: "2" }
              values { string_value: "Gabriel" }
              values { string_value: "Wright" }
            }
          }
        })pb", &expected_value));
  EXPECT_THAT(p.second, IsProtoEqual(expected_value));
}

#endif  // __cplusplus == 201703L

}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google
