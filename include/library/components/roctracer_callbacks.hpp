// Copyright (c) 2018 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// with the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
//
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimers in the
// documentation and/or other materials provided with the distribution.
//
// * Neither the names of Advanced Micro Devices, Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this Software without specific prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.

#pragma once

#include "library/components/roctracer.hpp"
#include "library/config.hpp"
#include "library/debug.hpp"
#include "library/dynamic_library.hpp"
#include "library/perfetto.hpp"
#include "library/ptl.hpp"

#include <roctracer.h>
#include <roctracer_ext.h>
#include <roctracer_hcc.h>
#include <roctracer_hip.h>

#define AMD_INTERNAL_BUILD 1
#include <ext/hsa_rt_utils.hpp>
#include <roctracer_hsa.h>

#include <iostream>
#include <memory>

// Macro to check ROC-tracer calls status
#define ROCTRACER_CALL(call)                                                             \
    do                                                                                   \
    {                                                                                    \
        int err = call;                                                                  \
        if(err != 0)                                                                     \
        {                                                                                \
            std::cerr << roctracer_error_string() << " in: " << #call << std::flush;     \
        }                                                                                \
    } while(0)

namespace omnitrace
{
using hsa_timer_t = hsa_rt_utils::Timer;
using timestamp_t = hsa_timer_t::timestamp_t;
using roctracer_bundle_t =
    tim::component_bundle<api::omnitrace, comp::roctracer_data, comp::wall_clock>;
using roctracer_hsa_bundle_t =
    tim::component_bundle<api::omnitrace, comp::roctracer_data>;
using roctracer_functions_t = std::vector<std::pair<std::string, std::function<void()>>>;

std::unique_ptr<hsa_timer_t>&
get_hsa_timer();

// HSA API callback function
void
hsa_api_callback(uint32_t domain, uint32_t cid, const void* callback_data, void* arg);

void
hsa_activity_callback(uint32_t op, activity_record_t* record, void* arg);

void
hip_exec_activity_callbacks(int64_t _tid);

// HIP API callback function
void
hip_api_callback(uint32_t domain, uint32_t cid, const void* callback_data, void* arg);

// Activity tracing callback
void
hip_activity_callback(const char* begin, const char* end, void*);

bool&
roctracer_is_setup();

roctracer_functions_t&
roctracer_setup_routines();

roctracer_functions_t&
roctracer_tear_down_routines();
}  // namespace omnitrace