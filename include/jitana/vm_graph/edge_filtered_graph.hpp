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

#ifndef JITANA_EDGE_FILTERED_GRAPH_HPP
#define JITANA_EDGE_FILTERED_GRAPH_HPP

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/copy.hpp>
#include <boost/type_erasure/any_cast.hpp>

namespace jitana {
    template <typename EdgePropType, typename Graph>
    struct edge_type_pred {
        const Graph* g;

        template <typename Edge>
        bool operator()(const Edge& e) const
        {
            namespace te = boost::type_erasure;
            return te::any_cast<const EdgePropType*>(&(*g)[e]) != nullptr;
        }
    };

    template <typename EdgePropType, typename Graph>
    inline edge_type_pred<EdgePropType, Graph> make_edge_type_pred(Graph& g)
    {
        return {&g};
    }

    /// A filtered graph whose edges are filtered to EdgePropType only.
    template <typename EdgePropType, typename Graph>
    using edge_filtered_graph
            = boost::filtered_graph<Graph, edge_type_pred<EdgePropType, Graph>>;

    /// Makes an edge_filtered_graph.
    template <typename EdgePropType, typename Graph>
    inline edge_filtered_graph<EdgePropType, Graph>
    make_edge_filtered_graph(Graph& g)
    {
        return {g, make_edge_type_pred<EdgePropType>(g)};
    }

    template <typename EdgePropType, typename Graph>
    inline Graph edge_filtered_copy(const Graph& g)
    {
        Graph dst;
        const auto& fg = make_edge_filtered_graph<EdgePropType>(g);

        // Create a vertex copier. This shouldn't be necessary, but the BGL
        // still doesn't support bundled properties.
        auto v_copier
                = [&](const auto& v1, const auto& v2) { dst[v1] = fg[v2]; };

        // Copy the graph.
        boost::copy_graph(fg, dst, boost::vertex_copy(v_copier));

        // We also need to copy the graph bundle.
        dst[boost::graph_bundle] = g[boost::graph_bundle];

        return dst;
    }
}

#endif
