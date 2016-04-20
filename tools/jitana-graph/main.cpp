/*
 * Copyright (c) 2015, 2016, Yutaka Tsutano
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
#include <chrono>

#include <boost/graph/graphviz.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/range/iterator_range.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/def_use.hpp>
#include <jitana/analysis/points_to.hpp>

void write_graphs(const jitana::virtual_machine& vm);

void test_virtual_machine()
{
    jitana::virtual_machine vm;

    {
        const auto& filenames
                = {"../../../dex/framework/core.dex",
                   "../../../dex/framework/framework.dex",
                   "../../../dex/framework/framework2.dex",
                   "../../../dex/framework/ext.dex",
                   "../../../dex/framework/conscrypt.dex",
                   "../../../dex/framework/okhttp.dex",
                   "../../../dex/framework/core-junit.dex",
                   "../../../dex/framework/android.test.runner.dex",
                   "../../../dex/framework/android.policy.dex"};
        jitana::class_loader loader(11, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    std::string apk_root = "../../../dex/extracted/";

    vm.add_apk(22, apk_root + "data@app@com.instagram.android-1.apk", 11);

    {
        const auto& filenames = {"../../../dex/app/test.dex"};
        jitana::class_loader loader(33, "Test", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    vm.add_apk(44, apk_root + "data@app@jp.bio100.android.superdepth-1.apk",
               11);

    {
        const auto& filenames = {"../../../dex/small_tests/01/01.dex"};
        jitana::class_loader loader(55, "SmallTest01", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../../../dex/small_tests/02/02.dex"};
        jitana::class_loader loader(66, "SmallTest02", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

    {
        const auto& filenames = {"../../../dex/small_tests/03/03.dex"};
        jitana::class_loader loader(77, "SmallTest03", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 11);
    }

#if 0
    vm.load_all_classes(66);
    // vm.find_class({33, "LTest;"}, true);
    vm.find_class({22, "Lcom/fasterxml/jackson/core/base/ParserMinimalBase;"},
                  true);
#else

#if 1
    jitana::jvm_method_hdl mh = {{77, "LTest;"}, "main([Ljava/lang/String;)V"};
#else
    jitana::jvm_method_hdl mh
            // = {{22, "Lcom/instagram/android/login/fragment/LoginFragment;"},
            = {{44, "Ljp/bio100/android/superdepth/SuperDepth;"},
               "onCreate(Landroid/os/Bundle;)V"};
// "onStart()V"};
// "onResume()V"};
// "onPause()V"};
// "onRestart()V"};
#endif
    if (auto mv = vm.find_method(mh, true)) {
        vm.load_recursive(*mv);

        // Compute the call graph.
        jitana::add_call_graph_edges(vm);

        // Compute the def-use edges.
        std::for_each(vertices(vm.methods()).first,
                      vertices(vm.methods()).second,
                      [&](const jitana::method_vertex_descriptor& v) {
                          add_def_use_edges(vm.methods()[v].insns);
                      });

        std::cout << "Making pointer assignment graph for " << mh << "...";
        std::cout << std::endl;

        {
            jitana::pointer_assignment_graph pag;
            jitana::contextual_call_graph ccg;

            auto start = std::chrono::system_clock::now();

            // Apply points-to analysis.
            jitana::update_points_to_graphs(pag, ccg, vm, *mv);

            auto duration
                    = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now() - start);

            std::cout << "# of pag vertices: " << num_vertices(pag) << "\n";
            std::cout << "# of pag edges: " << num_edges(pag) << "\n";
            size_t num_p2s = 0;
            for (const auto& v : boost::make_iterator_range(vertices(pag))) {
                num_p2s += pag[v].points_to_set.size();
            }
            std::cout << "# of p2s: " << num_p2s << "\n";
            std::cout << "# of p2s (per vertex): "
                      << (double(num_p2s) / num_vertices(pag)) << "\n";
            std::cout << "duration: " << duration.count() << " ms\n";

            std::cout << "Writing PAG..." << std::endl;
            {
                std::ofstream ofs("output/pag.dot");
                jitana::write_graphviz_pointer_assignment_graph(ofs, pag, &vm);
            }

            {
                std::ofstream ofs("output/ccg2.dot");
                boost::write_graphviz(ofs, ccg);
            }
        }
    }
    else {
        throw std::runtime_error("failed to find the method");
    }
#endif

    // Compute the call graph.
    jitana::add_call_graph_edges(vm);

    // Compute the def-use edges.
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      add_def_use_edges(vm.methods()[v].insns);
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
    try {
        test_virtual_machine();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        std::cerr << "Please make sure that ";
        std::cerr << "all dependencies are installed correctly, and ";
        std::cerr << "the DEX files exist.\n";
        return 1;
    }
}
