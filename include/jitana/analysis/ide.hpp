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

#ifndef JITANA_IDE_HPP
#define JITANA_IDE_HPP

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
        dex_method_hdl hdl;
    };

    /// A labeled exploded super graph edge property.
    struct lesg_edge_property {
    };

    /// A labeled exploded super graph property.
    struct lesg_property {
        std::unordered_map<dex_method_hdl, lesg_vertex_descriptor>
                hdl_to_vertex;
    };

    /// A labeled exploded super graph.
    using labeled_exploded_super_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, lesg_vertex_property,
                                    lesg_edge_property, lesg_property>;

    template <typename LESG>
    inline boost::optional<lesg_vertex_descriptor>
    lookup_lesg_vertex(const dex_method_hdl& hdl, const LESG& g)
    {
        const auto& lut = g[boost::graph_bundle].hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }
}

namespace jitana {
}

#endif
