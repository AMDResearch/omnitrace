// MIT License
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "library/common.hpp"
#include "library/defines.hpp"
#include "library/timemory.hpp"

#include <timemory/components/gotcha/backends.hpp>
#include <timemory/mpl/macros.hpp>

#include <array>
#include <cstddef>
#include <string>

namespace omnitrace
{
namespace causal
{
using timespec_t = struct timespec;
// this is used to wrap pthread_mutex()
struct unblocking_gotcha : comp::base<unblocking_gotcha, void>
{
    static constexpr size_t gotcha_capacity = 8;

    TIMEMORY_DEFAULT_OBJECT(unblocking_gotcha)

    // string id for component
    static std::string label();
    static std::string description();
    static void        preinit();

    // generate the gotcha wrappers
    static void configure();
    static void shutdown();

    static void start();
    static void stop();

    static void set_data(const comp::gotcha_data&);
};

using unblocking_gotcha_t =
    comp::gotcha<unblocking_gotcha::gotcha_capacity,
                 tim::lightweight_tuple<unblocking_gotcha>, category::causal>;
}  // namespace causal
}  // namespace omnitrace

OMNITRACE_DEFINE_CONCRETE_TRAIT(prevent_reentry, causal::unblocking_gotcha_t, false_type)
OMNITRACE_DEFINE_CONCRETE_TRAIT(static_data, causal::unblocking_gotcha_t, true_type)