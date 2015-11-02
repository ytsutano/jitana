#ifndef JITANA_CLASS_GRAPH_HPP
#define JITANA_CLASS_GRAPH_HPP

#include "jitana/graph/graph_common.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>

namespace jitana {
    namespace detail {
        using class_graph_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A class vertex descriptor.
    using class_vertex_descriptor
            = detail::class_graph_traits::vertex_descriptor;

    /// A class edge descriptor.
    using class_edge_descriptor = detail::class_graph_traits::edge_descriptor;

    /// A class graph vertex property.
    struct class_vertex_property {
        dex_type_hdl hdl;
        jvm_type_hdl jvm_hdl;
        dex_access_flags access_flags;
        std::vector<dex_field_hdl> static_fields;
        std::vector<dex_field_hdl> instance_fields;
        std::vector<dex_method_hdl> dtable;
        std::vector<dex_method_hdl> vtable;
        uint16_t static_size;
        uint16_t instance_size;
    };

    /// A class graph edge property.
    using class_edge_property = any_edge_property;

    /// A class graph property.
    struct class_graph_property {
        std::unordered_map<jvm_type_hdl, class_vertex_descriptor>
                jvm_hdl_to_vertex;
        std::unordered_map<dex_type_hdl, class_vertex_descriptor> hdl_to_vertex;
    };

    /// A class graph.
    using class_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS,
                                    class_vertex_property, class_edge_property,
                                    class_graph_property>;

    template <typename ClassGraph>
    inline boost::optional<class_vertex_descriptor>
    lookup_class_vertex(const dex_type_hdl& hdl, const ClassGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }

    template <typename ClassGraph>
    inline boost::optional<class_vertex_descriptor>
    lookup_class_vertex(const jvm_type_hdl& hdl, const ClassGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].jvm_hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }

    struct class_super_edge_property {
        bool interface;
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const class_super_edge_property& prop)
    {
        os << "dir=both, arrowhead=none, arrowtail=empty, ";
        if (prop.interface) {
            os << "color=yellow, weight=1";
        }
        else {
            os << "color=orange, weight=5";
        }
    }
}

#endif
