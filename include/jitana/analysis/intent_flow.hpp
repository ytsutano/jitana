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

#ifndef JITANA_INTENT_FLOW_HPP
#define JITANA_INTENT_FLOW_HPP

#include "jitana/jitana.hpp"
#include "jitana/algorithm/property_tree.hpp"
#include "jitana/algorithm/unique_sort.hpp"

#include <unordered_map>

#include <boost/range/iterator_range.hpp>

namespace jitana {
    struct intent_flow_edge_property {
        enum { explicit_intent, implicit_intent } kind;
        std::string description;
    };

    inline void print_graphviz_attr(std::ostream& os,
                                    const intent_flow_edge_property& prop)
    {
        os << "label=" << boost::escape_dot_string(prop.description) << ", ";
        switch (prop.kind) {
        case intent_flow_edge_property::explicit_intent:
            os << "fontcolor=red, color=red";
            break;
        case intent_flow_edge_property::implicit_intent:
            os << "fontcolor=darkgreen, color=darkgreen";
            break;
        }
    }
}

namespace jitana {
    namespace detail {
        inline auto compute_explicit_intent_handlers(virtual_machine& vm)
        {
            std::unordered_map<std::string,
                               std::vector<loader_vertex_descriptor>>
                    handlers;

            const auto& lg = vm.loaders();
            for (const auto& lv : boost::make_iterator_range(vertices(lg))) {
                const auto* info = get<apk_info>(&lg[lv].info);
                if (!info) {
                    // Not an APK.
                    continue;
                }

                const auto& app_pt
                        = info->manifest_ptree().get_child("application");
                const auto& package_name = info->package_name();

                auto add_intent_filters = [&](const std::string& intent_type) {
                    for (const auto& x : child_elements(app_pt, intent_type)) {
                        auto name = x.second.get<std::string>(
                                "<xmlattr>.android:name");
                        assert(name.size() > 0);

                        if (name[0] == '.') {
                            name = package_name + name;
                        }
                        else if (std::find(begin(name), end(name), '.')
                                 == end(name)) {
                            name = package_name + '.' + name;
                        }

                        handlers[name].push_back(lv);
                    }
                };

                add_intent_filters("activity");
                add_intent_filters("service");
                add_intent_filters("provider");
                add_intent_filters("receiver");
            }

            for (auto& x : handlers) {
                unique_sort(x.second);
            }

            return handlers;
        }

        inline auto compute_implicit_intent_handlers(virtual_machine& vm)
        {
            std::unordered_map<std::string,
                               std::vector<loader_vertex_descriptor>>
                    handlers;

            const auto& lg = vm.loaders();
            for (const auto& lv : boost::make_iterator_range(vertices(lg))) {
                const auto* info = get<apk_info>(&lg[lv].info);
                if (!info) {
                    // Not an APK.
                    continue;
                }

                const auto& app_pt
                        = info->manifest_ptree().get_child("application");

                auto add_intent_filters = [&](const std::string& intent_type) {
                    for (const auto& x : child_elements(app_pt, intent_type)) {
                        for (const auto& y :
                             child_elements(x.second, "intent-filter")) {
                            for (const auto& z :
                                 child_elements(y.second, "action")) {
                                const auto& name = z.second.get<std::string>(
                                        "<xmlattr>.android:name");
                                if (name != "android.intent.action.MAIN"
                                    && name != "android.intent.action.VIEW") {
                                    handlers[name].push_back(lv);
                                }
                            }
                        }
                    }
                };

                add_intent_filters("activity");
                add_intent_filters("service");
                add_intent_filters("provider");
                add_intent_filters("receiver");
            }

            for (auto& x : handlers) {
                unique_sort(x.second);
            }

            return handlers;
        }
    }
}

#endif
