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
#include "google/cloud/internal/disjunction.h"
#include "google/cloud/internal/invoke_result.h"
#include <tuple>
#include <type_traits>

namespace google {
namespace cloud {
namespace spanner {
inline namespace SPANNER_CLIENT_NS {
namespace internal {

/**
 * The default implementation for GetElementName().
 *
 * By default we use this function to get the element (field) names using
 * using a template member function `get_element_name<N>()`. Applications can
 * define this function in their own namespace to override this behavior via
 * ADL.
 */
template <std::size_t N, typename T>
auto GetElementName(T const& t) -> decltype(t.template get_element_name<N>()) {
  return t.template get_element_name<N>();
}

// The default case for SFINAE. The constant type is not `std::size_t`.
template <typename T, typename AlwaysVoid>
struct NumElementsImpl : std::integral_constant<int, 0> {};

// Specialize for any type that has specialized `std::tuple_size<T>`
template <typename T>
struct NumElementsImpl<T, decltype(void(std::tuple_size<T>::value))>
    : public std::tuple_size<T> {};

/**
 * A metafunction that returns the number of elements in a named structure.
 *
 * Applications can specialize `std::tuple_size<>` for their type (as they would
 * do for any type supporting structured binding), to use their types with
 * the library.
 *
 * @par Example
 *
 * @code
 * using Type = std::tuple<int, char, bool>;
 * assert(NumElements<Type>::value == 3);
 * @endcode
 */
template <typename T>
using NumElements = NumElementsImpl<typename std::decay<T>::type, void>;

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
  std::forward<F>(f)(GetElementName<I>(t), std::forward<decltype(e)>(e),
                     std::forward<Args>(args)...);
  ForEachNamed<I + 1>(std::forward<T>(t), std::forward<F>(f),
                      std::forward<Args>(args)...);
}

template <typename AlwaysVoid, typename T>
struct HasNumElementsImpl : public std::false_type {};

template <typename T>
struct HasNumElementsImpl<decltype(void(NumElements<T>::value)), T>
    : public std::true_type {};

template <typename T>
struct HasNumElements
    : public std::conditional<
          HasNumElementsImpl<void, T>::value,
          std::is_same<std::size_t, typename std::decay<decltype(
                                        NumElements<T>::value)>::type>,
          std::false_type>::type {};

template <typename T, std::size_t I, typename AlwaysVoid>
struct GetElementNameInvoker {
  void operator()() const;
};

template <typename T, std::size_t I>
struct GetElementNameInvoker<
    T, I, decltype(void(GetElementName<I>(std::declval<T const&>())))> {
  auto operator()(T const& s) const -> decltype(GetElementName<I>(s));
};

template <typename T, std::size_t I>
struct HasElementName : public google::cloud::internal::is_invocable<
                            GetElementNameInvoker<T, I, void>, T const&> {};

template <typename T, std::size_t I>
struct HasAllElementNamesImpl;
template <typename T>
struct HasAllElementNamesImpl<T, 0> : public HasElementName<T, 0> {};
template <typename T, std::size_t I>
struct HasAllElementNamesImpl
    : public std::conditional<HasElementName<T, I>::value,
                              HasAllElementNamesImpl<T, I - 1>,
                              std::false_type>::type {};

template <typename T, std::size_t I>
struct ElementNameIsConvertibleToString
    : public std::is_convertible<
          google::cloud::internal::invoke_result_t<
              GetElementNameInvoker<T, I, void>, T const&>,
          std::string> {};

template <typename T, std::size_t I>
struct AllElementNamesAreStringsImpl;

template <typename T>
struct AllElementNamesAreStringsImpl<T, 0>
    : public ElementNameIsConvertibleToString<T, 0> {};

template <typename T, std::size_t I>
struct AllElementNamesAreStringsImpl
    : public std::conditional<ElementNameIsConvertibleToString<T, I>::value,
                              ElementNameIsConvertibleToString<T, I - 1>,
                              std::false_type>::type {};

template <typename T>
struct AllElementNamesAreStrings
    : public std::conditional<
          HasAllElementNamesImpl<T, NumElements<T>::value>::value,
          AllElementNamesAreStringsImpl<T, NumElements<T>::value>,
          std::false_type>::type {};

template <typename T>
struct IsNamedStruct : public std::conditional<HasNumElements<T>::value,
                                               AllElementNamesAreStrings<T>,
                                               std::false_type>::type {};

}  // namespace internal
}  // namespace SPANNER_CLIENT_NS
}  // namespace spanner
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_SPANNER_GOOGLE_CLOUD_SPANNER_INTERNAL_TUPLE_UTILS_H_
