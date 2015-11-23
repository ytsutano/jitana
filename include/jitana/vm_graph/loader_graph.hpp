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

#ifndef JITANA_LOADER_GRAPH_HPP
#define JITANA_LOADER_GRAPH_HPP

#include "jitana/vm_core/class_loader.hpp"
#include "jitana/vm_graph/graph_common.hpp"

#include <iostream>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace jitana {
    namespace detail {
        using loader_graph_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A loader vertex descriptor.
    using loader_vertex_descriptor
            = detail::loader_graph_traits::vertex_descriptor;

    /// A loader edge descriptor.
    using loader_edge_descriptor = detail::loader_graph_traits::edge_descriptor;

    /// A loader graph vertex property.
    struct loader_vertex_property {
        class_loader loader;
    };

    /// A loader graph edge property.
    using loader_edge_property = any_edge_property;

    /// A loader graph property.
    struct loader_graph_property {
    };

    /// A loader graph.
    using loader_graph = boost::adjacency_list<boost::vecS, boost::vecS,
                                               boost::bidirectionalS,
                                               loader_vertex_property,
                                               loader_edge_property,
                                               loader_graph_property>;

    template <typename LoaderGraph>
    inline boost::optional<loader_vertex_descriptor>
    find_loader_vertex(const class_loader_hdl& hdl, const LoaderGraph& g)
    {
        for (const auto& v : boost::make_iterator_range(vertices(g))) {
            if (g[v].loader.hdl() == hdl) {
                return v;
            }
        }
        return boost::none;
    }

    struct loader_parent_edge_property {
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const loader_parent_edge_property&)
    {
        os << "color=blue, fontcolor=blue, label=parent";
    }
}

#endif
