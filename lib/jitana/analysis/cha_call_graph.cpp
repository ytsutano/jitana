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

#include "jitana/analysis/cha_call_graph.hpp"

#include <queue>

#include <boost/range/iterator_range.hpp>

using namespace jitana;

contextual_call_graph jitana::make_cha_call_graph(
        virtual_machine& vm,
        const std::vector<method_vertex_descriptor>& entry_points)
{
    contextual_call_graph ccg;

    const auto& mg = vm.methods();
    auto inheritance_mg
            = make_edge_filtered_graph<method_super_edge_property>(mg);

    // Copy the entry points.
    {
        auto& ep_vec = ccg[boost::graph_bundle].entry_points;
        for (const auto& mv : entry_points) {
            ccg[boost::graph_bundle].entry_points.push_back(mg[mv].hdl);
        }
        unique_sort(ep_vec);
    }

    std::vector<bool> visited(num_vertices(mg), false);

    std::queue<method_vertex_descriptor> worklist;
    for (const auto& mv : entry_points) {
        worklist.push(mv);
        visited[mv] = true;
    }

    while (!worklist.empty()) {
        auto mv = worklist.front();
        worklist.pop();

        std::cout << "-------------------------------------\n";
        std::cout << mg[mv].jvm_hdl << "\n";

        const auto& ig = mg[mv].insns;

        for (const auto& iv : boost::make_iterator_range(vertices(ig))) {
            const auto* invoke_insn = get<insn_invoke>(&ig[iv].insn);
            if (!invoke_insn) {
                continue;
            }

            ccg_edge_property eprop;
            eprop.virtual_call = info(op(*invoke_insn)).can_virtually_invoke();
            eprop.caller_insn_vertex = iv;

            std::cout << *invoke_insn << "\n";
            if (auto const_mv = vm.find_method(invoke_insn->const_val, true)) {
                boost::vector_property_map<int> color_map(
                        static_cast<unsigned>(num_vertices(inheritance_mg)));
                auto f = [&](method_vertex_descriptor target_mv,
                             const decltype(inheritance_mg)&) {
                    add_edge(mg[mv].hdl, mg[target_mv].hdl, eprop, ccg);
                    if (!visited[target_mv]) {
                        worklist.push(target_mv);
                        visited[target_mv] = true;
                    }
                    return false;
                };
                boost::depth_first_visit(inheritance_mg, *const_mv,
                                         boost::default_dfs_visitor{},
                                         color_map, f);
            }
        }
    }

    return ccg;
}
