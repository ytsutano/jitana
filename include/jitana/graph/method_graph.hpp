#ifndef JITANA_METHOD_GRAPH_HPP
#define JITANA_METHOD_GRAPH_HPP

#include "jitana/graph/insn_graph.hpp"
#include "jitana/graph/graph_common.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>

namespace jitana {
    namespace detail {
        using method_graph_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A method vertex descriptor.
    using method_vertex_descriptor
            = detail::method_graph_traits::vertex_descriptor;

    /// A method edge descriptor.
    using method_edge_descriptor = detail::method_graph_traits::edge_descriptor;

    struct method_param {
        std::string descriptor;
        std::string name;
    };

    /// A method graph vertex property.
    struct method_vertex_property {
        dex_method_hdl hdl;
        dex_type_hdl class_hdl;
        jvm_method_hdl jvm_hdl;
        dex_access_flags access_flags;
        std::vector<method_param> params;
        insn_graph insns;
    };

    /// A method graph edge property.
    using method_edge_property = any_edge_property;

    /// A method graph property.
    struct method_graph_property {
        std::unordered_map<jvm_method_hdl, method_vertex_descriptor>
                jvm_hdl_to_vertex;
        std::unordered_map<dex_method_hdl, method_vertex_descriptor>
                hdl_to_vertex;
    };

    /// A method graph.
    using method_graph = boost::adjacency_list<boost::vecS, boost::vecS,
                                               boost::bidirectionalS,
                                               method_vertex_property,
                                               method_edge_property,
                                               method_graph_property>;

    template <typename MethodGraph>
    inline boost::optional<method_vertex_descriptor>
    lookup_method_vertex(const dex_method_hdl& hdl, const MethodGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }

    template <typename MethodGraph>
    inline boost::optional<method_vertex_descriptor>
    lookup_method_vertex(const jvm_method_hdl& hdl, const MethodGraph& g)
    {
        const auto& lut = g[boost::graph_bundle].jvm_hdl_to_vertex;
        auto it = lut.find(hdl);
        if (it != end(lut)) {
            return it->second;
        }
        return boost::none;
    }

    struct method_super_edge_property {
        bool interface;
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const method_super_edge_property& prop)
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
