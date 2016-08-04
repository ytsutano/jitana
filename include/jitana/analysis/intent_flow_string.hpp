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

#ifndef JITANA_INTENT_FLOW_STRING_HPP
#define JITANA_INTENT_FLOW_STRING_HPP

#include "intent_flow.hpp"

#include <boost/range/iterator_range.hpp>

namespace jitana {
    inline void add_explicit_intent_flow_edges_string(virtual_machine& vm);
    inline void add_implicit_intent_flow_edges_string(virtual_machine& vm);

    inline void add_intent_flow_edges_string(virtual_machine& vm)
    {
        add_explicit_intent_flow_edges_string(vm);
        add_implicit_intent_flow_edges_string(vm);
    }
}

namespace jitana {
    inline void add_explicit_intent_flow_edges_string(virtual_machine& vm)
    {
        auto intent_handlers = detail::compute_explicit_intent_handlers(vm);

        auto& lg = vm.loaders();
        const auto& mg = vm.methods();

        for (const auto& mv : boost::make_iterator_range(vertices(mg))) {
            const auto& ig = mg[mv].insns;

            for (const auto& iv : boost::make_iterator_range(vertices(ig))) {
                const auto* cs_insn = get<insn_const_string>(&ig[iv].insn);
                if (!cs_insn) {
                    continue;
                }

                auto it = intent_handlers.find(cs_insn->const_val);
                if (it != end(intent_handlers)) {
                    intent_flow_edge_property prop;
                    prop.kind = intent_flow_edge_property::explicit_intent;
                    prop.description = cs_insn->const_val;
                    auto lv = *find_loader_vertex(
                            mg[mv].hdl.file_hdl.loader_hdl, lg);
                    for (const auto& target_lv : it->second) {
                        add_edge(lv, target_lv, prop, lg);
                    }
                }
            }
        }
    }

    inline void add_implicit_intent_flow_edges_string(virtual_machine& vm)
    {
        auto intent_handlers = detail::compute_implicit_intent_handlers(vm);

        auto& lg = vm.loaders();
        const auto& mg = vm.methods();

        for (const auto& mv : boost::make_iterator_range(vertices(mg))) {
            const auto& ig = mg[mv].insns;

            for (const auto& iv : boost::make_iterator_range(vertices(ig))) {
                const auto* cs_insn = get<insn_const_string>(&ig[iv].insn);
                if (!cs_insn) {
                    continue;
                }

                auto it = intent_handlers.find(cs_insn->const_val);
                if (it != end(intent_handlers)) {
                    intent_flow_edge_property prop;
                    prop.kind = intent_flow_edge_property::implicit_intent;
                    prop.description = cs_insn->const_val;
                    auto lv = *find_loader_vertex(
                            mg[mv].hdl.file_hdl.loader_hdl, lg);
                    for (const auto& target_lv : it->second) {
                        add_edge(lv, target_lv, prop, lg);
                    }
                }
            }
        }
    }
}

#endif
