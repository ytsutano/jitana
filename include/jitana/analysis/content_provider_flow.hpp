/*
 * Copyright (c) 2016, 2017, Yutaka Tsutano
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

#ifndef JITANA_CONTENT_PROVIDER_FLOW_HPP
#define JITANA_CONTENT_PROVIDER_FLOW_HPP

#include "jitana/jitana.hpp"
#include "jitana/algorithm/property_tree.hpp"
#include "jitana/algorithm/unique_sort.hpp"

#include <unordered_map>

#include <boost/range/iterator_range.hpp>

namespace jitana {
    struct content_provider_flow_edge_property {
        std::string description;
    };

    inline void
    print_graphviz_attr(std::ostream& os,
                        const content_provider_flow_edge_property& prop)
    {
        os << "label=" << boost::escape_dot_string(prop.description) << ", ";
        os << "fontcolor=orange, color=orange";
    }
}

namespace jitana {
    namespace detail {
        inline auto compute_content_provider_handlers(virtual_machine& vm)
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
                        auto val = x.second.get_optional<std::string>(
                                "<xmlattr>.android:name");
                        if (!val || val->size() == 0) {
                            std::cerr << package_name;
                            std::cerr << ": invalid XML";
                            std::cerr << " (content provider)\n";
                            continue;
                        }
                        std::string name = *val;

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

                add_intent_filters("provider");
            }

            for (auto& x : handlers) {
                unique_sort(x.second);
            }

            return handlers;
        }
    }
}

#endif
