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

#include "common/join.hpp"
#include "library/defines.hpp"

#include <timemory/api.hpp>
#include <timemory/backends/dmp.hpp>
#include <timemory/backends/process.hpp>
#include <timemory/utility/demangle.hpp>
#include <timemory/utility/filepath.hpp>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

TIMEMORY_DEFINE_NS_API(api, omnitrace)
TIMEMORY_DEFINE_NS_API(api, sampling)
TIMEMORY_DEFINE_NS_API(api, rocm_smi)
TIMEMORY_DEFINE_NS_API(api, rccl)

namespace omnitrace
{
namespace api      = ::tim::api;       // NOLINT
namespace category = ::tim::category;  // NOLINT
namespace filepath = ::tim::filepath;  // NOLINT

using ::tim::demangle;      // NOLINT
using ::tim::get_env;       // NOLINT
using ::tim::try_demangle;  // NOLINT
}  // namespace omnitrace

// same sort of functionality as python's " ".join([...])
#if !defined(JOIN)
#    define JOIN(...) ::omnitrace::common::join(__VA_ARGS__)
#endif
