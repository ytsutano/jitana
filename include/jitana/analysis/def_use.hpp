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

#ifndef JITANA_DEF_USE_HPP
#define JITANA_DEF_USE_HPP

#include "jitana/vm_core/virtual_machine.hpp"
#include "jitana/vm_core/insn_info.hpp"
#include "jitana/vm_graph/edge_filtered_graph.hpp"
#include "jitana/algorithm/monotonic_dataflow.hpp"

#include <algorithm>
#include <vector>

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/properties.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/range/iterator_range.hpp>

namespace jitana {
    struct insn_def_use_edge_property {
        register_idx reg;
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const insn_def_use_edge_property& prop)
    {
        os << "color=red, fontcolor=red";
        os << ", label=\"" << prop.reg << "\"";
    }

    inline void add_def_use_edges(insn_graph& g)
    {
        if (num_vertices(g) == 0) {
            return;
        }

        // Construct a list of defs and uses for each instruction.
        std::vector<std::vector<register_idx>> defs_map(num_vertices(g));
        std::vector<std::vector<register_idx>> uses_map(num_vertices(g));
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            defs_map[v] = defs(g[v].insn);
            uses_map[v] = uses(g[v].insn);
        }

        using elem = std::pair<insn_vertex_descriptor, register_idx>;
        using set = std::vector<elem>;
        auto flow_func
                = [&](insn_vertex_descriptor v, const set& inset, set& outset) {
                      // Propagate.
                      outset = inset;

                      // Kill.
                      for (const auto& x : defs_map[v]) {
                          auto e = std::remove_if(
                                  begin(outset), end(outset),
                                  [&](const elem& f) { return f.second == x; });
                          outset.erase(e, end(outset));
                      }

                      // Gen.
                      for (const auto& x : defs_map[v]) {
                          outset.emplace_back(v, x);
                      }
                      unique_sort(outset);
                  };
        auto comb_op = [](set& x, const set& y) {
            // Apply in-place union.
            set temp;
            std::set_union(begin(x), end(x), begin(y), end(y),
                           std::back_inserter(temp));
            x = temp;
        };
        auto cfg = make_edge_filtered_graph<insn_control_flow_edge_property>(g);
        std::vector<set> inset_map(num_vertices(g));
        std::vector<set> outset_map(num_vertices(g));
        monotonic_dataflow(cfg, inset_map, outset_map, comb_op, flow_func);

        // Update the graph with the data flow information.
        remove_edge_if(make_edge_type_pred<insn_def_use_edge_property>(g), g);
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            for (const auto& e : inset_map[v]) {
                if (e.first != v
                    && std::binary_search(begin(uses_map[v]), end(uses_map[v]),
                                          e.second)) {
                    insn_def_use_edge_property edge_prop;
                    edge_prop.reg = e.second;
                    add_edge(e.first, v, edge_prop, g);
                }
            }
        }
    }
}

#endif
