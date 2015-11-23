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

#define BOOST_TEST_MODULE test_class_graph
#define BOOST_TEST_INCLUDED
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

    auto o_v = vm.find_class({11, "Ljava/lang/Object;"}, true);
    auto a_v = vm.find_class({22, "LA;"}, true);
    auto b_v = vm.find_class({22, "LB;"}, true);
    auto x_v = vm.find_class({22, "LX;"}, true);

    // Make sure that all classes are found.
    BOOST_CHECK(!!o_v);
    BOOST_CHECK(!!a_v);
    BOOST_CHECK(!!b_v);
    BOOST_CHECK(!!x_v);

    const auto& cg = vm.classes();

    BOOST_CHECK(is_superclass_of(*a_v, *a_v, cg));

    BOOST_CHECK(is_superclass_of(*o_v, *a_v, cg));
    BOOST_CHECK(is_superclass_of(*a_v, *b_v, cg));
    BOOST_CHECK(is_superclass_of(*o_v, *b_v, cg));

    BOOST_CHECK(!is_superclass_of(*a_v, *o_v, cg));
    BOOST_CHECK(!is_superclass_of(*b_v, *a_v, cg));
    BOOST_CHECK(!is_superclass_of(*b_v, *o_v, cg));

    BOOST_CHECK(!is_superclass_of(*a_v, *x_v, cg));
    BOOST_CHECK(!is_superclass_of(*x_v, *a_v, cg));
}
