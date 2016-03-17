/*
 * Copyright (c) 2016 Shakthi Bachala
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
// rsg
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <jitana/analysis/resource_sharing.hpp>
#include <algorithm>
#include <string>
#include <fstream>
static std::vector<uint8_t> source;
static std::vector<uint8_t> sink;
static std::vector<std::pair<jitana::r_hdl, jitana::r_hdl>> so_si;
static std::vector<std::pair<int, std::string>> all_sink_intents;
static std::vector<std::pair<int, std::string>> all_source_intents;
static std::vector<std::pair<int, std::string>> class_loader_name_pair;

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
        jitana::class_loader loader(0, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    const std::string aut_details = "extracted/location.txt";

    std::pair<std::string, std::string> apk_name_location;
    std::vector<std::pair<std::string, std::string>> all_apk_details;
    std::pair<int, std::string> loader_name_pair;
    std::ifstream log(aut_details);
    std::ifstream log_in_stream;
    std::string log_line;
    std::pair<int, std::string> clnp;
    while (std::getline(log, log_line)) {
        log_in_stream >> log_line;
        std::string loc, n;
        std::size_t pos_space = log_line.find(" ");
        loc = log_line.substr(pos_space + 1);
        n = log_line.substr(0, pos_space);
        apk_name_location = std::make_pair(n, loc);
        all_apk_details.push_back(apk_name_location);
    }
    log_in_stream.close();
    int loader_class_value = 1;
    for (auto& name_location : all_apk_details) {
        int keyy = loader_class_value;
        std::string app_name = std::get<0>(name_location);
        jitana::app_names.push_back(app_name);
        std::string location_app = std::get<1>(name_location);
        std::string location_manifest = location_app;
        location_manifest.append(app_name);
        location_manifest.append(".manifest.xml");
        jitana::parse_manifest_load_intent(keyy, location_manifest,
                                           all_sink_intents);
        loader_name_pair = std::make_pair(loader_class_value,
                                          std::get<0>(name_location));
        location_app.append("classes.dex");
        clnp = std::make_pair(loader_class_value, std::get<0>(name_location));
        class_loader_name_pair.push_back(clnp);
        {
            const auto& filenames = {location_app};

            jitana::class_loader loader(loader_class_value,
                                        std::get<0>(name_location),
                                        begin(filenames), end(filenames));
            vm.add_loader(loader, 0);
        }
        vm.load_all_classes(loader_class_value);
        std::cout << "Loaded Successfully: " << loader_class_value << " "
                  << std::get<0>(name_location) << "\n";
        loader_class_value++;
    }

    jitana::add_call_graph_edges(vm);
    std::cout << "Call Graph SUCCESS"
              << "\n";
    std::for_each(vertices(vm.methods()).first, vertices(vm.methods()).second,
                  [&](const jitana::method_vertex_descriptor& v) {
                      add_data_flow_edges(vm.methods()[v].insns);
                  });
    std::cout << "DFA  SUCCESS"
              << "\n";

    jitana::add_resource_graph_edges_implicit(vm, all_source_intents,
                                              all_sink_intents);
    jitana::add_resource_graph_edges_explicit(vm, so_si);
    std::cout << "RSG  SUCCESS"
              << "\n";

    // std::cout << "Source : " << unsigned(source) << "  ; Sink: " <<
    // unsigned(sink) << "\n";
    std::cout << "# of Loaders: " << num_vertices(vm.loaders()) << "\n";
    std::cout << "# of classes: " << num_vertices(vm.classes()) << "\n";
    std::cout << "# of methods: " << num_vertices(vm.methods()) << "\n";
    std::cout << "# of fields:  " << num_vertices(vm.fields()) << "\n";
    std::cout << "Writing graphs..." << std::endl;
    write_graphs(vm);
}

void write_graphs(const jitana::virtual_machine& vm)
{
    {
        std::ofstream ofs("output/resource_sharing_graph.dot");

        boost::adjacency_list<> g;
        jitana::write_graphviz_resource_sharing_graph_implicit(
                ofs, all_source_intents, all_sink_intents,
                class_loader_name_pair, g);
        jitana::write_graphviz_resource_sharing_graph_explicit(
                ofs, so_si, class_loader_name_pair, g);
    }
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
        }
    }
}

int main()
{
    test_virtual_machine();
}
