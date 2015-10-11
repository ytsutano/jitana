#ifndef JITANA_DATA_FLOW_HPP
#define JITANA_DATA_FLOW_HPP

#include "jitana/core/virtual_machine.hpp"
#include "jitana/core/insn_info.hpp"
#include "jitana/core/variable.hpp"
#include "jitana/graph/edge_filtered_graph.hpp"
#include "jitana/algorithm/monotonic_dataflow.hpp"

#include <algorithm>
#include <vector>

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/properties.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/range/iterator_range.hpp>

namespace jitana {
    struct insn_data_flow_edge_property {
        variable var;
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const insn_data_flow_edge_property& prop)
    {
        os << "color=red, fontcolor=red";
        os << ", label=\"" << prop.var << "\"";
    }

    inline void add_data_flow_edges(virtual_machine& vm, insn_graph& g)
    {
        if (num_vertices(g) == 0) {
            return;
        }

        // Construct a list of defs and uses for each instruction.
        std::vector<std::vector<variable>> defs_map(num_vertices(g));
        std::vector<std::vector<variable>> uses_map(num_vertices(g));
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            defs_map[v] = defs(g[v].insn);
            uses_map[v] = uses(g[v].insn);
        }

        // Compute defs for entry instruction.
        {
            auto& entry_defs = defs_map[0];
            entry_defs.clear();

            // Put all the uses in the method to the defs.
            for (const auto& v : boost::make_iterator_range(vertices(g))) {
                entry_defs.insert(end(entry_defs), begin(uses_map[v]),
                                  end(uses_map[v]));
            }
            unique_sort(entry_defs);
        }

        // Compute uses for exit instruction.
        {
            auto& exit_uses = uses_map[num_vertices(g) - 1];
            exit_uses.clear();

            // Put all the defs in the method to the uses.
            for (const auto& v : boost::make_iterator_range(vertices(g))) {
                exit_uses.insert(end(exit_uses), begin(defs_map[v]),
                                 end(defs_map[v]));
            }
            unique_sort(exit_uses);

            // Remove registers from uses of the exit instruction.
            auto e = remove_if(begin(exit_uses), end(exit_uses),
                               [&](const variable& v) {
                                   return v.kind() == variable::kind_register;
                               });
            exit_uses.erase(e, end(exit_uses));

            // Register vR may be used by the exit instruction if the return
            // type is not void.
            if (g[boost::graph_bundle].jvm_hdl.unique_name.back() != 'V') {
                auto reg_vr = make_register_variable(register_idx::idx_result);
                exit_uses.push_back(reg_vr);
                unique_sort(exit_uses);
            }
        }

        using elem = std::pair<insn_vertex_descriptor, variable>;
        using set = std::vector<elem>;
        auto flow_func =
                [&](insn_vertex_descriptor v, const set& inset, set& outset) {
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
        remove_edge_if(make_edge_type_pred<insn_data_flow_edge_property>(g), g);
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            for (const auto& e : inset_map[v]) {
                if (e.first != v
                    && std::binary_search(begin(uses_map[v]), end(uses_map[v]),
                                          e.second)) {
                    insn_data_flow_edge_property edge_prop;
                    edge_prop.var = e.second;
                    add_edge(e.first, v, edge_prop, g);
                }
            }
        }
    }
}

#endif
