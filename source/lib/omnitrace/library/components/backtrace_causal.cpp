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

#include "library/components/backtrace_causal.hpp"
#include "library/causal/data.hpp"
#include "library/causal/delay.hpp"
#include "library/causal/experiment.hpp"
#include "library/config.hpp"
#include "library/debug.hpp"
#include "library/runtime.hpp"
#include "library/state.hpp"
#include "library/thread_data.hpp"
#include "library/thread_info.hpp"
#include "library/tracing.hpp"
#include "library/utility.hpp"

#include <timemory/components/timing/backends.hpp>
#include <timemory/components/timing/wall_clock.hpp>
#include <timemory/mpl/concepts.hpp>
#include <timemory/mpl/types.hpp>
#include <timemory/process/threading.hpp>
#include <timemory/units.hpp>
#include <timemory/utility/backtrace.hpp>

#include <atomic>
#include <ctime>
#include <type_traits>

namespace omnitrace
{
namespace component
{
namespace
{
using ::tim::backtrace::get_unw_signal_frame_stack;
using delay_statistics_t =
    thread_data<identity<tim::statistics<int64_t>>, category::sampling>;
using in_use_t = thread_data<identity<bool>, category::sampling>;

struct scoped_in_use
{
    scoped_in_use(int64_t _tid = threading::get_id())
    : value{ in_use_t::instances().at(_tid) }
    {
        value = true;
    }
    ~scoped_in_use() { value = false; }

    bool& value;
};

auto
is_in_use(int64_t _tid = threading::get_id())
{
    return in_use_t::instances().at(_tid);
}

auto samples = std::map<uint32_t, std::set<backtrace_causal::sample_data>>{};
}  // namespace

void
backtrace_causal::start()
{
    set_causal_state(CausalState::Enabled);
}

void
backtrace_causal::stop()
{
    set_causal_state(CausalState::Disabled);
}

void
backtrace_causal::sample(int _sig)
{
    constexpr size_t  depth        = causal::unwind_depth;
    constexpr int64_t ignore_depth = causal::unwind_offset;

    // update the last sample for backtrace signal(s) even when in use
    static thread_local int64_t _last_sample = 0;

    if(is_in_use())
    {
        if(_sig >= get_causal_backtrace_signal()) _last_sample = tracing::now();
        return;
    }
    scoped_in_use _in_use{};

    m_index = causal::experiment::get_index();
    m_stack = get_unw_signal_frame_stack<depth, ignore_depth>();
    // the batch handler timer delivers a signal according to the thread CPU
    // clock, ensuring that setting the current selection and processing the
    // delays only happens when the thread is active
    if(_sig == get_causal_batch_handler_signal())
    {
        if(!causal::experiment::is_active())
            causal::set_current_selection(m_stack);
        else
            causal::delay::process();
    }
    else if(_sig >= get_causal_backtrace_signal())
    {
        auto  _this_sample = tracing::now();
        auto& _period_stat = delay_statistics_t::instance();
        if(_last_sample > 0) _period_stat += (_this_sample - _last_sample);
        _last_sample = _this_sample;

        if(causal::experiment::is_active() && causal::experiment::is_selected(m_stack))
        {
            m_selected = true;
            causal::experiment::add_selected();
            // compute the delay time based on the rate of taking samples,
            // unless we have taken less than 10, in which case, we just
            // use the pre-computed value.
            auto _delay =
                (_period_stat.get_count() < 10)
                    ? causal::experiment::get_delay()
                    : (_period_stat.get_mean() * causal::experiment::get_delay_scaling());
            causal::delay::get_local() += _delay;
        }
    }
}

template <typename Tp>
Tp
backtrace_causal::get_period(uint64_t _units)
{
    using cast_type = std::conditional_t<std::is_floating_point<Tp>::value, Tp, double>;

    double _realtime_freq =
        (get_use_sampling_realtime()) ? get_sampling_real_freq() : 0.0;
    double _cputime_freq = (get_use_sampling_cputime()) ? get_sampling_cpu_freq() : 0.0;

    auto    _freq        = std::max<double>(_realtime_freq, _cputime_freq);
    double  _period      = 1.0 / _freq;
    int64_t _period_nsec = static_cast<int64_t>(_period * units::sec) % units::sec;
    return static_cast<Tp>(_period_nsec) / static_cast<cast_type>(_units);
}

tim::statistics<int64_t>
backtrace_causal::get_period_stats()
{
    scoped_in_use _in_use{};
    auto          _data = tim::statistics<int64_t>{};
    for(size_t i = 0; i < max_supported_threads; ++i)
    {
        scoped_in_use _thr_in_use{ static_cast<int64_t>(i) };
        const auto&   itr = delay_statistics_t::instances().at(i);
        if(itr.get_count() > 1) _data += itr;
    }
    return _data;
}

void
backtrace_causal::reset_period_stats()
{
    scoped_in_use _in_use{};
    for(size_t i = 0; i < max_supported_threads; ++i)
    {
        scoped_in_use _thr_in_use{ static_cast<int64_t>(i) };
        delay_statistics_t::instances().at(i).reset();
    }
}

std::set<backtrace_causal::sample_data>
backtrace_causal::get_samples(uint32_t _index)
{
    return samples[_index];
}

std::map<uint32_t, std::set<backtrace_causal::sample_data>>
backtrace_causal::get_samples()
{
    return samples;
}

void
backtrace_causal::add_sample(uint32_t _index, uintptr_t _v)
{
    auto& _samples = samples[_index];
    auto  _value   = sample_data{ _v };
    _value.count   = 1;
    auto itr       = _samples.find(_value);
    if(itr == _samples.end())
        _samples.emplace(_value);
    else
        itr->count += 1;
}

void
backtrace_causal::add_samples(uint32_t _index, const std::vector<uintptr_t>& _v)
{
    for(const auto& itr : _v)
        add_sample(_index, itr);
}
}  // namespace component
}  // namespace omnitrace

#define INSTANTIATE_BT_CAUSAL_PERIOD(TYPE)                                               \
    template TYPE omnitrace::component::backtrace_causal::get_period<TYPE>(uint64_t);

INSTANTIATE_BT_CAUSAL_PERIOD(float)
INSTANTIATE_BT_CAUSAL_PERIOD(double)
INSTANTIATE_BT_CAUSAL_PERIOD(int64_t)
INSTANTIATE_BT_CAUSAL_PERIOD(uint64_t)
