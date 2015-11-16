#define BOOST_TEST_MODULE test_class_graph
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <jitana/jitana.hpp>

BOOST_AUTO_TEST_CASE(predicates)
{
    jitana::virtual_machine vm;

    {
        const auto& filenames = {"../../dex/framework/core.dex",
                                 "../../dex/framework/framework.dex",
                                 "../../dex/framework/framework2.dex",
                                 "../../dex/framework/ext.dex",
                                 "../../dex/framework/conscrypt.dex",
                                 "../../dex/framework/okhttp.dex",
                                 "../../dex/framework/core-junit.dex",
                                 "../../dex/framework/android.test.runner.dex",
                                 "../../dex/framework/android.policy.dex"};
        jitana::class_loader loader(11, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    {
        const auto& filenames = {"../../dex/small_tests/02/02.dex"};
        jitana::class_loader loader(22, "SmallTest02", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    auto o_v = *vm.find_class({11, "Ljava/lang/Object;"}, true);
    auto a_v = *vm.find_class({22, "LA;"}, true);
    auto b_v = *vm.find_class({22, "LB;"}, true);
    auto x_v = *vm.find_class({22, "LX;"}, true);

    BOOST_TEST(is_superclass_of(a_v, a_v, vm.classes()));

    BOOST_TEST(is_superclass_of(o_v, a_v, vm.classes()));
    BOOST_TEST(is_superclass_of(a_v, b_v, vm.classes()));
    BOOST_TEST(is_superclass_of(o_v, b_v, vm.classes()));

    BOOST_TEST(!is_superclass_of(a_v, o_v, vm.classes()));
    BOOST_TEST(!is_superclass_of(b_v, a_v, vm.classes()));
    BOOST_TEST(!is_superclass_of(b_v, o_v, vm.classes()));

    BOOST_TEST(!is_superclass_of(a_v, x_v, vm.classes()));
    BOOST_TEST(!is_superclass_of(a_v, x_v, vm.classes()));
}
