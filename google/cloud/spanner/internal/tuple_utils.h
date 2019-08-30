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

#ifndef GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_TUPLE_UTILS_H_
#define GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_TUPLE_UTILS_H_

#include "google/cloud/spanner/version.h"
#include <tuple>
#include <type_traits>

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {
namespace internal {

/**
 * Get the field names for a NameStruct.
 *
 * This version gets the field name using a template member function
 * `get_field_name<N>()`, but we always use this function through the
 * GetFieldName adapter, so applications can define the function in their own
 * namespace.
 */
template <std::size_t N, typename T>
auto get_field_name(T const& t) -> decltype(t.template get_field_name<N>()) {
  return t.template get_field_name<N>();
}

/**
 * A metafunction that returns the name of each field in a named structure.
 *
 * @tparam N the index of the field
 * @tparam NamedStruct the type of the named struct.
 * @param s the named structure value, typically unused.
 */
template <std::size_t N, typename NamedStruct/*,
          typename std::enable_if<google::cloud::internal::is_invocable<
                                      decltype(&NamedStruct::get_field_name),
                                      NamedStruct const&>::value,
                                  int>::type = 0*/>
auto GetFieldName(NamedStruct const& s)
-> decltype(get_field_name<N>(std::declval<NamedStruct const&>())) {
  return get_field_name<N>(s);
}

// A metafunction that returns the number of elements in any class template
// that takes a variable number of arguments.
//
// Example:
//
//     using Type = std::tuple<int, char, bool>;
//     assert(NumElements<Type>::value == 3);
//
template <typename T>
struct NumElementsImpl : public std::tuple_size<T> {};

template <template <typename...> class T, typename... Ts>
struct NumElementsImpl<T<Ts...>> {
  static const std::size_t value = sizeof...(Ts);
};
template <template <typename...> class T, typename... Ts>
std::size_t const NumElementsImpl<T<Ts...>>::value;  // Declares storage

template <typename T>
using NumElements = NumElementsImpl<typename std::decay<T>::type>;

// Similar to `std::get<I>(std::tuple)`, except that this function is an
// extension point for `ForEach` (below) that callers can define in their own
// namespace for their own types to add support for `ForEach`. This overload
// simply forwards to `std::get`.
template <std::size_t I, typename Tup>
auto GetElement(Tup&& tup) -> decltype(std::get<I>(std::forward<Tup>(tup))) {
  using std::get;
  return get<I>(std::forward<Tup>(tup));
}

/// Used if `Tup` provides `.get<I>()` accessors.
template <std::size_t I, typename Tup, typename...>
auto GetElement(Tup&& tup) -> decltype(tup.template get<I>()) {
  return tup.template get<I>();
}

// Base case of `ForEach` that is called at the end of iterating a tuple.
// See the docs for the next overload to see how to use `ForEach`.
template <std::size_t I = 0, typename T, typename F, typename... Args>
typename std::enable_if<I == NumElements<T>::value, void>::type ForEach(
    T&&, F&&, Args&&...) {}

// This function template iterates the elements of a tuple-like type, calling
// the given functor with each element of the container as well as any
// additional arguments that were provided. A tuple-like type is any fixed-size
// heterogeneous container that has a `GetElement<I>(T)` function that can be
// found via ADL. The given functor should be able to accept each type in the
// container. All arguments are perfect-forwarded to the functor, so the
// functor may choose to accept the tuple arguments by value, const-ref, or
// even non-const reference, in which case the elements inside the tuple may be
// modified.
//
// Example:
//
//     struct Stringify {
//       template <typename T>
//       void operator()(T& t, std::vector<std::string>& out) const {
//         out.push_back(std::to_string(t));
//       }
//     };
//     auto tup = std::make_tuple(true, 42);
//     std::vector<std::string> v;
//     internal::ForEach(tup, Stringify{}, v);
//     EXPECT_THAT(v, testing::ElementsAre("1", "42"));
//
template <std::size_t I = 0, typename T, typename F, typename... Args>
typename std::enable_if<(I < NumElements<T>::value), void>::type ForEach(
    T&& t, F&& f, Args&&... args) {
  auto&& e = GetElement<I>(std::forward<T>(t));
  std::forward<F>(f)(std::forward<decltype(e)>(e), std::forward<Args>(args)...);
  ForEach<I + 1>(std::forward<T>(t), std::forward<F>(f),
                 std::forward<Args>(args)...);
}

template <std::size_t I = 0, typename T, typename F, typename... Args>
typename std::enable_if<I == NumElements<T>::value, void>::type ForEachNamed(
    T&&, F&&, Args&&...) {}

template <std::size_t I = 0, typename T, typename F, typename... Args>
typename std::enable_if<(I < NumElements<T>::value), void>::type ForEachNamed(
    T&& t, F&& f, Args&&... args) {
  auto&& e = GetElement<I>(std::forward<T>(t));
  std::forward<F>(f)(GetFieldName<I>(t), std::forward<decltype(e)>(e),
                     std::forward<Args>(args)...);
  ForEachNamed<I + 1>(std::forward<T>(t), std::forward<F>(f),
                      std::forward<Args>(args)...);
}

}  // namespace internal
}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_TUPLE_UTILS_H_
