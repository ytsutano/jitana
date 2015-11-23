#include "jitana/analysis/points_to.hpp"
#include "jitana/analysis/data_flow.hpp"
#include "jitana/algorithm/unique_sort.hpp"

#include <vector>
#include <queue>
#include <unordered_map>

#include <boost/variant.hpp>
#include <boost/pending/disjoint_sets.hpp>

using namespace jitana;

namespace {
    using pag_parent_map
            = boost::property_map<pointer_assignment_graph,
                                  pag_vertex_descriptor
                                          pag_vertex_property::*>::type;
    using pag_rank_map = boost::property_map<pointer_assignment_graph,
                                             int pag_vertex_property::*>::type;
    using pag_disjoint_sets
            = boost::disjoint_sets<pag_rank_map, pag_parent_map>;
}

namespace {
    struct invocation {
        dex_insn_hdl callsite;
        method_vertex_descriptor mv;

        friend bool operator==(const invocation& x, const invocation& y)
        {
            return x.mv == y.mv && x.callsite == y.callsite;
        }
    };
}

namespace std {
    template <>
    struct hash<invocation> {
        size_t operator()(const invocation& x) const
        {
            return std::hash<jitana::dex_insn_hdl>()(x.callsite)
                    * std::hash<jitana::method_vertex_descriptor>()(x.mv);
        }
    };
}

namespace {
    struct points_to_algorithm_data {
        pointer_assignment_graph& pag;
        contextual_call_graph& cg;
        virtual_machine& vm;
        bool on_the_fly_cg;
        pag_disjoint_sets ds;

        const insn_graph* ig = nullptr;
        insn_vertex_descriptor iv;
        dex_insn_hdl insn_hdl;
        dex_insn_hdl context = no_insn_hdl;

        std::deque<pag_vertex_descriptor> worklist;
        std::unordered_set<invocation> visited;

        points_to_algorithm_data(pointer_assignment_graph& pag,
                                 contextual_call_graph& cg, virtual_machine& vm,
                                 bool on_the_fly_cg)
                : pag(pag),
                  cg(cg),
                  vm(vm),
                  on_the_fly_cg(on_the_fly_cg),
                  ds(get(&pag_vertex_property::rank, pag),
                     get(&pag_vertex_property::parent, pag))
        {
        }

        void move_current_insn(const insn_graph* ig, insn_vertex_descriptor iv)
        {
            points_to_algorithm_data::ig = ig;
            points_to_algorithm_data::iv = iv;
            points_to_algorithm_data::insn_hdl = {
                    (*ig)[boost::graph_bundle].hdl, static_cast<uint16_t>(iv)};
        }

        void propagate_incremental(pag_vertex_descriptor src_v,
                                   pag_vertex_descriptor dst_v)
        {
            propagate(dst_v, pag[pag[src_v].parent].in_set);
        }

        void propagate_all(pag_vertex_descriptor src_v,
                           pag_vertex_descriptor dst_v)
        {
            propagate(dst_v, pag[pag[src_v].parent].points_to_set);
        }

    private:
        void propagate(pag_vertex_descriptor dst_v,
                       const std::vector<pag_vertex_descriptor>& out_set)
        {
            if (!out_set.empty()) {
                auto& dst_in_set = pag[pag[dst_v].parent].in_set;
                auto mid = dst_in_set.insert(end(dst_in_set), begin(out_set),
                                             end(out_set));
                std::inplace_merge(begin(dst_in_set), mid, end(dst_in_set));
                dst_in_set.erase(
                        std::unique(begin(dst_in_set), end(dst_in_set)),
                        end(dst_in_set));
            }

            if (!pag[dst_v].dirty) {
                worklist.push_back(dst_v);
                pag[dst_v].dirty = true;
            }
        }
    };
}

namespace {
    template <typename Func>
    inline void for_each_incoming_reg(points_to_algorithm_data& d_,
                                      register_idx reg, Func f)
    {
        using boost::make_iterator_range;
        using boost::type_erasure::any_cast;

        dex_reg_hdl reg_hdl(d_.insn_hdl, reg.value);

        for (const auto& e : make_iterator_range(in_edges(d_.iv, *d_.ig))) {
            using edge_prop_t = insn_data_flow_edge_property;
            const auto* de = any_cast<const edge_prop_t*>(&(*d_.ig)[e]);
            if (de != nullptr && de->reg == reg) {
                reg_hdl.insn_hdl.idx = source(e, *d_.ig);
                f(reg_hdl);
            }
        }
    }

    std::ofstream ccg_ofs("output/ccg.dot");

