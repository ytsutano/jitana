#define BOOST_TEST_MODULE test_virtual_machine
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <jitana/jitana.hpp>

BOOST_AUTO_TEST_CASE(first_test)
{
    BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(second_test)
{
    BOOST_TEST(1 + 1 == 2);
}
