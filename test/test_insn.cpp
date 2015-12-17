/*
 * Copyright (c) 2015 Yutaka Tsutano.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define BOOST_TEST_MODULE test_insn
#define BOOST_TEST_INCLUDED
#include <boost/test/unit_test.hpp>

#include <jitana/jitana.hpp>

BOOST_AUTO_TEST_CASE(check_equality)
{
    using opcode = jitana::opcode;

    jitana::insn i0 = jitana::insn_move(opcode::op_move, {{0, 1}}, {});
    jitana::insn i1 = jitana::insn_move(opcode::op_move, {{0, 1}}, {});
    jitana::insn i2 = jitana::insn_move(opcode::op_move, {{0, 2}}, {});
    jitana::insn i4 = jitana::insn_iput(opcode::op_iput, {{0, 1}}, {{1, 2}, 3});
    jitana::insn i5 = jitana::insn_iput(opcode::op_iput, {{0, 1}}, {{1, 2}, 4});
    jitana::insn i6 = jitana::insn_iget(opcode::op_iget, {{0, 1}}, {{1, 2}, 3});

    // Check the equality operators.
    BOOST_CHECK(i0 == i0);
    BOOST_CHECK(i0 == i1);
    BOOST_CHECK(i4 == i4);
    BOOST_CHECK(!(i0 == i2));
    BOOST_CHECK(!(i4 == i5));
    BOOST_CHECK(!(i4 == i6));

    BOOST_CHECK(op(i0) == op(i0));
    BOOST_CHECK(op(i0) == op(i1));
    BOOST_CHECK(op(i4) == op(i4));
    BOOST_CHECK(op(i0) == op(i2));
    BOOST_CHECK(op(i4) == op(i5));
    BOOST_CHECK(op(i4) != op(i6));

    BOOST_CHECK(regs(i0) == regs(i0));
    BOOST_CHECK(regs(i0) == regs(i1));
    BOOST_CHECK(regs(i4) == regs(i4));
    BOOST_CHECK(regs(i0) != regs(i2));
    BOOST_CHECK(regs(i4) == regs(i5));
    BOOST_CHECK(regs(i4) == regs(i6));

    BOOST_CHECK(jitana::const_val<jitana::dex_field_hdl>(i4)
                == jitana::const_val<jitana::dex_field_hdl>(i4));
    BOOST_CHECK(jitana::const_val<jitana::dex_field_hdl>(i4)
                != jitana::const_val<jitana::dex_field_hdl>(i5));
    BOOST_CHECK(jitana::const_val<jitana::dex_field_hdl>(i4)
                != jitana::const_val<jitana::dex_field_hdl>(i6));

#if 0
    // Check the inequality operators.
    // Note: this can only be compiled with Boost 1.58 or later due to a bug:
    // https://svn.boost.org/trac/boost/ticket/8620.
    BOOST_CHECK(!(i0 != i0));
    BOOST_CHECK(!(i0 != i1));
    BOOST_CHECK(!(i4 != i4));
    BOOST_CHECK(i0 != i2);
    BOOST_CHECK(i4 != i5);
    BOOST_CHECK(i4 != i6);
#endif
}
