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

#include <iostream>
#include <vector>
#include <regex>

#include <boost/graph/graphviz.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/range/iterator_range.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>

void write_graphs(const jitana::virtual_machine& vm);

void test_virtual_machine()
{
    jitana::virtual_machine vm;

    {
        const auto& filenames = {"../dex/framework/core.dex",
                                 "../dex/framework/framework.dex",
                                 "../dex/framework/framework2.dex",
                                 "../dex/framework/ext.dex",
                                 "../dex/framework/conscrypt.dex",
                                 "../dex/framework/okhttp.dex",
                                 "../dex/framework/core-junit.dex",
                                 "../dex/framework/android.test.runner.dex",
                                 "../dex/framework/android.policy.dex"};
        jitana::class_loader loader(11, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    {
        const auto& filenames = {"../dex/app/instagram_classes.dex"};
        jitana::class_loader loader(22, "Instagram", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../dex/app/test.dex"};
        jitana::class_loader loader(33, "Test", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../dex/app/super_depth_classes.dex"};
        jitana::class_loader loader(44, "SuperDepth", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../dex/small_tests/01/01.dex"};
        jitana::class_loader loader(55, "SmallTest01", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../dex/small_tests/02/02.dex"};
        jitana::class_loader loader(66, "SmallTest02", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    vm.load_all_classes(66);
    // vm.find_class({33, "LTest;"}, true);
    vm.find_class({22, "Lcom/fasterxml/jackson/core/base/ParserMinimalBase;"},
                  true);

    // Compute the call graph.
    jitana::add_call_graph_edges(vm);

    // Compute the data-flow.
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      add_data_flow_edges(vm.methods()[v].insns);
                  });

    std::cout << "# of classes: " << num_vertices(vm.classes()) << "\n";
    std::cout << "# of methods: " << num_vertices(vm.methods()) << "\n";
    std::cout << "# of fields:  " << num_vertices(vm.fields()) << "\n";

    std::cout << "Writing graphs..." << std::endl;
    write_graphs(vm);
}

void write_graphs(const jitana::virtual_machine& vm)
{
    {
        std::ofstream ofs("output/loader_graph.dot");
        write_graphviz_loader_graph(ofs, vm.loaders());
    }

    {
        std::ofstream ofs("output/class_graph.dot");
        write_graphviz_class_graph(ofs, vm.classes());
    }

    {
        std::ofstream ofs("output/method_graph.dot");
        write_graphviz_method_graph(ofs, vm.methods());
    }

    {
        std::ofstream ofs("output/field_graph.dot");
        write_graphviz_field_graph(ofs, vm.fields());
    }

    for (const auto& v : boost::make_iterator_range(vertices(vm.methods()))) {
        const auto& ig = vm.methods()[v].insns;
        if (num_vertices(ig) > 0) {
            std::stringstream ss;
            ss << "output/insn/" << vm.methods()[v].hdl << ".dot";
            std::ofstream ofs(ss.str());
            write_graphviz_insn_graph(ofs, ig, &vm);
        }
    }
}

int main()
{
    test_virtual_machine();
}
