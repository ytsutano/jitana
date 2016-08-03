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

#ifndef JITANA_CONTEXTUAL_CALL_GRAPH_HPP
#define JITANA_CONTEXTUAL_CALL_GRAPH_HPP

#include "jitana/jitana.hpp"

#include <unordered_map>

namespace jitana {
    namespace detail {
        using ccg_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A contextual call graph vertex descriptor.
    using ccg_vertex_descriptor = detail::ccg_traits::vertex_descriptor;

    /// A contextual call graph vertex property.
    struct ccg_vertex_property {
        dex_method_hdl hdl;

        ccg_vertex_property()
        {
        }

        ccg_vertex_property(dex_method_hdl hdl) : hdl(hdl)
        {
        }
    };

    /// A contextual call graph edge property.
    struct ccg_edge_property {
        bool virtual_call;
        insn_vertex_descriptor caller_insn_vertex;
    };

    /// A contextual call graph property.
    struct ccg_property {
        std::vector<dex_method_hdl> entry_points;
    };
}

namespace boost {
    namespace graph {
        template <>
        struct internal_vertex_name<jitana::ccg_vertex_property> {
            using type = multi_index::member<jitana::ccg_vertex_property,
                                             jitana::dex_method_hdl,
                                             &jitana::ccg_vertex_property::hdl>;
        };

        template <>
        struct internal_vertex_constructor<jitana::ccg_vertex_property> {
            using type = vertex_from_name<jitana::ccg_vertex_property>;
        };
    }
}

namespace jitana {
    /// A contextual call graph.
    using contextual_call_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, ccg_vertex_property,
                                    ccg_edge_property, ccg_property>;

    template <typename CCG>
    inline void
    write_graphviz_contextual_call_graph(std::ostream& os, const CCG& g,
                                         virtual_machine* vm = nullptr)
    {
        auto vprop_writer = [&](std::ostream& os, const auto& v) {
            os << "[";
            if (vm) {
                const auto& mg = vm->methods();
                auto mv = vm->find_method(g[v].hdl, false);
                if (mv) {
                    os << "label=\"" << mg[*mv].jvm_hdl << "\",";
                }
            }
            else {
                os << "label=\"" << g[v].hdl << "\",";
            }
            os << "URL=\"insn/" << g[v].hdl << ".dot\",";
            os << "shape=record";
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            os << "color=red, label=";
            os << (g[e].virtual_call ? "virtual" : "direct");
            os << ", taillabel=" << g[e].caller_insn_vertex;
            os << "]";
        };

        write_graphviz(os, g, vprop_writer, eprop_writer);
    }
}

#endif
