/*
 * Copyright (c) 2016 Shakthi Bachala
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

// This file was contributed by Shakthi Bachala for the ISSTA specific analysis.
// It's not officially part of Jitana.

#ifndef JITANA_RESOURCE_SHARING_HPP
#define JITANA_RESOURCE_SHARING_HPP

#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_map>

#include "jitana/jitana.hpp"
#include "jitana/vm_graph/graph_common.hpp"

#include <boost/graph/graphviz.hpp>
#include <boost/range/adaptors.hpp>

namespace jitana {
    static std::vector<std::string> app_names;
    struct r_hdl {
        uint8_t id;
        std::string name;
    };

    inline void add_resource_graph_edges_explicit(
            virtual_machine& vm, std::vector<std::pair<r_hdl, r_hdl>>& sos_sis)
    {
        bool sink_set_flag = false;
        std::vector<std::string> sink_values;
        std::string sink_intent_value;

        jitana::jvm_method_hdl mhi2 = {{11, "Landroid/content/Intent;"},
                                       "setClassName(Ljava/lang/String;Ljava/"
                                       "lang/String;)Landroid/content/Intent;"};
        auto vmh2 = vm.find_method(mhi2, true);

        std::for_each(
                vertices(vm.methods()).first, vertices(vm.methods()).second,
                [&](const jitana::method_vertex_descriptor& v) {
                    jitana::insn_graph ig = vm.methods()[v].insns;
                    r_hdl r_so;
                    r_hdl r_si;
                    jitana::register_idx sink_register_idx;

                    for (const auto& iv :
                         boost::adaptors::reverse(vertices(ig))) {
                        const auto& prop = ig[iv];
                        const auto& insn_const_val
                                = jitana::const_val<std::string>(prop.insn);
                        const auto& insn_info = info(op(prop.insn));
                        const auto& uses_reg = uses(prop.insn);
                        const auto& def_reg = defs(prop.insn);
                        if (sink_set_flag == true && insn_info.sets_register()
                            && std::string(insn_info.mnemonic())
                                            .compare("const-string")
                                    == 0) {
                            for (auto& idx : def_reg) {
                                if (idx == sink_register_idx) {
                                    sink_set_flag = false;
                                    std::pair<r_hdl, r_hdl> r_p;
                                    sink_intent_value = *insn_const_val;
                                    sink_intent_value.insert(
                                            sink_intent_value.begin(), 'L');
                                    std::replace(sink_intent_value.begin(),
                                                 sink_intent_value.end(), '.',
                                                 '/');
                                    sink_intent_value.insert(
                                            sink_intent_value.end(), ';');
                                    r_si.name = sink_intent_value;
                                    r_si.id = -1;
                                    r_p = std::make_pair(r_so, r_si);
                                    sos_sis.push_back(r_p);
                                    // Reset Values
                                    //	r_so.name[0] =  "NULL_VAL";
                                    r_so.name = "NULL_VAL";
                                    r_so.id = -1;
                                    r_si.name = "NULL_VAL";
                                    r_si.id = -1;
                                }
                            }
                        }
                        if (insn_info.can_virtually_invoke()
                            || insn_info.can_directly_invoke()) {
                            jitana::dex_method_hdl method_hdl
                                    = *jitana::
                                              const_val<jitana::dex_method_hdl>(
                                                      prop.insn);
                            if (auto imm = vm.find_method(method_hdl, true)) {
                                if ((vm.methods()[*imm].jvm_hdl.unique_name
                                     == mhi2.unique_name)
                                    && (vm.methods()[*imm]
                                                .jvm_hdl.type_hdl.descriptor
                                        == mhi2.type_hdl.descriptor)) {
                                    r_so.name = vm.methods()[v]
                                                        .jvm_hdl.unique_name;
                                    r_so.id = vm.methods()[v]
                                                      .class_hdl.file_hdl
                                                      .loader_hdl.idx;
                                    int i = 0;
                                    for (auto& idx :
                                         boost::adaptors::reverse(uses_reg)) {
                                        if (i == 0) {
                                            sink_set_flag = true;
                                            sink_register_idx = idx;
                                            i++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    r_so.name = "NULL_VAL";
                    r_so.id = -1;
                    r_si.name = "NULL_VAL";
                    r_si.id = -1;
                    sink_set_flag = false;
                });
        std::for_each(
                vertices(vm.classes()).first, vertices(vm.classes()).second,
                [&](const jitana::class_vertex_descriptor& v) {
                    for (auto& so_si : sos_sis) {
                        if (vm.classes()[v].jvm_hdl.descriptor
                            == so_si.second.name) {
                            so_si.second.id
                                    = vm.classes()[v]
                                              .hdl.file_hdl.loader_hdl.idx;
                        }
                    }
                });
    }

    inline void add_resource_graph_edges_implicit(
            virtual_machine& vm,
            std::vector<std::pair<int, std::string>>& all_source_intents,
            std::vector<std::pair<int, std::string>>& /*all_sink_intents*/)
    {
        std::pair<int, std::string> so_intent;
        bool intent_set_flag = false;
        jitana::register_idx sink_register_intent_idx;
        std::string sink_intent_value;
        jitana::jvm_method_hdl mhi5
                = {{11, "Landroid/content/Intent;"},
                   "setAction(Ljava/lang/String;)Landroid/content/Intent;"};
        auto vmh5 = vm.find_method(mhi5, true);

        std::for_each(
                vertices(vm.methods()).first, vertices(vm.methods()).second,
                [&](const jitana::method_vertex_descriptor& v) {
                    jitana::insn_graph ig = vm.methods()[v].insns;
                    for (const auto& iv :
                         boost::adaptors::reverse(vertices(ig))) {
                        const auto& prop = ig[iv];
                        const auto& insn_const_val
                                = jitana::const_val<std::string>(prop.insn);
                        const auto& insn_info = info(op(prop.insn));
                        const auto& uses_reg = uses(prop.insn);
                        const auto& def_reg = defs(prop.insn);
                        if (intent_set_flag == true && insn_info.sets_register()
                            && std::string(insn_info.mnemonic())
                                            .compare("const-string")
                                    == 0) {
                            for (auto& idx : def_reg) {
                                if (idx == sink_register_intent_idx) {
                                    sink_intent_value = *insn_const_val;
                                    so_intent = make_pair(
                                            unsigned(vm.methods()[v]
                                                             .class_hdl.file_hdl
                                                             .loader_hdl.idx),
                                            *insn_const_val);
                                    intent_set_flag = false;
                                    std::cout << sink_intent_value << "\n";
                                    ;
                                    all_source_intents.push_back(so_intent);
                                }
                            }
                        }
                        if (insn_info.can_virtually_invoke()
                            || insn_info.can_directly_invoke()) {
                            jitana::dex_method_hdl method_hdl
                                    = *jitana::
                                              const_val<jitana::dex_method_hdl>(
                                                      prop.insn);
                            if (auto imm = vm.find_method(method_hdl, true)) {
                                if ((vm.methods()[*imm].jvm_hdl.unique_name
                                     == mhi5.unique_name)
                                    && (vm.methods()[*imm]
                                                .jvm_hdl.type_hdl.descriptor
                                        == mhi5.type_hdl.descriptor)) {
                                    int i = 0;
                                    std::cout << vm.methods()[v]
                                                         .jvm_hdl.unique_name
                                              << " " << vm.methods()[v].hdl.idx
                                              << " "
                                              << vm.methods()[v]
                                                         .jvm_hdl
                                                         .return_descriptor()
                                              << "\n";

                                    std::cout << " Target "
                                              << " jvm_type_hdl "
                                              << vm.methods()[*imm]
                                                         .jvm_hdl.type_hdl
                                              << "\t"
                                              << "unique_name "
                                              << vm.methods()[*imm]
                                                         .jvm_hdl.unique_name
                                              << "\n";
                                    for (auto& idx :
                                         boost::adaptors::reverse(uses_reg)) {
                                        if (i == 0) {
                                            sink_register_intent_idx = idx;
                                            i++;
                                            intent_set_flag = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                });
    }

    void write_graphviz_resource_sharing_graph_explicit(
            std::ostream& os, std::vector<std::pair<r_hdl, r_hdl>>& sos_sis,
            std::vector<std::pair<int, std::string>>& /*class_loader_np*/,
            boost::adjacency_list<>& g)
    {
        std::string source_app_name;
        std::string sink_app_name;
        for (auto& so_si : sos_sis) {
            if ((unsigned(so_si.first.id) != 255)
                && (unsigned(so_si.second.id) != 255)) {
                std::cout << "Explicit Source Values:"
                          << unsigned(so_si.first.id) << "Explicit Sink Values:"
                          << unsigned(so_si.second.id) << "\n";
                if (so_si.second.id != 255) {
                    boost::add_edge(so_si.second.id, so_si.first.id, g);
                }
            }
        }

        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
            // label_ss << "";
            label_ss << "{";
            label_ss << v;
            if (v == 0) {
                label_ss << "|"
                         << "system_apk";
            }
            else {
                label_ss << "|" << app_names[v - 1];
            }
            label_ss << "}";
            os << "[";
            os << "label=" << escape_dot_record_string(label_ss.str());
            os << ",";
            os << "shape=record";
            os << ",";
            os << "colorscheme=pastel19, style=filled, ";
            os << "]";
        };
        auto eprop_writer = [&](std::ostream& os, const auto& /*e*/) {
            os << "[";
            os << "dir=both, arrowhead=none, arrowtail=empty, ";
            os << "color=blue, weight=1";
            os << "]";
        };
        auto gprop_writer = [&](std::ostream& os) { os << "rankdir=LR;"; };
        write_graphviz(os, g, prop_writer, eprop_writer, gprop_writer);
    }

    void write_graphviz_resource_sharing_graph_implicit(
            std::ostream& /*os*/,
            std::vector<std::pair<int, std::string>>& source,
            std::vector<std::pair<int, std::string>>& sink,
            std::vector<std::pair<int, std::string>>& /*class_loader_np*/,
            boost::adjacency_list<>& g)
    {
        std::string source_app_name;
        std::string sink_app_name;

        for (auto& so : source) {
            for (auto& si : sink) {
                if ((si.second == so.second) && (si.first != so.first)) {
                    std::cout
                            << "Implicit  Source Values:" << unsigned(so.first)
                            << "Implicit Sink Values:" << unsigned(si.first)
                            << "Intent Values:" << so.second << "\n";
                    boost::add_edge(unsigned(si.first), unsigned(so.first), g);
                }
            }
        }
    }

    void parse_manifest_load_intent(
            int ky, std::string aut_manifest_tree,
            std::vector<std::pair<int, std::string>>& all_si_intents)
    {
        std::ifstream ifs_aut_manifest_tree(aut_manifest_tree);
        std::ifstream log_in_stream;
        std::string log_each_line;
        std::vector<std::string> log_all_line;

        int intent_filter = 0;
        int intent_filter_action = 0;
        int counter_ifa = 0;
        for (int i = 0; std::getline(ifs_aut_manifest_tree, log_each_line);
             i++) {
            log_all_line.push_back(log_each_line);
        }
        log_in_stream.close();

        std::pair<int, std::string> si_intent;
        for (auto& line : log_all_line) {
            log_in_stream >> line;
            std::string loc, n;
            std::size_t pos_space = line.find(" ");
            n = line.substr(pos_space + 1);
            loc = line.substr(0, pos_space);
            if (counter_ifa == 1) {
                std::size_t ita_start = line.find("(Raw: ");
                if (ita_start != std::string::npos) {
                    std::string i_f = line.substr(ita_start + 7);
                    counter_ifa = 0;
                    i_f.erase(std::remove(i_f.begin(), i_f.end(), '\"'),
                              i_f.end());
                    i_f.erase(std::remove(i_f.begin(), i_f.end(), ')'),
                              i_f.end());
                    std::cout << " Key1 " << ky << " Value " << i_f << "\n";
                    if ((i_f != "android.intent.action.MAIN")
                        && (i_f != "android.intent.action.VIEW")) {
                        si_intent = std::make_pair(ky, i_f);
                        all_si_intents.push_back(si_intent);
                    }
                }
            }
            std::size_t ifa_found = line.find("E: action");
            if (ifa_found != std::string::npos) {
                intent_filter_action++;
                counter_ifa = 1;
            }

            std::size_t if_found = line.find("E: intent-filter");
            if (if_found != std::string::npos) {
                intent_filter++;
            }
        }
    }
}

#endif
