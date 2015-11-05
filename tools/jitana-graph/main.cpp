#include <iostream>
#include <vector>
#include <regex>

#include <boost/graph/graphviz.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/range/iterator_range.hpp>

#include <jitana/jitana.hpp>
#include <jitana/vm_core/dex_file.hpp>
#include <jitana/vm_graph/graphviz.hpp>
#include <jitana/vm_graph/edge_filtered_graph.hpp>
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
                      add_data_flow_edges(vm, vm.methods()[v].insns);
                  });

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
            write_graphviz_insn_graph(ofs, ig);
        }
    }
}

int main()
{
    test_virtual_machine();
}
