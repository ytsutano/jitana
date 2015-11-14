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

#ifndef JITANA_MONOTONIC_DATAFLOW_HPP
#define JITANA_MONOTONIC_DATAFLOW_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>

#include <vector>
#include <queue>

namespace jitana {
    template <typename CFG, typename SetMap, typename CombOp, typename FlowFunc>
    void monotonic_dataflow(const CFG& g, SetMap& inset_map, SetMap& outset_map,
                            CombOp comb_op, FlowFunc flow_func)
    {
        using vertex_descriptor =
                typename boost::graph_traits<CFG>::vertex_descriptor;

        // Compute the initial outset.
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            flow_func(v, inset_map[v], outset_map[v]);
        }

        std::queue<vertex_descriptor> worklist;
        std::vector<bool> dirty(num_vertices(g), true);

        // Fill the worklist with the reverse postorder from the control flow
        // graph.
        const auto& rg = boost::make_reverse_graph(g);
        using RCFG = decltype(rg);
        struct rev_postorder_maker : public boost::default_dfs_visitor {
            std::queue<vertex_descriptor>& worklist;

            rev_postorder_maker(std::queue<vertex_descriptor>& worklist)
                    : worklist(worklist)
            {
            }

            void finish_vertex(const vertex_descriptor& u, const RCFG&)
            {
                worklist.push(u);
            }
        } vis(worklist);
        boost::depth_first_search(rg, visitor(vis));

        // Iterate over the worklist.
        while (!worklist.empty()) {
            auto v = worklist.front();

            auto preds = in_edges(v, g);
            if (preds.first != preds.second) {
                // Compute the inset from the outset of the predecessors.
                auto new_inset = outset_map[source(*preds.first, g)];
                ++preds.first;
                for (const auto& p : boost::make_iterator_range(preds)) {
                    comb_op(new_inset, outset_map[source(p, g)]);
                }

                // Check if the inset has really changed.
                if (inset_map[v] != new_inset) {
                    // Update the inset.
                    inset_map[v] = std::move(new_inset);

                    // Compute the outset.
                    flow_func(v, inset_map[v], outset_map[v]);

                    // Add the successors to the worklist.
                    auto succs = adjacent_vertices(v, g);
                    for (const auto& s : boost::make_iterator_range(succs)) {
                        if (!dirty[s]) {
                            worklist.push(s);
                            dirty[s] = true;
                        }
                    }
                }
            }

            worklist.pop();
            dirty[v] = false;
        }
    }
}

#endif
