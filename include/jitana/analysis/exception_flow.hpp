/*
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

#ifndef JITANA_INTENT_FLOW_HPP
#define JITANA_INTENT_FLOW_HPP

#include "jitana/jitana.hpp"

namespace jitana {
    struct insn_exception_flow_edge_property {
    };

    inline void
    print_graphviz_attr(std::ostream& os,
                        const insn_exception_flow_edge_property& /*prop*/)
    {
        os << "color=darkgreen, fontcolor=darkgreen, weight=1";
    }

    inline void add_exception_flow_edges(virtual_machine& /*vm*/, insn_graph& g)
    {
        if (num_vertices(g) == 0) {
            return;
        }

        remove_edge_if(
                make_edge_type_pred<insn_exception_flow_edge_property>(g), g);

        auto gprop = g[boost::graph_bundle];

        for (const auto& tc : gprop.try_catches) {
            auto scan_try_block_insns = [&](auto handler_v) {
                for (auto v = tc.first; v <= tc.last; ++v) {
                    const auto& insn = g[v].insn;
                    if (info(op(insn)).can_throw()) {
                        insn_exception_flow_edge_property edge_prop;
                        add_edge(v, handler_v, edge_prop, g);
                    }
                }
            };

            for (const auto& h : tc.hdls) {
                scan_try_block_insns(h.second);
            }

            if (tc.has_catch_all) {
                scan_try_block_insns(tc.catch_all);
            }
        }

        auto exit_v = *--vertices(g).second;
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            const auto& insn = g[v].insn;
            if (info(op(insn)).can_throw()) {
                insn_exception_flow_edge_property edge_prop;
                add_edge(v, exit_v, edge_prop, g);
            }
        }
    }
}

#endif
