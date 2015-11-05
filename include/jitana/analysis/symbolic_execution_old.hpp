#ifndef JITANA_SYMBOLIC_EXECUTION_HPP
#define JITANA_SYMBOLIC_EXECUTION_HPP

#include "jitana/jitana.hpp"
#include "jitana/vm_graph/edge_filtered_graph.hpp"

#include <iostream>
#include <unordered_map>

#include <boost/graph/depth_first_search.hpp>
#include <boost/range/iterator_range.hpp>

#include <stp/c_interface.h>

namespace jitana {
    class symbolic_execution_tree {
    public:
        struct exec_state_diff {
            std::unordered_map<register_idx, ::Expr> registers;
        };

        struct tree_vertex_property {
            insn_vertex_descriptor insn_vertex;
            exec_state_diff state_diff;
            ::Expr bool_pc = nullptr;
            ::Expr int_pc = nullptr;
            ::Expr heap_array;
            ::Expr static_array;
            uint32_t heap_head;
        };

        using tree_graph
                = boost::adjacency_list<boost::vecS, boost::vecS,
                                        boost::bidirectionalS,
                                        tree_vertex_property,
                                        insn_control_flow_edge_property>;
        using tree_vertex_descriptor
                = boost::graph_traits<tree_graph>::vertex_descriptor;
        using tree_edge_descriptor
                = boost::graph_traits<tree_graph>::edge_descriptor;

    public:
        symbolic_execution_tree(virtual_machine& vm,
                                const method_vertex_descriptor& v);

        symbolic_execution_tree(const symbolic_execution_tree&) = default;
        symbolic_execution_tree(symbolic_execution_tree&&) = default;

        ~symbolic_execution_tree();

        void append(insn_vertex_descriptor iv);

        ::Expr in_register_expr(register_idx reg_idx,
                                tree_vertex_descriptor tv) const;
        ::Expr out_register_expr(register_idx reg_idx,
                                 tree_vertex_descriptor tv) const;

        const virtual_machine& vm() const
        {
            return vm_;
        }

        virtual_machine& vm()
        {
            return vm_;
        }

        const tree_graph& tree() const
        {
            return tree_;
        }

        const insn_graph& cfg() const
        {
            return cfg_;
        }

        const ::VC& vc() const
        {
            return vc_;
        }

        void print_graphviz(std::ostream& os) const;

        uint32_t static_offset(class_vertex_descriptor v)
        {
            auto it = static_offsets_.find(v);
            if (it != end(static_offsets_)) {
                return it->second;
            }

            auto offset = static_head_;
            static_offsets_[v] = offset;
            static_head_ += vm_.classes()[v].static_size;

            return offset;
        }

    private:
        virtual_machine& vm_;
        tree_graph tree_;
        tree_vertex_descriptor last_tree_vertex_;
        insn_graph cfg_;
        ::VC vc_;
        uint32_t static_head_ = 0;
        std::unordered_map<class_vertex_descriptor, uint32_t> static_offsets_;
    };

    void symbolic_execution(virtual_machine& vm,
                            const method_vertex_descriptor& v);
}

#endif
