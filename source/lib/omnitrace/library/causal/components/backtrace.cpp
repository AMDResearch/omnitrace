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

#include "library/causal/components/backtrace.hpp"
#include "core/concepts.hpp"
#include "core/config.hpp"
#include "core/debug.hpp"
#include "core/state.hpp"
#include "core/utility.hpp"
#include "library/causal/data.hpp"
#include "library/causal/delay.hpp"
#include "library/causal/experiment.hpp"
#include "library/perf.hpp"
#include "library/runtime.hpp"
#include "library/thread_data.hpp"
#include "library/thread_info.hpp"
#include "library/tracing.hpp"

#include <timemory/components/timing/backends.hpp>
#include <timemory/components/timing/wall_clock.hpp>
#include <timemory/mpl/concepts.hpp>
#include <timemory/mpl/type_traits.hpp>
#include <timemory/mpl/types.hpp>
#include <timemory/process/threading.hpp>
#include <timemory/units.hpp>
#include <timemory/utility/backtrace.hpp>

#include <atomic>
#include <ctime>
#include <execinfo.h>
#include <type_traits>

namespace omnitrace
{
namespace causal
{
namespace component
{
namespace
{
using ::tim::backtrace::get_unw_signal_frame_stack_raw;

auto&
get_delay_statistics()
{
    using thread_data_t =
        thread_data<identity<tim::statistics<int64_t>>, category::sampling>;

    static_assert(
        use_placement_new_when_generating_unique_ptr<thread_data_t>::value,
        "delay statistics thread data should use placement new to allocate unique_ptr");

    static auto& _v = thread_data_t::instance(construct_on_init{});
    return _v;
}

int realtime_signal = 0;
int cputime_signal  = 0;
int overflow_signal = 0;

void
generic_global_init()
{
    // do not delete these lines. The thread data needs to be allocated
    // before it is called in sampler or else a deadlock will occur when
    // the sample interrupts a malloc call
    if(realtime_signal + cputime_signal + overflow_signal == 0)
    {
        (void) get_delay_statistics();
        realtime_signal = get_sampling_realtime_signal();
        cputime_signal  = get_sampling_cputime_signal();
        overflow_signal = get_sampling_overflow_signal();
    }
}
}  // namespace

void
overflow::global_init()
{
    // do not delete these lines.
    generic_global_init();
}

void
backtrace::global_init()
{
    // do not delete these lines.
    generic_global_init();
}

void
sample_rate::sample(int _sig)
{
    if(_sig != realtime_signal) return;

    OMNITRACE_SCOPED_THREAD_STATE(ThreadState::Internal);

    // update the last sample for backtrace signal(s) even when in use
    static thread_local int64_t _last_sample = 0;

    auto  _tid         = threading::get_id();
    auto  _this_sample = tracing::now();
    auto& _period_stat = get_delay_statistics()->at(_tid);

    if(_last_sample > 0) _period_stat += (_this_sample - _last_sample);

    _last_sample = _this_sample;
}

void
overflow::sample(int _sig)
{
    if(_sig == realtime_signal) return;

    OMNITRACE_SCOPED_THREAD_STATE(ThreadState::Internal);

    static thread_local const auto& _tinfo      = thread_info::get();
    auto                            _tid        = _tinfo->index_data->sequent_value;
    auto&                           _perf_event = perf::get_instance(_tid);

    if(!_perf_event) return;

    m_index = causal::experiment::get_index();

    _perf_event->stop();

    for(auto itr : *_perf_event)
    {
        if(itr.is_sample())
        {
            auto _sample_ip = itr.get_ip();
            auto _data      = callchain_t{};
            _data.emplace_back(_sample_ip);
            for(auto ditr : itr.get_callchain())
            {
                if(ditr != _sample_ip) _data.emplace_back(ditr);
                if(_data.size() == _data.capacity()) break;
            }

            if(causal::experiment::is_active() && causal::experiment::is_selected(_data))
            {
                ++m_selected;
                causal::experiment::add_selected();
                auto _delay = 1.0 * units::msec * causal::experiment::get_delay_scaling();
                causal::delay::get_local() += _delay;
            }
            else if(!causal::experiment::is_active())
            {
                causal::set_current_selection(_data);
            }

            m_stack.emplace_back(_data);
        }
    }

    _perf_event->start();

    if(_sig == cputime_signal && causal::experiment::is_active())
        causal::delay::process();
}

void
backtrace::sample(int _sig)
{
    if(_sig == overflow_signal) return;

    constexpr size_t  depth        = ::omnitrace::causal::unwind_depth;
    constexpr int64_t ignore_depth = ::omnitrace::causal::unwind_offset;

    // update the last sample for backtrace signal(s) even when in use
    static thread_local size_t      _protect_flag = 0;
    static thread_local const auto& _tinfo        = thread_info::get();

    if((_protect_flag & 1) == 1 ||
       OMNITRACE_UNLIKELY(!trait::runtime_enabled<causal::component::backtrace>::get()))
    {
        return;
    }

    ++_protect_flag;
    // on RedHat, the unw_step within get_unw_signal_frame_stack_raw involves a mutex lock
    OMNITRACE_SCOPED_THREAD_STATE(ThreadState::Internal);
    m_index = causal::experiment::get_index();
    m_stack = get_unw_signal_frame_stack_raw<depth, ignore_depth>();

    // the batch handler timer delivers a signal according to the thread CPU
    // clock, ensuring that setting the current selection and processing the
    // delays only happens when the thread is active
    if(_sig == cputime_signal)
    {
        if(causal::experiment::is_active())
            causal::delay::process();
        else
            causal::set_current_selection(m_stack);
    }
    else if(_sig == realtime_signal)
    {
        auto  _tid         = _tinfo->index_data->sequent_value;
        auto& _period_stat = get_delay_statistics()->at(_tid);

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
    else
    {
        OMNITRACE_THROW("unhandled signal %i\n", _sig);
    }

    ++_protect_flag;
}

template <typename Tp>
Tp
backtrace::get_period(uint64_t _units)
{
    using cast_type = std::conditional_t<std::is_floating_point<Tp>::value, Tp, double>;

    double  _period      = 1.0 / 1000.0;
    int64_t _period_nsec = static_cast<int64_t>(_period * units::sec) % units::sec;
    return static_cast<Tp>(_period_nsec) / static_cast<cast_type>(_units);
}

tim::statistics<int64_t>
backtrace::get_period_stats()
{
    auto _data = tim::statistics<int64_t>{};
    if(!get_delay_statistics()) return _data;
    for(auto itr : *get_delay_statistics())
    {
        if(itr.get_count() > 1) _data += itr;
    }
    return _data;
}

void
backtrace::reset_period_stats()
{
    for(auto& itr : *get_delay_statistics())
    {
        itr.reset();
    }
}
}  // namespace component
}  // namespace causal
}  // namespace omnitrace

#define INSTANTIATE_BT_CAUSAL_PERIOD(TYPE)                                               \
    template TYPE omnitrace::causal::component::backtrace::get_period<TYPE>(uint64_t);

INSTANTIATE_BT_CAUSAL_PERIOD(float)
INSTANTIATE_BT_CAUSAL_PERIOD(double)
INSTANTIATE_BT_CAUSAL_PERIOD(int64_t)
INSTANTIATE_BT_CAUSAL_PERIOD(uint64_t)
