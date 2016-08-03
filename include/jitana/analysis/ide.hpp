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
        dex_reg_hdl hdl;
        bool return_vertex;
    };

    /// A labeled exploded super graph edge property.
    struct lesg_edge_property {
        enum {
            kind_normal,
            kind_call,
            kind_return,
            kind_call_to_return,
            kind_register_chain
        } kind;
    };

    /// A labeled exploded super graph property.
    struct lesg_property {
    };

    /// A labeled exploded super graph.
    using labeled_exploded_super_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, lesg_vertex_property,
                                    lesg_edge_property, lesg_property>;

    template <typename LESG>
    inline void write_graphviz_labeled_exploded_super_graph(std::ostream& os,
                                                            const LESG& g)
    {
        auto vprop_writer = [&](std::ostream& os, const auto& v) {
            os << "[";
            os << "label=\"{" << g[v].hdl.insn_hdl.method_hdl << "|{"
               << g[v].hdl.insn_hdl.idx << "|";
            if (g[v].hdl.idx != -1) {
                os << register_idx(g[v].hdl.idx);
            }
            os << "}}\",";
            if (g[v].return_vertex) {
                os << "color=gray;";
            }
            os << "URL=\"insn/" << g[v].hdl.insn_hdl.method_hdl << ".dot\",";
            os << "shape=record";
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            switch (g[e].kind) {
            case lesg_edge_property::kind_normal:
                os << "color=black, weight=10";
                break;
            case lesg_edge_property::kind_call:
                os << "color=red";
                break;
            case lesg_edge_property::kind_return:
                os << "color=orange";
                break;
            case lesg_edge_property::kind_call_to_return:
                os << "color=gray, weight=2";
                break;
            case lesg_edge_property::kind_register_chain:
                os << "weight=1000, rank=same";
                break;
            }
            os << "]";
        };

        write_graphviz(os, g, vprop_writer, eprop_writer);
    }
}

namespace jitana {
    struct string_propagation_insn_visitor
            : public boost::static_visitor<void> {
        template <typename T>
        void operator()(const T& x) const
        {
            std::cout << "T\n";
        }
    };

