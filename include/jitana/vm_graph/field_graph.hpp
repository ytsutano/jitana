#ifndef JITANA_FIELD_GRAPH_HPP
#define JITANA_FIELD_GRAPH_HPP

#include "jitana/vm_graph/insn_graph.hpp"
#include "jitana/vm_graph/graph_common.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>

namespace jitana {
    namespace detail {
        using field_graph_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A field vertex descriptor.
    using field_vertex_descriptor
            = detail::field_graph_traits::vertex_descriptor;

    /// A field edge descriptor.
    using field_edge_descriptor = detail::field_graph_traits::edge_descriptor;

    /// A field graph vertex property.
    struct field_vertex_property {
        enum { static_field, instance_field } kind;
        dex_field_hdl hdl;
        jvm_field_hdl jvm_hdl;
        dex_type_hdl class_hdl;
        dex_access_flags access_flags;
        uint16_t offset;
        uint8_t size;
        char type_char;
    };

    /// A field graph edge property.
    using field_edge_property = any_edge_property;

    /// A field graph property.
    struct field_graph_property {
        std::unordered_map<jvm_field_hdl, field_vertex_descriptor>
                jvm_hdl_to_vertex;

        std::unordered_map<dex_field_hdl, field_vertex_descriptor>
                hdl_to_vertex;
    };

    /// A field graph.
    using field_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS,
                                    field_vertex_property, field_edge_property,
                                    field_graph_property>;

    template <typename MethodGraph>
    inline boost::optional<field_vertex_descriptor>
    lookup_field_vertex(const dex_field_hdl& hdl, const MethodGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }

    template <typename MethodGraph>
    inline boost::optional<field_vertex_descriptor>
    lookup_field_vertex(const jvm_field_hdl& hdl, const MethodGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].jvm_hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }
}

#endif
