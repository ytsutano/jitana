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
    };

    /// A contextual call graph edge property.
    struct ccg_edge_property {
    };

    /// A contextual call graph property.
    struct ccg_property {
        std::unordered_map<dex_method_hdl, ccg_vertex_descriptor> hdl_to_vertex;
    };

    /// A contextual call graph.
    using contextual_call_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, ccg_vertex_property,
                                    ccg_edge_property, ccg_property>;

    template <typename CCG>
    inline boost::optional<ccg_vertex_descriptor>
    lookup_ccg_vertex(const dex_method_hdl& hdl, const CCG& g)
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
