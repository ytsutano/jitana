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
#include <algorithm>

#include <boost/graph/graphviz.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/range/iterator_range.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>
#include <jitana/analysis/points_to.hpp>

struct benchmark_data {
    std::string loader_name;
    long n_classes;
    long n_methods;
    long n_fields;
    long n_dex_insns;
    double t_loading = std::numeric_limits<double>::max();
    double t_call_graph = std::numeric_limits<double>::max();
    double t_data_flow = std::numeric_limits<double>::max();
};

int setup_class_loaders(jitana::virtual_machine& vm)
{
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
        jitana::class_loader loader(0, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    int loader_idx = 1;

    auto add_loaders = [&](std::string name,
                           std::initializer_list<std::string> filenames) {
        jitana::class_loader loader(loader_idx, name, begin(filenames),
                                    end(filenames));
        vm.add_loader(loader, 0);
        ++loader_idx;
    };

#if 0
    add_loaders("SmallTest01", {"../../../dex/small_tests/01/01.dex"});
    add_loaders("SmallTest02", {"../../../dex/small_tests/02/02.dex"});
    add_loaders("SmallTest03", {"../../../dex/small_tests/03/03.dex"});
    add_loaders("Test", {"../../../dex/app/test.dex"});
#endif
    add_loaders("SuperDepth", {"../../../dex/app/super_depth_classes.dex"});
    add_loaders("GoogleEarth", {"../../../dex/app/google_earth_classes.dex"});
    add_loaders("Twitter", {"../../../dex/app/twitter_classes.dex"});
    add_loaders("Instagram", {"../../../dex/app/instagram_classes.dex"});
    add_loaders(
            "Facebook",
            {
                    "../../../dex/facebook/facebook_classes.dex",
                    "../../../dex/facebook/app_secondary_program_dex/"
                    "program-7feaf7c75a5305b1083a160f14eade3949051d0eb6b0baa6."
                    "dex",
                    "../../../dex/facebook/app_secondary_program_dex/"
                    "program-e2ca9fdbaa4e32f97b90b37618078b878d3e0cfab6b0baa6."
                    "dex",
                    "../../../dex/facebook/app_secondary_program_dex/"
                    "program-eb5202dbb54c0efff1c01c5faa45c485de84abf7b6b0baa6."
                    "dex",
            });

    return loader_idx;
}

void run_benchmarks(benchmark_data& bd, jitana::virtual_machine vm,
                    jitana::class_loader_hdl lh)
{
    const auto& lg = vm.loaders();
    bd.loader_name = lg[*find_loader_vertex(lh, lg)].loader.name();

    // Load all classes.
    {
        auto start = std::chrono::system_clock::now();

        vm.load_all_classes(lh);

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start);
        bd.t_loading = std::min(bd.t_loading, duration.count() / 1000.0);
    }

    // Compute the call graph.
    {
        auto start = std::chrono::system_clock::now();

        jitana::add_call_graph_edges(vm);

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start);
        bd.t_call_graph = std::min(bd.t_call_graph, duration.count() / 1000.0);
    }

    // Compute the data-flow.
    {
        auto start = std::chrono::system_clock::now();

        std::for_each(vertices(vm.methods()).first,
                      vertices(vm.methods()).second,
                      [&](const jitana::method_vertex_descriptor& v) {
                          add_data_flow_edges(vm.methods()[v].insns);
                      });

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start);
        bd.t_data_flow = std::min(bd.t_data_flow, duration.count() / 1000.0);
    }

    bd.n_dex_insns = 0;
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      int n = num_vertices(vm.methods()[v].insns);
                      // We should have 2 pseudo instructions if the insn graph
                      // is non-empty.
                      if (n >= 2) {
                          bd.n_dex_insns += n - 2;
                      }
                  });

    bd.n_classes = num_vertices(vm.classes());
    bd.n_methods = num_vertices(vm.methods());
    bd.n_fields = num_vertices(vm.fields());
}

void run_benchmark()
{
    jitana::virtual_machine vm;
    int n_loaders = setup_class_loaders(vm);

    std::cout << "Name";
    std::cout << ",# of Classes";
    std::cout << ",# of Methods";
    std::cout << ",# of Fields";
    std::cout << ",# of DEX Insns";
    std::cout << ",Loading (ms)";
    std::cout << ",Call Graph (ms)";
    std::cout << ",Data-Flow (ms)";
    std::cout << std::endl;

    {
        std::ofstream ofs("output/loader_graph.dot");
        write_graphviz_loader_graph(ofs, vm.loaders());
    }

    for (int i = 1; i < n_loaders; ++i) {
        benchmark_data bd;

        for (int j = 0; j < 5; ++j) {
            run_benchmarks(bd, vm, i);
        }

        std::cout << bd.loader_name << ",";
        std::cout << bd.n_classes << ",";
        std::cout << bd.n_methods << ",";
        std::cout << bd.n_fields << ",";
        std::cout << bd.n_dex_insns << ",";
        std::cout << bd.t_loading << ",";
        std::cout << bd.t_call_graph << ",";
        std::cout << bd.t_data_flow << std::endl;
    }
}

int main()
{
    run_benchmark();
}
