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
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <jitana/jitana.hpp>

BOOST_AUTO_TEST_CASE(check_equality)
{
    jitana::insn i0 = jitana::insn_move(jitana::opcode::op_move, {{0, 1}}, {});
    jitana::insn i1 = jitana::insn_move(jitana::opcode::op_move, {{0, 1}}, {});
    jitana::insn i2 = jitana::insn_move(jitana::opcode::op_move, {{0, 2}}, {});

    BOOST_TEST(i0 == i0);
    BOOST_TEST(i0 == i1);
    BOOST_TEST(i0 != i2);
}
