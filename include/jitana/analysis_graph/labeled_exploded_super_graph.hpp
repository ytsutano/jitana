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

#ifndef JITANA_LABELED_EXPLODED_SUPER_GRAPH_HPP
#define JITANA_LABELED_EXPLODED_SUPER_GRAPH_HPP

#include "jitana/jitana.hpp"

namespace jitana {
    namespace detail {
        using lesg_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A labeled exploded super graph vertex descriptor.
    using lesg_vertex_descriptor = detail::lesg_traits::vertex_descriptor;

    /// A labeled exploded super graph vertex property.
    struct lesg_vertex_property {
        dex_reg_hdl hdl;
        bool return_vertex;
    };

    /// A labeled exploded super graph edge property.
    struct lesg_edge_property {
        enum {
            kind_normal,
            kind_call,
            kind_return,
            kind_call_to_return,
            kind_register_chain
        } kind;
    };

    /// A labeled exploded super graph property.
    struct lesg_property {
    };

    /// A labeled exploded super graph.
    using labeled_exploded_super_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, lesg_vertex_property,
                                    lesg_edge_property, lesg_property>;
}

namespace boost {
    namespace graph {
        template <>
        struct internal_vertex_name<jitana::lesg_vertex_property> {
            using type
                    = multi_index::member<jitana::lesg_vertex_property,
                                          jitana::dex_reg_hdl,
                                          &jitana::lesg_vertex_property::hdl>;
        };

        template <>
        struct internal_vertex_constructor<jitana::lesg_vertex_property> {
            using type = vertex_from_name<jitana::lesg_vertex_property>;
        };
    }
}

namespace jitana {
    template <typename LESG>
    inline void write_graphviz_labeled_exploded_super_graph(std::ostream& os,
                                                            const LESG& g)
    {
        auto vprop_writer = [&](std::ostream& os, const auto& v) {
            os << "[";
            os << "label=\"{" << g[v].hdl.insn_hdl.method_hdl << "|{"
               << g[v].hdl.insn_hdl.idx << "|";
            if (g[v].hdl.idx != -1) {
                os << register_idx(g[v].hdl.idx);
            }
            os << "}}\",";
            if (g[v].return_vertex) {
                os << "color=gray;";
            }
            os << "URL=\"insn/" << g[v].hdl.insn_hdl.method_hdl << ".dot\",";
            os << "shape=record";
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            switch (g[e].kind) {
            case lesg_edge_property::kind_normal:
                os << "color=black, weight=10";
                break;
            case lesg_edge_property::kind_call:
                os << "color=red";
                break;
            case lesg_edge_property::kind_return:
                os << "color=orange";
                break;
            case lesg_edge_property::kind_call_to_return:
                os << "color=gray, weight=2";
                break;
            case lesg_edge_property::kind_register_chain:
                os << "weight=1000, rank=same";
                break;
            }
            os << "]";
        };

        write_graphviz(os, g, vprop_writer, eprop_writer);
    }
}

#endif
