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

#include "library/binary/address_range.hpp"
#include "library/binary/fwd.hpp"

namespace omnitrace
{
namespace binary
{
struct dwarf_entry
{
    TIMEMORY_DEFAULT_OBJECT(dwarf_entry)

    bool         begin_statement = false;
    bool         end_sequence    = false;
    bool         line_block      = false;
    bool         prologue_end    = false;
    bool         epilogue_begin  = false;
    unsigned int line            = 0;
    int          col             = 0;
    unsigned int vliw_op_index   = 0;
    unsigned int isa             = 0;
    unsigned int discriminator   = 0;
    uintptr_t    address         = 0;
    std::string  file            = {};

    bool is_valid() const;

    bool     operator<(const dwarf_entry&) const;
    bool     operator==(const dwarf_entry&) const;
    bool     operator!=(const dwarf_entry&) const;
    explicit operator bool() const { return is_valid(); }

    static std::deque<dwarf_entry> process_dwarf(int _fd, std::vector<address_range>&);

    template <typename ArchiveT>
    void serialize(ArchiveT&, const unsigned int);
};
}  // namespace binary
}  // namespace omnitrace