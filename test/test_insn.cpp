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
