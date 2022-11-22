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
#include "library/components/fwd.hpp"
#include "library/defines.hpp"
#include "timemory/components/base/declaration.hpp"
#include "timemory/components/timing/wall_clock.hpp"
#include "timemory/mpl/type_traits.hpp"
#include "timemory/utility/types.hpp"

#include <timemory/components/base.hpp>
#include <timemory/hash/types.hpp>
#include <timemory/macros/language.hpp>
#include <timemory/mpl/concepts.hpp>
#include <timemory/tpls/cereal/cereal.hpp>

#include <cstdint>
#include <unordered_map>

namespace omnitrace
{
namespace causal
{
struct progress_point : comp::base<progress_point, int64_t>
{
    using base_type     = comp::base<progress_point, int64_t>;
    using value_type    = int64_t;
    using hash_type     = tim::hash_value_t;
    using iterator_type = progress_point*;

    static std::string label();
    static std::string description();

    TIMEMORY_DEFAULT_OBJECT(progress_point)

    static value_type record() noexcept
    {
        return tim::get_clock_real_now<int64_t, std::nano>();
    }

    double get() const noexcept { return load() / static_cast<double>(get_unit()); }
    auto   get_display() const noexcept { return get(); }

    void start() noexcept { value = record(); }
    void stop() noexcept { accum += (value = (record() - value)); }

    void set_iterator(iterator_type _v) { m_iterator = _v; }
    auto get_iterator() const { return m_iterator; }

    void print(std::ostream& os) const;

    void set_hash(hash_type _v) { m_hash = _v; }
    auto get_hash() const { return m_hash; }

    template <typename ArchiveT>
    void save(ArchiveT& ar, const unsigned _version) const
    {
        namespace cereal = ::tim::cereal;

        ar(cereal::make_nvp("hash", m_hash),
           cereal::make_nvp("name", std::string{ tim::get_hash_identifier(m_hash) }));
        base_type::save(ar, _version);
    }

private:
    hash_type       m_hash     = 0;
    progress_point* m_iterator = nullptr;
};

std::unordered_map<tim::hash_value_t, progress_point>
get_progress_points();
}  // namespace causal
}  // namespace omnitrace

OMNITRACE_DEFINE_CONCRETE_TRAIT(uses_storage, causal::progress_point, false_type)
OMNITRACE_DEFINE_CONCRETE_TRAIT(flat_storage, causal::progress_point, true_type)
OMNITRACE_DEFINE_CONCRETE_TRAIT(uses_timing_units, causal::progress_point, true_type)
OMNITRACE_DEFINE_CONCRETE_TRAIT(is_timing_category, causal::progress_point, true_type)

namespace tim
{
namespace operation
{
template <>
struct push_node<omnitrace::causal::progress_point>
{
    using type = omnitrace::causal::progress_point;

    TIMEMORY_DEFAULT_OBJECT(push_node)

    push_node(type& _obj, scope::config _scope, hash_value_t _hash,
              int64_t _tid = threading::get_id())
    {
        (*this)(_obj, _scope, _hash, _tid);
    }

    void operator()(type& _obj, scope::config, hash_value_t _hash,
                    int64_t _tid = threading::get_id()) const;
};

template <>
struct pop_node<omnitrace::causal::progress_point>
{
    using type = omnitrace::causal::progress_point;

    TIMEMORY_DEFAULT_OBJECT(pop_node)

    pop_node(type& _obj, int64_t _tid = threading::get_id()) { (*this)(_obj, _tid); }

    void operator()(type& _obj, int64_t _tid = threading::get_id()) const;
};
}  // namespace operation
}  // namespace tim