    inline void add_invoke_edges(points_to_algorithm_data& d_,
                                 method_vertex_descriptor mv,
                                 const insn_invoke& insn)
    {
        auto add_call_edge = [&](const dex_reg_hdl& dst_reg_hdl,
                                 const dex_reg_hdl& src_reg_hdl) {
            auto src_v = make_vertex_for_reg(src_reg_hdl, d_.context, d_.pag);
            auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.insn_hdl, d_.pag);

            add_edge(src_v, dst_v, {pag_edge_property::kind_assign}, d_.pag);
            // d_.propagate_incremental(src_v, dst_v);
        };

        const auto& mg = d_.vm.methods();
        const auto& tgt_mvprop = mg[mv];
        const auto& params = tgt_mvprop.params;
        const auto& tgt_igprop = tgt_mvprop.insns[boost::graph_bundle];

        if (tgt_mvprop.access_flags & acc_abstract) {
            return;
        }

        // lookup_ccg_vertex(tgt_mvprop.hdl, d_.cg);
        ccg_ofs << "\"" << d_.insn_hdl.method_hdl << "\"";
        ccg_ofs << "->";
        ccg_ofs << "\"" << tgt_mvprop.hdl << "\"";
        ccg_ofs << "[label=\"";
        ccg_ofs << d_.insn_hdl.idx;
        ccg_ofs << "\"];\n";

        // Parameters.
        {
            std::vector<int> reg_offsets;
            {
                int offset = 0;

                // Non-static method has a this pointer as the first
                // argument.
                if (!(tgt_mvprop.access_flags & acc_static)) {
                    reg_offsets.push_back(0);
                    ++offset;
                }

                for (const auto& p : params) {
                    char desc = p.descriptor[0];

                    reg_offsets.push_back((desc == 'L' || desc == '[') ? offset
                                                                       : -1);

                    ++offset;
                    if (desc == 'J' || desc == 'D') {
                        // Ignore next register if the parameter is a
                        // wide type.
                        ++offset;
                    }
                }
            }

            dex_insn_hdl tgt_entry_insn_hdl(tgt_mvprop.hdl, 0);
            dex_reg_hdl dst_reg_hdl(tgt_entry_insn_hdl, 0);

            auto actual_reg_start
                    = tgt_igprop.registers_size - tgt_igprop.ins_size;
            if (insn.is_regs_range()) {
                auto formal_reg_start = insn.regs[0].value;
                for (auto off : reg_offsets) {
                    if (off != -1) {
                        dst_reg_hdl.idx = actual_reg_start + off;
                        for_each_incoming_reg(
                                d_, formal_reg_start + off,
                                [&](const dex_reg_hdl& src_reg_hdl) {
                                    add_call_edge(dst_reg_hdl, src_reg_hdl);
                                });
                    }
                }
            }
            else {
                for (size_t i = 0; i < reg_offsets.size(); ++i) {
                    auto off = reg_offsets[i];
                    if (off != -1) {
                        dst_reg_hdl.idx = actual_reg_start + off;
                        for_each_incoming_reg(
                                d_, insn.regs[i].value,
                                [&](const dex_reg_hdl& src_reg_hdl) {
                                    add_call_edge(dst_reg_hdl, src_reg_hdl);
                                });
                    }
                }
            }
        }

        // Return value.
        auto ret_desc = tgt_mvprop.jvm_hdl.return_descriptor()[0];
        if (ret_desc == 'L' || ret_desc == '[') {
            dex_insn_hdl tgt_exit_insn_hdl(tgt_mvprop.hdl,
                                           num_vertices(tgt_mvprop.insns) - 1);
            dex_reg_hdl src_reg_hdl(tgt_exit_insn_hdl,
                                    register_idx::idx_result);
            dex_reg_hdl dst_reg_hdl(d_.insn_hdl, register_idx::idx_result);

            auto src_v = make_vertex_for_reg(src_reg_hdl, d_.insn_hdl, d_.pag);
            auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.context, d_.pag);

