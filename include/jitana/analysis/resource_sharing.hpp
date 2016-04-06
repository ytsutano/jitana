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
#include "jitana/analysis_graph/resource_sharing_graph.hpp"

#include <boost/graph/graphviz.hpp>
#include <boost/range/adaptors.hpp>

namespace jitana {
    static std::vector<std::string> app_names; // TODO: remove.

    struct r_hdl {
        uint8_t id;
        std::string name;
    };

    inline void add_resource_graph_edges_explicit(
            virtual_machine& vm, std::vector<std::pair<r_hdl, r_hdl>>& sos_sis)
    {
        jvm_method_hdl intent_mh = {{0, "Landroid/content/Intent;"},
                                    "setClassName(Ljava/lang/String;Ljava/"
                                    "lang/String;)Landroid/content/Intent;"};
        auto intent_mv = vm.find_method(intent_mh, true);

        const auto& mg = vm.methods();
        bool sink_set_flag = false;
        for (const auto& v : boost::make_iterator_range(vertices(mg))) {
            const auto& ig = mg[v].insns;
            r_hdl r_so;
            r_hdl r_si;
            register_idx sink_reg_idx;

            for (const auto& iv : boost::adaptors::reverse(vertices(ig))) {
                const auto& insn = ig[iv].insn;

                if (sink_set_flag) {
                    const auto* csi = get<insn_const_string>(&insn);
                    if (csi && csi->regs[0] == sink_reg_idx) {
                        sink_set_flag = false;

                        r_si.name = "L";
                        r_si.name += csi->const_val;
                        r_si.name += ";";
                        std::replace(begin(r_si.name), end(r_si.name), '.',
                                     '/');
                        r_si.id = -1;
                        sos_sis.emplace_back(r_so, r_si);
                    }
                }

                if (info(op(insn)).can_invoke()) {
                    auto imm = vm.find_method(*const_val<dex_method_hdl>(insn),
                                              true);
                    if (imm && *imm == intent_mv) {
                        r_so.name = mg[v].jvm_hdl.unique_name;
                        r_so.id = mg[v].class_hdl.file_hdl.loader_hdl.idx;

                        const auto& u = uses(insn);
                        if (!u.empty()) {
                            sink_set_flag = true;
                            sink_reg_idx = u.back();
                        }
                    }
                }
            }

            sink_set_flag = false;
        }

        const auto& cg = vm.classes();
        for (const auto& v : boost::make_iterator_range(vertices(cg))) {
            for (auto& so_si : sos_sis) {
                if (cg[v].jvm_hdl.descriptor == so_si.second.name) {
                    so_si.second.id = cg[v].hdl.file_hdl.loader_hdl.idx;
                }
            }
        }
    }

    inline void add_resource_graph_edges_implicit(
            virtual_machine& vm,
            std::vector<std::pair<class_loader_hdl, std::string>>&
                    all_source_intents)
    {
        jvm_method_hdl intent_mh
                = {{0, "Landroid/content/Intent;"},
                   "setAction(Ljava/lang/String;)Landroid/content/Intent;"};
        auto intent_mv = vm.find_method(intent_mh, true);

        const auto& mg = vm.methods();
        bool intent_set_flag = false;
        register_idx sink_reg_intent_idx = register_idx::idx_unknown;
        for (const auto& v : boost::make_iterator_range(vertices(mg))) {
            const auto& ig = mg[v].insns;
            for (const auto& iv : boost::adaptors::reverse(vertices(ig))) {
                const auto& insn = ig[iv].insn;

                if (intent_set_flag) {
                    const auto* csi = get<insn_const_string>(&insn);
                    if (csi && csi->regs[0] == sink_reg_intent_idx) {
                        intent_set_flag = false;
                        all_source_intents.emplace_back(
                                mg[v].class_hdl.file_hdl.loader_hdl.idx,
                                csi->const_val);
                    }
                }

                if (info(op(insn)).can_invoke()) {
                    auto imm = vm.find_method(*const_val<dex_method_hdl>(insn),
                                              true);
                    if (imm && *imm == intent_mv) {
                        const auto& u = uses(insn);
                        if (!u.empty()) {
                            intent_set_flag = true;
                            sink_reg_intent_idx = u.back();
                        }
                    }
                }
            }
        }
    }

    inline void write_graphviz_resource_sharing_graph_explicit(
            std::ostream& os, std::vector<std::pair<r_hdl, r_hdl>>& sos_sis,
            resource_sharing_graph& rsg)
    {
        for (const auto& so_si : sos_sis) {
            if (so_si.first.id != 255 && so_si.second.id != 255) {
                rsg_edge_property eprop;
                eprop.kind = rsg_edge_property::explicit_intent;
                eprop.description = so_si.second.name;
                add_edge(so_si.first.id, so_si.second.id, eprop, rsg);
            }
        }

        auto prop_writer = [&](std::ostream& os, const auto& v) {
            std::stringstream label_ss;
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
            os << "colorscheme=pastel19, ";
            if (degree(v, rsg) > 0) {
                os << "style=filled, ";
            }
            else {
                os << "style=dotted, ";
            }
            os << "fillcolor=";
            os << ((9 + unsigned(v) - 3) % 9 + 1);
            os << "]";
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            switch (rsg[e].kind) {
            case rsg_edge_property::explicit_intent:
                os << "color=red, weight=10";
                break;
            case rsg_edge_property::implicit_intent:
                os << "color=blue";
                break;
            }
            // os << ", label=" << escape_dot_record_string(rsg[e].description);
            os << "]";
        };

        auto gprop_writer = [&](std::ostream& os) {
            os << "\n";
            os << "\n";
        };

        write_graphviz(os, rsg, prop_writer, eprop_writer, gprop_writer);
    }

    inline void write_graphviz_resource_sharing_graph_implicit(
            std::vector<std::pair<class_loader_hdl, std::string>>& source,
            std::vector<std::pair<class_loader_hdl, std::string>>& sink,
            resource_sharing_graph& rsg)
    {
        for (const auto& so : source) {
            for (const auto& si : sink) {
                if (si.second == so.second && si.first != so.first) {
                    rsg_edge_property ep;
                    ep.kind = rsg_edge_property::implicit_intent;
                    ep.description = so.second;
                    add_edge(unsigned(so.first), unsigned(si.first), ep, rsg);
                }
            }
        }
    }

    inline void parse_manifest_load_intent(
            class_loader_hdl loader_hdl, const std::string& aut_manifest_tree,
            std::vector<std::pair<class_loader_hdl, std::string>>&
                    all_si_intents)
    {
        bool found_action = false;

        std::ifstream ifs(aut_manifest_tree);
        std::string line;
        while (std::getline(ifs, line)) {
            if (found_action) {
                auto pos = line.find("(Raw: \"");
                if (pos != std::string::npos) {
                    found_action = false;

                    auto s = line.substr(pos + 7);
                    s.erase(std::find(begin(s), end(s), '"'), end(s));
                    if (s != "android.intent.action.MAIN"
                        && s != "android.intent.action.VIEW") {
                        all_si_intents.emplace_back(loader_hdl, s);
                    }
                }
            }

            if (line.find("E: action") != std::string::npos) {
                found_action = true;
            }
        }
    }
}

#endif