    template <typename ContextualCallGraph>
    labeled_exploded_super_graph
    make_labeled_exploded_super_graph(virtual_machine& vm,
                                      const ContextualCallGraph& ccg)
    {
        labeled_exploded_super_graph lesg;
        const auto& mg = vm.methods();

        struct vertex_lut_entry {
            std::vector<lesg_vertex_descriptor> in_vertices;
            std::vector<lesg_vertex_descriptor> out_vertices;
        };
        std::unordered_map<dex_method_hdl, vertex_lut_entry> vertex_lut;

        // We have special registers in DEX which are represented as negative
        // numbers in Jitana. Here, we want to represent all registers as
        // non-negative numbers, we use the following rules:
        //
        //     reg_idx::idx_exception (-3) => 0.
        //     reg_idx::idx_result    (-2) => 1.
        //     reg_idx::idx_unknown   (-1) => 2 (empty fact).
        //     reg_idx(n)                  => n + 3.
        size_t idx_exception_reg = 0;
        size_t idx_result_reg = 1;
        size_t idx_empty_fact = 2;

        // Create local vertices and edges.
        for (const auto& ccg_v : boost::make_iterator_range(vertices(ccg))) {
            const auto& mh = ccg[ccg_v].hdl;
            const auto& mv = *vm.find_method(mh, true);
            const auto& ig = mg[mv].insns;

            if (num_vertices(ig) == 0) {
                std::cout << "no vertices\n";
                continue;
            }

            int num_regs = ig[boost::graph_bundle].registers_size + 3;
            auto& in_lut = vertex_lut[mh].in_vertices;
            auto& out_lut = vertex_lut[mh].out_vertices;
            in_lut.resize(num_vertices(ig));
            out_lut.resize(num_vertices(ig));

            // Add vertices.
            for (const auto& iv : boost::make_iterator_range(vertices(ig))) {
                in_lut[iv] = num_vertices(lesg);
                for (int i = 0; i < num_regs; ++i) {
                    dex_reg_hdl hdl;
                    hdl.insn_hdl.method_hdl = mh;
                    hdl.insn_hdl.idx = iv;
                    hdl.idx = i - 3;
                    add_vertex({hdl, false}, lesg);
                }

                for (int i = 0; i < num_regs - 1; ++i) {
                    add_edge(in_lut[iv] + i, in_lut[iv] + i + 1,
                             {lesg_edge_property::kind_register_chain}, lesg);
                }

                if (get<insn_invoke>(&ig[iv].insn)) {
                    out_lut[iv] = num_vertices(lesg);

                    for (int reg = 0; reg < num_regs; ++reg) {
                        dex_reg_hdl hdl;
                        hdl.insn_hdl.method_hdl = mh;
                        hdl.insn_hdl.idx = iv;
                        hdl.idx = reg - 3;
                        add_vertex({hdl, true}, lesg);
                    }

                    for (int i = 0; i < num_regs - 1; ++i) {
                        add_edge(out_lut[iv] + i, out_lut[iv] + i + 1,
                                 {lesg_edge_property::kind_register_chain},
                                 lesg);
                    }
                }
                else {
                    out_lut[iv] = in_lut[iv];
                }
            }

            auto cfg
                    = make_edge_filtered_graph<insn_control_flow_edge_property>(
                            ig);

            std::vector<bool> reg_def_bits(num_regs);

            // Add local edges.
            for (const auto& iv : boost::make_iterator_range(vertices(cfg))) {
#if 0
                string_propagation_insn_visitor vis;
                boost::apply_visitor(vis, ig[iv].insn);
#else
                std::fill(begin(reg_def_bits), end(reg_def_bits), false);
                if (iv != 0) {
                    for (const auto& reg : defs(ig[iv].insn)) {
                        reg_def_bits[reg.value + 3] = true;
                    }
                }

                for (const auto& e :
                     boost::make_iterator_range(out_edges(iv, cfg))) {
                    auto tgt_iv = target(e, cfg);

                    for (int reg = 0; reg < num_regs; ++reg) {
                        auto src_reg = reg_def_bits[reg] ? idx_empty_fact : reg;

                        auto src_v = out_lut[iv] + src_reg;
                        auto tgt_v = in_lut[tgt_iv] + reg;

                        add_edge(src_v, tgt_v,
                                 {lesg_edge_property::kind_normal}, lesg);
                    }

                    if (out_lut[iv] != in_lut[iv]) {
                        // This is an invoke instruction: add call-to-return
                        // edges.
                        for (int reg = 2; reg < num_regs; ++reg) {
                            auto src_v = in_lut[iv] + reg;
                            auto tgt_v = out_lut[iv] + reg;

                            add_edge(src_v, tgt_v,
                                     {lesg_edge_property::kind_call_to_return},
                                     lesg);
                        }
                    }
                }
#endif
            }
        }

        // Create global edges.
        for (const auto& ccg_e : boost::make_iterator_range(edges(ccg))) {
            const auto& caller_mh = ccg[source(ccg_e, ccg)].hdl;
            const auto& callee_mh = ccg[target(ccg_e, ccg)].hdl;
            const auto& caller_mv = *vm.find_method(caller_mh, true);
            const auto& callee_mv = *vm.find_method(callee_mh, true);
            const auto& caller_ig = mg[caller_mv].insns;
            const auto& callee_ig = mg[callee_mv].insns;
            auto caller_iv = ccg[ccg_e].caller_insn_vertex;

            const auto* invoke_insn
                    = get<insn_invoke>(&caller_ig[caller_iv].insn);
            if (!invoke_insn) {
                throw std::runtime_error(
                        "call edge from non-invoke instruction");
            }

            auto caller_lut_it = vertex_lut.find(caller_mh);
            auto callee_lut_it = vertex_lut.find(callee_mh);

            if (caller_lut_it == end(vertex_lut)
                || callee_lut_it == end(vertex_lut)) {
                std::cout << "lut does not exist\n";
                continue;
            }

            const auto& caller_in_lut = vertex_lut[caller_mh].in_vertices;
            const auto& caller_out_lut = vertex_lut[caller_mh].out_vertices;
            const auto& callee_in_lut = vertex_lut[callee_mh].in_vertices;
            const auto& callee_out_lut = vertex_lut[callee_mh].out_vertices;

            auto call_v = caller_in_lut[caller_iv];
            auto return_v = caller_out_lut[caller_iv];
            if (call_v == return_v) {
                throw std::runtime_error(
                        "call edge from non-invoke instruction");
            }

            auto entry_v = callee_in_lut.front();
            auto exit_v = callee_out_lut.back();

            add_edge(call_v + idx_empty_fact, entry_v + idx_empty_fact,
                     {lesg_edge_property::kind_call}, lesg);
            auto regs_size = callee_ig[boost::graph_bundle].registers_size;
            auto ins_size = callee_ig[boost::graph_bundle].ins_size;

            auto caller_regs = regs_vec(*invoke_insn);
            for (size_t i = 0; i < caller_regs.size(); ++i) {
                std::cout << caller_regs[i].value << " -> "
                          << regs_size - ins_size + i << "\n";
                add_edge(call_v + caller_regs[i].value + 3,
                         entry_v + regs_size - ins_size + i + 3,
                         {lesg_edge_property::kind_call}, lesg);
            }
            std::cout << "\n";

            add_edge(exit_v + idx_result_reg, return_v + idx_result_reg,
                     {lesg_edge_property::kind_return}, lesg);
            add_edge(exit_v + idx_exception_reg, return_v + idx_exception_reg,
                     {lesg_edge_property::kind_return}, lesg);
            add_edge(exit_v + idx_empty_fact, return_v + idx_empty_fact,
                     {lesg_edge_property::kind_return}, lesg);
        }

        return lesg;
    }
}

#endif
