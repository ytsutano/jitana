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

#ifndef JITANA_RESOURCE_SHARING_GRAPH_HPP
#define JITANA_RESOURCE_SHARING_GRAPH_HPP

#include "jitana/jitana.hpp"

#include <unordered_map>

namespace jitana {
    namespace detail {
        using rsg_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A resource sharing graph vertex descriptor.
    using rsg_vertex_descriptor = detail::rsg_traits::vertex_descriptor;

    /// A resource sharing graph vertex property.
    struct rsg_vertex_property {
        class_loader_hdl hdl;
    };

    /// A resource sharing graph edge property.
    struct rsg_edge_property {
        enum { explicit_intent, implicit_intent } kind;
        std::string description;
    };

    /// A resource sharing graph property.
    struct rsg_property {
        std::unordered_map<class_loader_hdl, rsg_vertex_descriptor>
                hdl_to_vertex;
    };

    /// A resource sharing graph.
    using resource_sharing_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, rsg_vertex_property,
                                    rsg_edge_property, rsg_property>;

    template <typename RSG>
    inline boost::optional<rsg_vertex_descriptor>
    lookup_rsg_vertex(const class_loader_hdl& hdl, const RSG& g)
    {
        const auto& lut = g[boost::graph_bundle].hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }
}

#endif
