# ~~~
# Copyright 2019 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ~~~

# Find out what flags turn on all available warnings and turn those warnings
# into errors.
include(CheckCXXCompilerFlag)

function(google_cloud_cpp_add_option_if_supported target scope option)
    check_cxx_compiler_flag("${option}" option_is_supported)
    if (option_is_supported)
        target_compile_options("${target}" "${scope}" "${option}")
    endif ()
endfunction ()

function (google_cloud_cpp_add_common_options target)
    # Enable -Wall -Wextra and -Werror
    # Disable -Wmismatched-tags because libstdc++ defines `std::tuple_size` as
    # both a `struct` and a `class` template. If the flag is enabled it is impossible
    # to specialize this type.
    foreach (option "-Wall" "-Wextra" "-Wno-mismatched-tags" "-Werror")
        google_cloud_cpp_add_option_if_supported("${target}" PRIVATE "${option}")
    endforeach()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
        AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 5.0)
        # With GCC 4.x this warning is too noisy to be useful.
        target_compile_options(${target}
                               PRIVATE "-Wno-missing-field-initializers")
    endif ()
endfunction ()