            add_edge(src_v, dst_v, {pag_edge_property::kind_assign}, d_.pag);
            // d_.propagate_incremental(src_v, dst_v);
        }
    }

    inline void add_alloc_edge(points_to_algorithm_data& d_,
                               register_idx dst_reg,
                               const boost::optional<dex_type_hdl>& type)
    {
        dex_reg_hdl dst_reg_hdl(d_.insn_hdl, dst_reg.value);

        auto src_v = make_vertex_for_alloc(d_.insn_hdl, d_.pag);
        auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.context, d_.pag);

        d_.pag[src_v].type = type;
        d_.pag[dst_v].type = type;

        add_edge(src_v, dst_v, {pag_edge_property::kind_alloc}, d_.pag);
        d_.propagate_all(src_v, dst_v);
    }

    inline void add_assign_edge(points_to_algorithm_data& d_,
                                register_idx dst_reg, register_idx src_reg,
                                const boost::optional<dex_type_hdl>& dst_type
                                = boost::none)
    {
        using boost::make_iterator_range;
        using boost::type_erasure::any_cast;

        dex_reg_hdl dst_reg_hdl(d_.insn_hdl, dst_reg.value);

        // If the destination is the result register, we know that it
        // comes from the return instruction. Thus we associate the
        // result resiter to the exit instruction.
        if (dst_reg == register_idx::idx_result) {
            dst_reg_hdl.insn_hdl.idx = num_vertices(*d_.ig) - 1;
        }

        auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.context, d_.pag);
        d_.pag[dst_v].type = dst_type;

        for_each_incoming_reg(d_, src_reg, [&](const dex_reg_hdl& src_reg_hdl) {
            auto src_v = make_vertex_for_reg(src_reg_hdl, d_.context, d_.pag);

            add_edge(src_v, dst_v, {pag_edge_property::kind_assign}, d_.pag);
            // d_.propagate_incremental(src_v, dst_v);
        });
    }

    inline void add_astore_edge(points_to_algorithm_data& d_,
                                register_idx src_reg, register_idx obj_reg,
                                register_idx /*idx_reg*/)
    {
        for_each_incoming_reg(d_, src_reg, [&](const dex_reg_hdl& src_reg_hdl) {
            for_each_incoming_reg(
                    d_, obj_reg, [&](const dex_reg_hdl& obj_reg_hdl) {
                        auto src_v = make_vertex_for_reg(src_reg_hdl,
                                                         d_.context, d_.pag);
                        auto dst_v = make_vertex_for_reg_dot_array(
                                obj_reg_hdl, d_.context, d_.pag);
                        auto obj_v = make_vertex_for_reg(obj_reg_hdl,
                                                         d_.context, d_.pag);

                        d_.pag[obj_v].dereferenced_by.push_back(dst_v);
                        unique_sort(d_.pag[obj_v].dereferenced_by);

                        add_edge(src_v, dst_v, {pag_edge_property::kind_astore},
                                 d_.pag);
                    });
        });
    }

    inline void add_aload_edge(points_to_algorithm_data& d_,
                               register_idx dst_reg, register_idx obj_reg,
                               register_idx /*idx_reg*/)
    {
        dex_reg_hdl dst_reg_hdl(d_.insn_hdl, dst_reg.value);

        for_each_incoming_reg(d_, obj_reg, [&](const dex_reg_hdl& obj_reg_hdl) {
            auto src_v = make_vertex_for_reg_dot_array(obj_reg_hdl, d_.context,
                                                       d_.pag);
            auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.context, d_.pag);
            auto obj_v = make_vertex_for_reg(obj_reg_hdl, d_.context, d_.pag);

            d_.pag[obj_v].dereferenced_by.push_back(src_v);
            unique_sort(d_.pag[obj_v].dereferenced_by);

            add_edge(src_v, dst_v, {pag_edge_property::kind_aload}, d_.pag);
        });
    }

    inline void add_istore_edge(points_to_algorithm_data& d_,
                                register_idx src_reg, register_idx obj_reg,
                                const dex_field_hdl& field_hdl)
    {
        const auto& fv = d_.vm.find_field(field_hdl, false);
        if (!fv) {
            std::cerr << "istore: field not found: " << d_.vm.jvm_hdl(field_hdl)
                      << "\n";
            return;
        }

        const auto& fg = d_.vm.fields();
        if (fg[*fv].type_char == 'L' || fg[*fv].type_char == '[') {
            for_each_incoming_reg(d_, src_reg, [&](const dex_reg_hdl&
                                                           src_reg_hdl) {
                for_each_incoming_reg(
                        d_, obj_reg, [&](const dex_reg_hdl& obj_reg_hdl) {
                            auto src_v = make_vertex_for_reg(
                                    src_reg_hdl, d_.context, d_.pag);
                            auto dst_v = make_vertex_for_reg_dot_field(
                                    obj_reg_hdl, field_hdl, d_.context, d_.pag);
                            auto obj_v = make_vertex_for_reg(
                                    obj_reg_hdl, d_.context, d_.pag);

                            d_.pag[obj_v].dereferenced_by.push_back(dst_v);
                            unique_sort(d_.pag[obj_v].dereferenced_by);

                            add_edge(src_v, dst_v,
                                     {pag_edge_property::kind_istore}, d_.pag);
                        });
            });
        }
    }

    inline void add_iload_edge(points_to_algorithm_data& d_,
                               register_idx dst_reg, register_idx obj_reg,
                               const dex_field_hdl& field_hdl)
    {
        const auto& fv = d_.vm.find_field(field_hdl, false);
        if (!fv) {
            std::cerr << "iload: field not found: " << d_.vm.jvm_hdl(field_hdl)
                      << "\n";
            return;
        }

        const auto& fg = d_.vm.fields();
        if (fg[*fv].type_char == 'L' || fg[*fv].type_char == '[') {
            dex_reg_hdl dst_reg_hdl(d_.insn_hdl, dst_reg.value);

            for_each_incoming_reg(
                    d_, obj_reg, [&](const dex_reg_hdl& obj_reg_hdl) {
                        auto src_v = make_vertex_for_reg_dot_field(
                                obj_reg_hdl, field_hdl, d_.context, d_.pag);
                        auto dst_v = make_vertex_for_reg(dst_reg_hdl,
                                                         d_.context, d_.pag);
                        auto obj_v = make_vertex_for_reg(obj_reg_hdl,
                                                         d_.context, d_.pag);

                        d_.pag[obj_v].dereferenced_by.push_back(src_v);
                        unique_sort(d_.pag[obj_v].dereferenced_by);

                        add_edge(src_v, dst_v, {pag_edge_property::kind_iload},
                                 d_.pag);
                    });
        }
    }

    inline void add_sstore_edge(points_to_algorithm_data& d_,
                                register_idx src_reg,
                                const dex_field_hdl& field_hdl)
    {
        const auto& fg = d_.vm.fields();
        const auto& fv = d_.vm.find_field(field_hdl, false);
        if (!fv) {
            std::stringstream ss;
            ss << "ailed to find the static field ";
            ss << d_.vm.jvm_hdl(field_hdl);
            throw std::runtime_error(ss.str());
        }

        if (fg[*fv].type_char == 'L' || fg[*fv].type_char == '[') {
            for_each_incoming_reg(
                    d_, src_reg, [&](const dex_reg_hdl& src_reg_hdl) {
                        auto src_v = make_vertex_for_reg(src_reg_hdl,
                                                         d_.context, d_.pag);
                        auto dst_v = make_vertex_for_static_field(field_hdl,
                                                                  d_.pag);

                        add_edge(src_v, dst_v, {pag_edge_property::kind_sstore},
                                 d_.pag);
                    });
        }
    }

    inline void add_sload_edge(points_to_algorithm_data& d_,
                               register_idx dst_reg,
                               const dex_field_hdl& field_hdl)
    {
        const auto& fg = d_.vm.fields();
        const auto& fv = d_.vm.find_field(field_hdl, false);
        if (!fv) {
            std::stringstream ss;
            ss << "ailed to find the static field ";
            ss << d_.vm.jvm_hdl(field_hdl);
            throw std::runtime_error(ss.str());
        }

        if (fg[*fv].type_char == 'L' || fg[*fv].type_char == '[') {
            dex_reg_hdl dst_reg_hdl(d_.insn_hdl, dst_reg.value);

            auto src_v = make_vertex_for_static_field(field_hdl, d_.pag);
            auto dst_v = make_vertex_for_reg(dst_reg_hdl, d_.context, d_.pag);

            add_edge(src_v, dst_v, {pag_edge_property::kind_sload}, d_.pag);
        }
    }
};

