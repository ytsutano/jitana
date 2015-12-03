/*
 * Copyright (c) 2015 Yutaka Tsutano.
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

#ifndef JITANA_GRAPHVIZ_HPP
#define JITANA_GRAPHVIZ_HPP

#include "jitana/jitana.hpp"

#include <regex>

#include <boost/graph/graphviz.hpp>

namespace jitana {
    inline std::string escape_dot_record_string(std::string s)
    {
        s = std::regex_replace(s, std::regex("<|>"), "\\$&");
        return boost::escape_dot_string(s);
    }

    /// Writes a class loader graph to the stream in the Graphviz format.
    template <typename LoaderGraph>
    inline void write_graphviz_loader_graph(std::ostream& os,
                                            const LoaderGraph& g)
    {
        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            label_ss << "{";
            label_ss << unsigned(g[v].loader.hdl());
            label_ss << "|" << g[v].loader.name();
            label_ss << "}|";
            for (const auto& dex_file : g[v].loader.dex_files()) {
                label_ss << dex_file.filename() << "\\n";
            }

            os << "[";
            os << "label=" << escape_dot_record_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "colorscheme=pastel19, style=filled, ";
            os << "fillcolor=";
            os << ((9 + unsigned(g[v].loader.hdl()) - 3) % 9 + 1);
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=RL;"; };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    /// Writes a class graph to the stream in the Graphviz format.
    template <typename ClassGraph>
    inline void write_graphviz_class_graph(std::ostream& os,
                                           const ClassGraph& g)
    {
        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            label_ss << "{";
            label_ss << g[v].hdl;
            label_ss << "|" << g[v].access_flags;
            label_ss << "}";
            label_ss << "|" << g[v].jvm_hdl.descriptor;

            os << "[";
            os << "label=" << boost::escape_dot_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "colorscheme=pastel19, style=filled, ";
            os << "fillcolor=";
            os << ((9 + unsigned(g[v].hdl.file_hdl.loader_hdl) - 3) % 9 + 1);
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=LR;"; };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    /// Writes a method graph to the stream in the Graphviz format.
    template <typename MethodGraph>
    inline void write_graphviz_method_graph(std::ostream& os,
                                            const MethodGraph& g)
    {
        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            label_ss << "{";
            label_ss << unsigned(g[v].hdl.file_hdl.loader_hdl);
            label_ss << "|" << g[v].access_flags;
            label_ss << "}";
            label_ss << "|" << g[v].jvm_hdl.type_hdl.descriptor;
            label_ss << "|" << g[v].jvm_hdl.unique_name;

            os << "[";
            os << "label=" << escape_dot_record_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "colorscheme=pastel19, style=filled, ";
            os << "fillcolor=";
            os << ((9 + unsigned(g[v].hdl.file_hdl.loader_hdl) - 3) % 9 + 1);
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=LR;"; };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    /// Writes a method graph to the stream in the Graphviz format.
    template <typename MethodGraph>
    inline void write_graphviz_method_graph(std::ostream& os,
                                            const MethodGraph& g,
                                            virtual_machine& vm)
    {
        const auto& cg = vm.classes();

        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            label_ss << "{";
            label_ss << g[v].hdl;
            label_ss << "|" << g[v].access_flags;
            label_ss << "}";
            label_ss << "|";
            if (auto cv = vm.find_class(g[v].class_hdl, false)) {
                label_ss << cg[*cv].descriptor << "\\n";
            }
            label_ss << g[v].unique_name;
            label_ss << "|";
            for (const auto& p : g[v].params) {
                label_ss << p.descriptor << " " << p.name << "\\l";
            }

            os << "[";
            os << "label=" << escape_dot_record_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "colorscheme=pastel19, style=filled, ";
            os << "fillcolor=";
            os << ((9 + unsigned(g[v].hdl.file_hdl.loader_hdl) - 3) % 9 + 1);
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=LR;"; };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    /// Writes a field graph to the stream in the Graphviz format.
    template <typename FieldGraph>
    inline void write_graphviz_field_graph(std::ostream& os,
                                           const FieldGraph& g)
    {
        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            label_ss << "{";
            label_ss << g[v].hdl;
            label_ss << "|class_hdl=" << g[v].class_hdl;
            label_ss << "}";
            label_ss << "|";
            label_ss << "{";
            label_ss << "offset=" << g[v].offset;
            label_ss << "|" << g[v].type_char;
            label_ss << "|" << g[v].jvm_hdl.unique_name;
            label_ss << "}";
            label_ss << "|" << g[v].access_flags;

            os << "[";
            os << "label=" << escape_dot_record_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "color="
               << (g[v].kind == field_vertex_property::static_field ? "red"
                                                                    : "blue");
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=LR;"; };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    namespace detail {
        class verbose_insn_const_print : public boost::static_visitor<bool> {
        public:
            verbose_insn_const_print(std::ostream& os,
                                     const virtual_machine& vm)
                    : os_(os), vm_(vm)
            {
            }

            template <typename T>
            bool operator()(const T& x)
            {
                return print(x.const_val);
            }

        private:
            bool print(const dex_type_hdl& x)
            {
                os_ << vm_.jvm_hdl(x);
                return true;
            }

            bool print(const dex_method_hdl& x)
            {
                os_ << vm_.jvm_hdl(x);
                return true;
            }

            bool print(const dex_field_hdl& x)
            {
                os_ << vm_.jvm_hdl(x);
                return true;
            }

            template <typename T>
            bool print(const T&)
            {
                return false;
            }

        private:
            std::ostream& os_;
            const virtual_machine& vm_;
        };
    }

    /// Writes an instruction graph to the stream in the Graphviz format.
    template <typename InsnGraph>
    inline void write_graphviz_insn_graph(std::ostream& os, const InsnGraph& g,
                                          const virtual_machine* vm = nullptr)
    {
        auto prop_writer = [&](std::ostream& os, const auto& v) {
            if (is_pseudo(g[v].insn)) {
                std::stringstream label_ss;

                label_ss << v << ": " << g[v].insn;
                os << "[";
                os << "label=" << escape_dot_record_string(label_ss.str());
                os << ",";
                os << "shape=plaintext";
                os << "]";
            }
            else {
                bool block_head = is_basic_block_head(v, g);
                std::stringstream label_ss;
                if (block_head) {
                    label_ss << "{" << g[v].counter << "|{";
                }
                label_ss << v;
                label_ss << "|" << g[v].off;
                if (g[v].line_num > 0) {
                    label_ss << " (line=" << g[v].line_num << ")";
                }
                label_ss << "|" << g[v].insn;
                if (vm) {
                    label_ss << "\\l";
                    detail::verbose_insn_const_print iprint(label_ss, *vm);
                    if (boost::apply_visitor(iprint, g[v].insn)) {
                        label_ss << "\\l";
                    }
                }
                if (block_head) {
                    label_ss << "}}";
                }

                os << "[";
                os << "label=" << escape_dot_record_string(label_ss.str());
                os << ",";
                os << "shape=record";
                if (block_head) {
                    os << ", color=red";
                    if (g[v].counter != 0) {
                        os << ", style=filled, fillcolor=mistyrose";
                    }
                }
                else {
                    if (g[v].counter != 0) {
                        std::runtime_error("counter is nonzero");
                    }
                }
                os << "]";
            }
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            print_graphviz_attr(os, g[e]);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) {
            auto gprop = g[boost::graph_bundle];

            os << "rankdir=UD;\n";
            os << "labelloc=t;\n";
            os << "label=\"" << gprop.hdl;
            os << "\\n";
            os << gprop.jvm_hdl.type_hdl.descriptor << "\n"
               << gprop.jvm_hdl.unique_name << "\";\n";

            // Print exeception handlers.
            for (const auto& tc : gprop.try_catches) {
                auto print_try_block_seq = [&]() {
                    for (auto v = tc.first;; ++v) {
                        os << v;
                        if (v == tc.last)
                            break;
                        os << ", ";
                    }
                };

                for (const auto& h : tc.hdls) {
                    print_try_block_seq();
                    os << " -> " << h.second;
                    os << " [label=\"" << h.first
                       << "\", color=darkgreen, style=dotted];\n";
                }

                if (tc.has_catch_all) {
                    print_try_block_seq();
                    os << " -> " << tc.catch_all;
                    os << " [color=darkgreen, style=dotted];\n";
                }
            }
        };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }
}

#endif
