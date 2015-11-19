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

#define BOOST_TEST_MODULE test_points_to
#define BOOST_TEST_INCLUDED
#include <boost/test/unit_test.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>
#include <jitana/analysis/points_to.hpp>

BOOST_AUTO_TEST_CASE(points_to)
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

    {
        const auto& filenames = {"../../dex/small_tests/03/03.dex"};
        jitana::class_loader loader(33, "SmallTest02", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    jitana::jvm_method_hdl mh = {{22, "LTest;"}, "main([Ljava/lang/String;)V"};

    jitana::pointer_assignment_graph pag;
    jitana::contextual_call_graph cg;

    if (auto mv = vm.find_method(mh, true)) {
        vm.load_recursive(*mv);

        // Compute the call graph.
        jitana::add_call_graph_edges(vm);

        // Compute the data-flow.
        std::for_each(vertices(vm.methods()).first,
                      vertices(vm.methods()).second,
                      [&](const jitana::method_vertex_descriptor& v) {
                          add_data_flow_edges(vm.methods()[v].insns);
                      });

        std::cout << "Making pointer assignment graph for " << mh << "...";
        std::cout << std::endl;

        // Apply points-to analysis.
        jitana::update_points_to_graphs(pag, cg, vm, *mv);
    }
    else {
        throw std::runtime_error("failed to find the method");
    }

    BOOST_CHECK(num_vertices(pag) == 51108);
    BOOST_CHECK(num_edges(pag) == 141599);
}
