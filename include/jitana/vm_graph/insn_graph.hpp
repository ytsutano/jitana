#ifndef JITANA_INSN_GRAPH_HPP
#define JITANA_INSN_GRAPH_HPP

#include "jitana/vm_core/insn.hpp"
#include "jitana/vm_graph/graph_common.hpp"

#include <iostream>
#include <vector>

#include <boost/variant.hpp>

namespace jitana {
    namespace detail {
        using insn_graph_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// An instruction vertex descriptor.
    using insn_vertex_descriptor = detail::insn_graph_traits::vertex_descriptor;

    /// An instruction edge descriptor.
    using insn_edge_descriptor = detail::insn_graph_traits::edge_descriptor;

    /// An instruction graph vertex property.
    struct insn_vertex_property {
        dex_insn_hdl hdl;
        jitana::insn insn;
        long long counter = 0;
        uint32_t off;
        int line_num = 0;
    };

    /// An instruction graph edge property.
    using insn_edge_property = any_edge_property;

    /// A try catch block.
    struct try_catch_block {
        insn_vertex_descriptor first;
        insn_vertex_descriptor last;
        std::vector<std::pair<dex_type_hdl, insn_vertex_descriptor>> hdls;
        bool has_catch_all;
        insn_vertex_descriptor catch_all;
    };

    /// An instruction graph property.
    struct insn_graph_property {
        std::unordered_map<uint16_t, insn_vertex_descriptor> offset_to_vertex;
        dex_method_hdl hdl;
        jvm_method_hdl jvm_hdl;
        std::vector<try_catch_block> try_catches;
        size_t registers_size;
        size_t ins_size;
        size_t outs_size;
        uint32_t insns_off;
        // std::vector<std::string> param_names;
    };

    /// An instruction graph.
    using insn_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, insn_vertex_property,
                                    insn_edge_property, insn_graph_property>;

    template <typename InsnGraph>
    inline boost::optional<class_vertex_descriptor>
    lookup_insn_vertex(uint16_t off, const InsnGraph& g)
    {
        if (num_vertices(g) <= 2) {
            return boost::none;
        }
        auto it_begin = vertices(g).first;
        auto it_end = vertices(g).second;

        // Ignore the entry instruction.
        ++it_begin;

        // Ignore the exit instruction.
        --it_end;

        // Use binary search to find the vertex with the offset.
        auto it = std::lower_bound(
                it_begin, it_end, off,
                [&g](const insn_vertex_descriptor& v, uint16_t off) {
                    return g[v].off < off;
                });
        if (it != it_end && !(off < g[*it].off)) {
            return *it;
        }

        return boost::none;
    }

    struct insn_control_flow_fallthrough {
        friend std::ostream& operator<<(std::ostream& os,
                                        const insn_control_flow_fallthrough&)
        {
            return os;
        }
    };

    using insn_control_flow_branch_cond
            = boost::variant<insn_control_flow_fallthrough, bool, int32_t>;

    struct insn_control_flow_edge_property {
        insn_control_flow_branch_cond branch_cond;

        insn_control_flow_edge_property()
                : branch_cond(insn_control_flow_fallthrough())
        {
        }

        insn_control_flow_edge_property(
                const insn_control_flow_branch_cond& branch_cond)
                : branch_cond(branch_cond)
        {
        }
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const insn_control_flow_edge_property& prop)
    {
        os << "color=blue, fontcolor=blue";

        const auto& cond = prop.branch_cond;

        if (get<insn_control_flow_fallthrough>(&cond)) {
            os << ",weight=100";
        }
        else {
            os << ",weight=10";
        }
        os << ", taillabel=\"" << std::boolalpha << cond << "\"";
    }

    template <typename InsnGraph>
    bool is_basic_block_head(insn_vertex_descriptor v, const InsnGraph& g)
    {
        for (const auto& e : boost::make_iterator_range(in_edges(v, g))) {
            auto src_v = source(e, g);
            const auto& src_insn = g[src_v].insn;
            if (get<insn_if>(&src_insn) || get<insn_if_z>(&src_insn)
                || get<insn_switch>(&src_insn) || get<insn_entry>(&src_insn)) {
                return true;
            }
        }
        return false;
    }
}

#endif