namespace {
    class pag_insn_visitor : public boost::static_visitor<void> {
    public:
        pag_insn_visitor(points_to_algorithm_data& d,
                         std::queue<invocation>& invoc_queue)
                : d_(d), invoc_queue_(invoc_queue)
        {
        }

        void operator()(const insn_move& x)
        {
            switch (x.op) {
            case opcode::op_move_object:
            case opcode::op_move_object_from16:
            case opcode::op_move_object_16:
            case opcode::op_move_result_object:
                add_assign_edge(d_, x.regs[0], x.regs[1]);
                break;
            default:
                break;
            }
        }

        void operator()(const insn_return& x)
        {
            switch (x.op) {
            case opcode::op_return_object:
                add_assign_edge(d_, register_idx(register_idx::idx_result),
                                x.regs[0]);
                break;
            default:
                break;
            }
        }

        void operator()(const insn_check_cast& x)
        {
            auto cv = d_.vm.find_class(x.const_val, false);
            if (!cv) {
                std::cerr << "insn_check_cast: class not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            add_assign_edge(d_, x.regs[0], x.regs[0], d_.vm.classes()[*cv].hdl);
        }

        void operator()(const insn_const_string& x)
        {
            auto loader_idx = d_.insn_hdl.method_hdl.file_hdl.loader_hdl.idx;
            auto cv = d_.vm.find_class({loader_idx, "Ljava/lang/String;"},
                                       false);
            if (!cv) {
                std::cerr << "insn_const_string: class not found: ";
                std::cerr << "Ljava/lang/String;\n";
                return;
            }

            add_alloc_edge(d_, x.regs[0], d_.vm.classes()[*cv].hdl);
        }

        void operator()(const insn_const_class& x)
        {
            auto loader_idx = d_.insn_hdl.method_hdl.file_hdl.loader_hdl.idx;
            auto cv = d_.vm.find_class({loader_idx, "Ljava/lang/Class;"},
                                       false);
            if (!cv) {
                std::cerr << "insn_const_class: class not found: ";
                std::cerr << "Ljava/lang/Class;\n";
                return;
            }

            add_alloc_edge(d_, x.regs[0], d_.vm.classes()[*cv].hdl);
        }

        void operator()(const insn_new_instance& x)
        {
            auto cv = d_.vm.find_class(x.const_val, false);
            if (!cv) {
                std::cerr << "insn_new_instance: class not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            // Run <clinit> of the target class.
            {
                const auto& cg = d_.vm.classes();
                auto clinit_mv = d_.vm.find_method(
                        jvm_method_hdl(cg[*cv].jvm_hdl, "<clinit>()V"), false);
                if (clinit_mv) {
                    invoc_queue_.push({no_insn_hdl, *clinit_mv});
                }
            }

            add_alloc_edge(d_, x.regs[0], d_.vm.classes()[*cv].hdl);
        }

        void operator()(const insn_new_array& x)
        {
#if 1
            // For now array is assumed to be Ljava/lang/Object;.
            auto loader_idx = d_.insn_hdl.method_hdl.file_hdl.loader_hdl.idx;
            auto cv = d_.vm.find_class({loader_idx, "Ljava/lang/Object;"},
                                       false);
            if (!cv) {
                std::cerr << "insn_new_array: class not found: ";
                std::cerr << "Ljava/lang/Object;\n";
                return;
            }

            add_alloc_edge(d_, x.regs[0], d_.vm.classes()[*cv].hdl);
#else
            add_alloc_edge(x.regs[0], boost::none); // x.const_val);
#endif
        }

        void operator()(const insn_filled_new_array&)
        {
        }

        void operator()(const insn_aget& x)
        {
            add_aload_edge(d_, x.regs[0], x.regs[1], x.regs[2]);
        }

        void operator()(const insn_aput& x)
        {
            add_astore_edge(d_, x.regs[0], x.regs[1], x.regs[2]);
        }

        void operator()(const insn_iget& x)
        {
            auto fv = d_.vm.find_field(x.const_val, false);
            if (!fv) {
                std::cerr << "insn_iget: field not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            add_iload_edge(d_, x.regs[0], x.regs[1], d_.vm.fields()[*fv].hdl);
        }

        void operator()(const insn_iput& x)
        {
            auto fv = d_.vm.find_field(x.const_val, false);
            if (!fv) {
                std::cerr << "insn_iput: field not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            add_istore_edge(d_, x.regs[0], x.regs[1], d_.vm.fields()[*fv].hdl);
        }

        void operator()(const insn_sget& x)
        {
            auto fv = d_.vm.find_field(x.const_val, false);
            if (!fv) {
                std::cerr << "insn_sget: field not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            // Run <clinit> of the target class.
            {
                const auto& fg = d_.vm.fields();
                auto clinit_mv = d_.vm.find_method(
                        jvm_method_hdl(fg[*fv].jvm_hdl.type_hdl, "<clinit>()V"),
                        false);
                if (clinit_mv) {
                    invoc_queue_.push({no_insn_hdl, *clinit_mv});
                }
            }

            add_sload_edge(d_, x.regs[0], d_.vm.fields()[*fv].hdl);
        }

        void operator()(const insn_sput& x)
        {
            auto fv = d_.vm.find_field(x.const_val, false);
            if (!fv) {
                std::cerr << "insn_sput: field not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            // Run <clinit> of the target class.
            {
                const auto& fg = d_.vm.fields();
                auto clinit_mv = d_.vm.find_method(
                        jvm_method_hdl(fg[*fv].jvm_hdl.type_hdl, "<clinit>()V"),
                        false);
                if (clinit_mv) {
                    invoc_queue_.push({no_insn_hdl, *clinit_mv});
                }
            }

            add_sstore_edge(d_, x.regs[0], d_.vm.fields()[*fv].hdl);
        }

        void operator()(const insn_invoke& x)
        {
            auto mv = d_.vm.find_method(x.const_val, false);
            if (!mv) {
                std::cerr << "insn_invoke: method not found: "
                          << d_.vm.jvm_hdl(x.const_val) << "\n";
                return;
            }

            const auto& mg = d_.vm.methods();
            switch (x.op) {
            case opcode::op_invoke_static:
            case opcode::op_invoke_static_range:
                // Run <clinit> of the target class.
                {
                    auto clinit_mv = d_.vm.find_method(
                            jvm_method_hdl(mg[*mv].jvm_hdl.type_hdl,
                                           "<clinit>()V"),
                            false);
                    if (clinit_mv) {
                        invoc_queue_.push({no_insn_hdl, *clinit_mv});
                    }
                }
                break;
            default:
                break;
            }

            if (info(x.op).can_virtually_invoke()) {
                for_each_incoming_reg(
                        d_, x.regs[0], [&](const dex_reg_hdl& obj_reg_hdl) {
                            auto obj_v = make_vertex_for_reg(
                                    obj_reg_hdl, d_.context, d_.pag);
                            auto& vms = d_.pag[obj_v].virtual_invoke_insns;
                            vms.emplace_back(d_.context, d_.insn_hdl);
                            unique_sort(vms);
                        });
            }

            if (!d_.on_the_fly_cg || !info(x.op).can_virtually_invoke()) {
                auto inheritance_mg
                        = make_edge_filtered_graph<method_super_edge_property>(
                                mg);

                boost::vector_property_map<int> color_map(
                        static_cast<unsigned>(num_vertices(inheritance_mg)));
                auto f = [&](method_vertex_descriptor v,
                             const decltype(inheritance_mg)&) {
                    invoc_queue_.push({d_.insn_hdl, v});
                    add_invoke_edges(d_, v, x);
                    return false;
                };
                boost::depth_first_visit(inheritance_mg, *mv,
                                         boost::default_dfs_visitor{},
                                         color_map, f);
            }
        }

        void operator()(const insn_invoke_quick&)
        {
        }

        template <typename T>
        void operator()(const T&)
        {
        }

    private:
        points_to_algorithm_data& d_;
        std::queue<invocation>& invoc_queue_;
    };

    class pag_updater {
    public:
        pag_updater(pointer_assignment_graph& pag, contextual_call_graph& cg,
                    virtual_machine& vm, bool on_the_fly_cg)
                : d_(pag, cg, vm, on_the_fly_cg)
        {
        }

        bool update(const method_vertex_descriptor& entry_mv)
        {
            make_vertices_from_method(entry_mv);
            simplify();

#define PRINT_PROGRESS 0
#if PRINT_PROGRESS
            int counter = 0;
            std::printf("%12s%12s%12s%12s%12s%12s\n", "counter", "worklist",
                        "vertices", "alloc", "alloc.field", "alloc.array");
            auto print_stats = [&]() {
                const auto& gprop = d_.pag[boost::graph_bundle];
                std::printf("%12d%12lu%12lu%12lu%12lu%12lu\n", counter,
                            d_.worklist.size(), num_vertices(d_.pag),
                            gprop.alloc_vertex_lut.size(),
                            gprop.alloc_dot_field_vertex_lut.size(),
                            gprop.alloc_dot_array_vertex_lut.size());
            };
#endif

            while (!d_.worklist.empty()) {
#if PRINT_PROGRESS
                constexpr int period = 10000;
                if (counter % period == 0) {
                    print_stats();
                }
                counter++;
#endif

                // Pop a vertex from the d_.worklist.
                auto v = d_.worklist.front();
                d_.worklist.pop_front();
                d_.pag[v].dirty = false;

                filter_in_set(v);

                if (d_.pag[d_.pag[v].parent].in_set.empty()) {
                    // Points-to does not need to be changed.
                    continue;
                }

                update_points_to_set(v);
                update_dereferencer(v);

                if (d_.on_the_fly_cg
                    && !d_.pag[v].virtual_invoke_insns.empty()) {
                    // Compute a set of actual types of objects pointed by the
                    // in-set of the register.
                    std::vector<dex_type_hdl> alloc_types;
                    for (auto alloc_v : d_.pag[d_.pag[v].parent].in_set) {
                        alloc_types.push_back(*d_.pag[alloc_v].type);
                    }
                    unique_sort(alloc_types);

                    for (const auto& ih : d_.pag[v].virtual_invoke_insns) {
                        auto mv = *d_.vm.find_method(ih.second.method_hdl,
                                                     false);
                        const insn_graph& ig = d_.vm.methods()[mv].insns;
                        insn_vertex_descriptor iv = ih.second.idx;
                        if (const auto* insn = get<insn_invoke>(&ig[iv].insn)) {
                            // Target JVM method handle.
                            auto target_jmh = d_.vm.jvm_hdl(insn->const_val);

                            for (const auto& ath : alloc_types) {
                                // Update the type of the target_jmh to the
                                // actual one.
                                target_jmh.type_hdl = d_.vm.jvm_hdl(ath);

                                // Process the invoked method.
                                auto mv = d_.vm.find_method(target_jmh, false);
                                auto prev_context = d_.context;
                                auto prev_insn_hdl = d_.insn_hdl;
                                auto prev_iv = d_.iv;
                                auto prev_ig = d_.ig;
                                d_.context = ih.first;
                                d_.insn_hdl = ih.second;
                                d_.iv = iv;
                                d_.ig = &ig;
                                add_invoke_edges(d_, *mv, *insn);
                                make_vertices_from_method(*mv, ih.second);
                                d_.context = prev_context;
                                d_.insn_hdl = prev_insn_hdl;
                                d_.iv = prev_iv;
                                d_.ig = prev_ig;

                                simplify();
                            }
                        }
                        else {
                            throw std::runtime_error("shouldn't happen");
                        }
                    }
                }

                process_outgoing_edges(v);

                d_.pag[d_.pag[v].parent].in_set.clear();
            }
#if PRINT_PROGRESS
            print_stats();
#endif

            return true;
        }

    private:
        void filter_in_set(const pag_vertex_descriptor& v)
        {
            auto& p2s = d_.pag[d_.pag[v].parent].points_to_set;
            auto& in_set = d_.pag[d_.pag[v].parent].in_set;

            // Remove the elements already exist in p2s from in_set.
            std::vector<pag_vertex_descriptor> temp;
            temp.reserve(in_set.size());
            std::set_difference(begin(in_set), end(in_set), begin(p2s),
                                end(p2s), std::back_inserter(temp));
            swap(in_set, temp);

            // Apply type filtering.
            if (const auto& type = d_.pag[v].type) {
                if (auto cv = d_.vm.find_class(*type, false)) {
                    const auto& cg = d_.vm.classes();
                    auto it = std::remove_if(
                            begin(in_set), end(in_set),
                            [&](const pag_vertex_descriptor& alloc_v) {
                                const auto& alloc_type = d_.pag[alloc_v].type;
                                if (!alloc_type) {
                                    return false;
                                }
                                auto alloc_cv
                                        = d_.vm.find_class(*alloc_type, false);
                                if (!alloc_cv) {
                                    return false;
                                }
                                return !is_superclass_of(*cv, *alloc_cv, cg);
                            });
                    in_set.erase(it, end(in_set));
                }
            }
        }

        void update_points_to_set(const pag_vertex_descriptor& v)
        {
            // Merge in_set into p2s_set.
            auto& p2s = d_.pag[d_.pag[v].parent].points_to_set;
            auto& in_set = d_.pag[d_.pag[v].parent].in_set;
            auto p2s_mid = p2s.insert(end(p2s), begin(in_set), end(in_set));
            std::inplace_merge(begin(p2s), p2s_mid, end(p2s));
        }

        void update_dereferencer(const pag_vertex_descriptor& v)
        {
            using edge_list = std::vector<std::pair<pag_vertex_descriptor,
                                                    pag_vertex_descriptor>>;

            struct visitor : boost::static_visitor<void> {
                visitor(pag_vertex_descriptor dereferencer_v,
                        pag_vertex_descriptor obj_v,
                        points_to_algorithm_data& d, edge_list& edges_to_add)
                        : dereferencer_v_(dereferencer_v),
                          obj_v(obj_v),
                          d_(d),
                          edges_to_add_(edges_to_add)
                {
                }

                void operator()(const pag_reg&) const
                {
                }

                void operator()(const pag_alloc&) const
                {
                }

                void operator()(const pag_reg_dot_field& x) const
                {
                    auto& g = d_.pag;
                    auto obj_in_set = d_.pag[d_.pag[obj_v].parent].in_set;

                    const auto& fg = d_.vm.fields();
                    const auto& cg = d_.vm.classes();

                    auto field_hdl = x.field_hdl;
                    auto fv = d_.vm.find_field(field_hdl, false);
                    auto cv = d_.vm.find_class(fg[*fv].class_hdl, false);

                    for (auto alloc_v : obj_in_set) {
                        if (const auto& alloc_type = d_.pag[alloc_v].type) {
                            if (auto alloc_cv
                                = d_.vm.find_class(*alloc_type, false)) {
                                if (!is_superclass_of(*cv, *alloc_cv, cg)) {
                                    continue;
                                }
                            }
                        }

                        const auto& alloc_vertex
                                = get<pag_alloc>(g[alloc_v].vertex);
                        auto adf_v = make_vertex_for_alloc_dot_field(
                                alloc_vertex.hdl, field_hdl, g);

                        auto inv_adj
                                = inv_adjacent_vertices(dereferencer_v_, g);
                        for (auto v : boost::make_iterator_range(inv_adj)) {
                            edges_to_add_.push_back(std::make_pair(v, adf_v));
                        }

                        auto adj = adjacent_vertices(dereferencer_v_, g);
                        for (auto v : boost::make_iterator_range(adj)) {
                            edges_to_add_.push_back(std::make_pair(adf_v, v));
                        }
                    }
                }

                void operator()(const pag_alloc_dot_field&) const
                {
                }

                void operator()(const pag_static_field&) const
                {
                }

                void operator()(const pag_reg_dot_array&) const
                {
                    auto& g = d_.pag;
                    auto obj_in_set = d_.pag[d_.pag[obj_v].parent].in_set;

                    for (auto alloc_v : obj_in_set) {
                        const auto& alloc_vertex
                                = get<pag_alloc>(g[alloc_v].vertex);
                        auto adf_v = make_vertex_for_alloc_dot_array(
                                alloc_vertex.hdl, g);

                        auto inv_adj
                                = inv_adjacent_vertices(dereferencer_v_, g);
                        for (auto v : boost::make_iterator_range(inv_adj)) {
                            edges_to_add_.push_back(std::make_pair(v, adf_v));
                        }

                        auto adj = adjacent_vertices(dereferencer_v_, g);
                        for (auto v : boost::make_iterator_range(adj)) {
                            edges_to_add_.push_back(std::make_pair(adf_v, v));
                        }
                    }
                }

                void operator()(const pag_alloc_dot_array&) const
                {
                }

            private:
                pag_vertex_descriptor dereferencer_v_;
                pag_vertex_descriptor obj_v;
                points_to_algorithm_data& d_;
                edge_list& edges_to_add_;
            };

            auto& g = d_.pag;
            edge_list edges_to_add;

            auto dereferenced_by = g[v].dereferenced_by;
            for (auto dereferencer_v : dereferenced_by) {
                visitor vis(dereferencer_v, v, d_, edges_to_add);
                boost::apply_visitor(vis, g[dereferencer_v].vertex);
            }

            for (const auto& e : edges_to_add) {
                auto adj = adjacent_vertices(e.first, g);
                if (std::find(adj.first, adj.second, e.second) == adj.second) {
                    // The edge hasn't been added: add it now.
                    add_edge(e.first, e.second,
                             {pag_edge_property::kind_assign}, g);
                    d_.propagate_all(e.first, e.second);
                }
            }
        }

        void process_outgoing_edges(const pag_vertex_descriptor& v)
        {
            auto& g = d_.pag;

            // Process the outgoing edges.
            for (const auto& oe : boost::make_iterator_range(out_edges(v, g))) {
                switch (g[oe].kind) {
                case pag_edge_property::kind_alloc:
                case pag_edge_property::kind_assign:
                case pag_edge_property::kind_sstore:
                case pag_edge_property::kind_sload:
                    d_.propagate_incremental(v, target(oe, g));
                    break;
                case pag_edge_property::kind_istore:
                case pag_edge_property::kind_iload:
                case pag_edge_property::kind_astore:
                case pag_edge_property::kind_aload:
                    break;
                }
            }
        }

        void make_vertices_from_method(const method_vertex_descriptor& root_mv,
                                       const dex_insn_hdl& root_context
                                       = no_insn_hdl)
        {
            std::queue<invocation> invoc_queue;
            invoc_queue.push({root_context, root_mv});

            for (; !invoc_queue.empty(); invoc_queue.pop()) {
                auto invoc = invoc_queue.front();

                // If the method is already visited, we just ignore.
                if (d_.visited.find(invoc) != end(d_.visited)) {
                    continue;
                }
                d_.visited.insert(invoc);

                d_.context = invoc.callsite;
                auto mv = invoc.mv;

                const auto& mg = d_.vm.methods();
                const auto& mvprop = mg[mv];
                const auto& ig = mvprop.insns;

                ccg_ofs << "\"" << mvprop.hdl << "\"";
                ccg_ofs << "[label=\"";
                ccg_ofs << mvprop.jvm_hdl;
                ccg_ofs << "\"]\n";

                pag_insn_visitor vis(d_, invoc_queue);
                for (auto iv : boost::make_iterator_range(vertices(ig))) {
                    d_.move_current_insn(&ig, iv);
                    boost::apply_visitor(vis, ig[iv].insn);
                }
            }
        }

        void simplify()
        {
            // Colapse SCC.
        }

    private:
        points_to_algorithm_data d_;
    };
}

bool jitana::update_points_to_graphs(pointer_assignment_graph& pag,
                                     contextual_call_graph& cg,
                                     virtual_machine& vm,
                                     const method_vertex_descriptor& mv,
                                     bool on_the_fly_cg)
{
    pag_updater updater(pag, cg, vm, on_the_fly_cg);
    return updater.update(mv);
}
