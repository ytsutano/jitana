/*
 * Copyright (c) 2016, Shakthi Bachala
 * Copyright (c) 2016, Yutaka Tsutano
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
#include <algorithm>
#include <string>
#include <fstream>

#include <boost/graph/graphviz.hpp>

#include <jitana/jitana.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>
#include <jitana/analysis/resource_sharing.hpp>

static std::vector<std::pair<jitana::r_hdl, jitana::r_hdl>> so_si;
static std::vector<std::pair<jitana::class_loader_hdl, std::string>>
        all_sink_intents;
static std::vector<std::pair<jitana::class_loader_hdl, std::string>>
        all_source_intents;

void write_graphs(const jitana::virtual_machine& vm);

void run_iac_analysis()
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
        jitana::class_loader loader(0, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    int loader_idx = 1;
    std::ifstream location_ifs("extracted/location.txt");
    std::string name;
    while (std::getline(location_ifs, name)) {
        jitana::app_names.push_back(name);

        std::string dir = "extracted/" + name + "/";
        std::string manifest = dir + name + ".manifest.xml";
        jitana::parse_manifest_load_intent(loader_idx, manifest,
                                           all_sink_intents);

        {
            const auto& filenames = {dir + "classes.dex"};
            jitana::class_loader loader(loader_idx, name, begin(filenames),
                                        end(filenames));
            vm.add_loader(loader, 0);
        }

        vm.load_all_classes(loader_idx);
        std::cout << "Loaded Successfully: " << loader_idx << " " << name
                  << "\n";

        ++loader_idx;
    }

    // Compute the call graph.
    jitana::add_call_graph_edges(vm);

    // Compute the data-flow.
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      add_data_flow_edges(vm.methods()[v].insns);
                  });

    // Create a resource sharing graph (kind of).
    jitana::add_resource_graph_edges_implicit(vm, all_source_intents);
    jitana::add_resource_graph_edges_explicit(vm, so_si);

    std::cout << "Writing graphs..." << std::endl;
    write_graphs(vm);

    std::cout << "# of Loaders: " << num_vertices(vm.loaders()) << "\n";
    std::cout << "# of classes: " << num_vertices(vm.classes()) << "\n";
    std::cout << "# of methods: " << num_vertices(vm.methods()) << "\n";
    std::cout << "# of fields:  " << num_vertices(vm.fields()) << "\n";
}

void write_graphs(const jitana::virtual_machine& vm)
{
    {
        std::ofstream ofs("output/resource_sharing_graph.dot");

        jitana::resource_sharing_graph g;
        jitana::write_graphviz_resource_sharing_graph_implicit(
                all_source_intents, all_sink_intents, g);
        jitana::write_graphviz_resource_sharing_graph_explicit(ofs, so_si, g);
    }

    {
        std::ofstream ofs("output/loader_graph.dot");
        write_graphviz_loader_graph(ofs, vm.loaders());
    }

    {
        std::ofstream ofs("output/class_graph.dot");
        write_graphviz_class_graph(ofs, vm.classes());
    }
}

int main()
{
    run_iac_analysis();
}
