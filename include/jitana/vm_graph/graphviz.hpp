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

#if 1
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
#endif

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

    /// Writes an instruction graph to the stream in the Graphviz format.
    template <typename InsnGraph>
    inline void write_graphviz_insn_graph(std::ostream& os, const InsnGraph& g)
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
#if 1
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
#endif
        };

        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }
}

#endif
