/*
 * Copyright (c) 2015, 2016, Yutaka Tsutano
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

#ifndef JITANA_POINTER_ASSIGNMENT_GRAPH_HPP
#define JITANA_POINTER_ASSIGNMENT_GRAPH_HPP

#include "jitana/jitana.hpp"

#include <sstream>

namespace jitana {
    const dex_insn_hdl no_insn_hdl = {{{0, 0}, 0}, 0};
}

namespace jitana {
    struct pag_reg {
        dex_reg_hdl hdl;

        friend bool operator==(const pag_reg& x, const pag_reg& y)
        {
            return x.hdl == y.hdl;
        }

        friend void operator<<(std::ostream& os, const pag_reg& x)
        {
            os << x.hdl;
        }
    };

    struct pag_alloc {
        dex_insn_hdl hdl;

        friend bool operator==(const pag_alloc& x, const pag_alloc& y)
        {
            return x.hdl == y.hdl;
        }

        friend void operator<<(std::ostream& os, const pag_alloc& x)
        {
            os << "alloc\\n" << x.hdl;
        }
    };

    struct pag_reg_dot_field {
        dex_reg_hdl reg_hdl;
        dex_field_hdl field_hdl;

        friend bool operator==(const pag_reg_dot_field& x,
                               const pag_reg_dot_field& y)
        {
            return x.reg_hdl == y.reg_hdl && x.field_hdl == y.field_hdl;
        }

        friend void operator<<(std::ostream& os, const pag_reg_dot_field& x)
        {
            os << "reg_dot_field\\n" << x.reg_hdl << " -> " << x.field_hdl;
        }
    };

    struct pag_alloc_dot_field {
        dex_insn_hdl insn_hdl;
        dex_field_hdl field_hdl;

        friend bool operator==(const pag_alloc_dot_field& x,
                               const pag_alloc_dot_field& y)
        {
            return x.insn_hdl == y.insn_hdl && x.field_hdl == y.field_hdl;
        }

        friend void operator<<(std::ostream& os, const pag_alloc_dot_field& x)
        {
            os << "alloc_dot_field\\n" << x.insn_hdl << " . " << x.field_hdl;
        }
    };

    struct pag_static_field {
        dex_field_hdl hdl;

        friend bool operator==(const pag_static_field& x,
                               const pag_static_field& y)
        {
            return x.hdl == y.hdl;
        }

        friend void operator<<(std::ostream& os, const pag_static_field& x)
        {
            os << "static_field\\n" << x.hdl;
        }
    };

    struct pag_reg_dot_array {
        dex_reg_hdl hdl;

        friend bool operator==(const pag_reg_dot_array& x,
                               const pag_reg_dot_array& y)
        {
            return x.hdl == y.hdl;
        }

        friend void operator<<(std::ostream& os, const pag_reg_dot_array& x)
        {
            os << "reg_dot_array\\n(*" << x.hdl << ")[x]";
        }
    };

    struct pag_alloc_dot_array {
        dex_insn_hdl hdl;

        friend bool operator==(const pag_alloc_dot_array& x,
                               const pag_alloc_dot_array& y)
        {
            return x.hdl == y.hdl;
        }

        friend void operator<<(std::ostream& os, const pag_alloc_dot_array& x)
        {
            os << "alloc_dot_array\\n" << x.hdl << "[x]";
        }
    };
}

namespace std {
    template <>
    struct hash<jitana::pag_reg> {
        size_t operator()(const jitana::pag_reg& x) const
        {
            return std::hash<jitana::dex_reg_hdl>()(x.hdl);
        }
    };

    template <>
    struct hash<jitana::pag_alloc> {
        size_t operator()(const jitana::pag_alloc& x) const
        {
            return std::hash<jitana::dex_insn_hdl>()(x.hdl);
        }
    };

    template <>
    struct hash<jitana::pag_reg_dot_field> {
        size_t operator()(const jitana::pag_reg_dot_field& x) const
        {
            return std::hash<jitana::dex_reg_hdl>()(x.reg_hdl) << 32
                    ^ std::hash<jitana::dex_field_hdl>()(x.field_hdl);
        }
    };

    template <>
    struct hash<jitana::pag_alloc_dot_field> {
        size_t operator()(const jitana::pag_alloc_dot_field& x) const
        {
            return std::hash<jitana::dex_insn_hdl>()(x.insn_hdl) << 32
                    ^ std::hash<jitana::dex_field_hdl>()(x.field_hdl);
        }
    };

    template <>
    struct hash<jitana::pag_static_field> {
        size_t operator()(const jitana::pag_static_field& x) const
        {
            return std::hash<jitana::dex_field_hdl>()(x.hdl);
        }
    };

    template <>
    struct hash<jitana::pag_reg_dot_array> {
        size_t operator()(const jitana::pag_reg_dot_array& x) const
        {
            return std::hash<jitana::dex_reg_hdl>()(x.hdl);
        }
    };

    template <>
    struct hash<jitana::pag_alloc_dot_array> {
        size_t operator()(const jitana::pag_alloc_dot_array& x) const
        {
            return std::hash<jitana::dex_insn_hdl>()(x.hdl);
        }
    };
}

namespace jitana {
    namespace detail {
        using pag_traits
                = boost::adjacency_list_traits<boost::vecS, boost::vecS,
                                               boost::bidirectionalS>;
    }

    /// A pointer assignment graph vertex descriptor.
    using pag_vertex_descriptor = detail::pag_traits::vertex_descriptor;

    /// A pointer assignment graph vertex property.
    struct pag_vertex_property {
        boost::variant<pag_reg, pag_alloc, pag_reg_dot_field,
                       pag_alloc_dot_field, pag_static_field, pag_reg_dot_array,
                       pag_alloc_dot_array>
                vertex;
        dex_insn_hdl context;

        boost::optional<dex_type_hdl> type;

        pag_vertex_descriptor parent;
        int rank;

        std::vector<pag_vertex_descriptor> in_set;
        std::vector<pag_vertex_descriptor> points_to_set;
        std::vector<pag_vertex_descriptor> dereferenced_by;
        std::vector<std::pair<dex_insn_hdl, dex_insn_hdl>> virtual_invoke_insns;

        bool dirty = false;
    };

    /// A pointer assignment graph edge property.
    struct pag_edge_property {
        enum {
            kind_alloc,
            kind_assign,
            kind_istore,
            kind_iload,
            kind_sstore,
            kind_sload,
            kind_astore,
            kind_aload
        } kind;
    };

    /// A pointer assignment graph property.
    struct pag_property {
        template <typename T>
        using lookup_table = std::unordered_multimap<T, pag_vertex_descriptor>;

        lookup_table<pag_reg> reg_vertex_lut;
        lookup_table<pag_alloc> alloc_vertex_lut;
        lookup_table<pag_reg_dot_field> reg_dot_field_vertex_lut;
        lookup_table<pag_alloc_dot_field> alloc_dot_field_vertex_lut;
        lookup_table<pag_static_field> static_field_vertex_lut;
        lookup_table<pag_reg_dot_array> reg_dot_array_vertex_lut;
        lookup_table<pag_alloc_dot_array> alloc_dot_array_vertex_lut;
    };

    /// A pointer assignment graph.
    using pointer_assignment_graph
            = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, pag_vertex_property,
                                    pag_edge_property, pag_property>;
}

namespace jitana {
    template <typename Key, typename PAG, typename LUT>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_vertex(const Key& key, const dex_insn_hdl& context, const PAG& g,
                      const LUT& lut)
    {
        // Get the vertices with the specified key.
        auto range = lut.equal_range(key);

        // Use a linear search to find the one with the matching context. This
        // may become a bottle neck if a number of contexts per key becomes
        // larger. If that happens, we need a new data structure with better
        // complexity and memory locality.
        auto it = std::find_if(range.first, range.second, [&](const auto& p) {
            return g[p.second].context == context;
        });
        if (it != range.second) {
            return it->second;
        }

        return boost::none;
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_reg_vertex(const pag_reg& key, const dex_insn_hdl& context,
                          const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].reg_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_alloc_vertex(const pag_alloc& key, const dex_insn_hdl& context,
                            const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].alloc_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_reg_dot_field_vertex(const pag_reg_dot_field& key,
                                    const dex_insn_hdl& context, const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].reg_dot_field_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_alloc_dot_field_vertex(const pag_alloc_dot_field& key,
                                      const dex_insn_hdl& context, const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].alloc_dot_field_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_static_field_vertex(const pag_static_field& key,
                                   const dex_insn_hdl& context, const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].static_field_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_reg_dot_array_vertex(const pag_reg_dot_array& key,
                                    const dex_insn_hdl& context, const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].reg_dot_array_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }

    template <typename PAG>
    inline boost::optional<pag_vertex_descriptor>
    lookup_pag_alloc_dot_array_vertex(const pag_alloc_dot_array& key,
                                      const dex_insn_hdl& context, const PAG& g)
    {
        const auto& lut = g[boost::graph_bundle].alloc_dot_array_vertex_lut;
        return lookup_pag_vertex(key, context, g, lut);
    }
}

namespace jitana {
    inline pag_vertex_descriptor
    make_vertex_for_reg(const dex_reg_hdl& hdl, const dex_insn_hdl& context,
                        pointer_assignment_graph& g)
    {
        pag_reg reg{hdl};
        if (auto v = lookup_pag_reg_vertex(reg, context, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = reg;
        g[v].context = context;
        g[v].parent = v;
        g[boost::graph_bundle].reg_vertex_lut.insert({reg, v});
        return v;
    }

    inline pag_vertex_descriptor
    make_vertex_for_alloc(const dex_insn_hdl& hdl, pointer_assignment_graph& g)
    {
        pag_alloc alloc{hdl};
        if (auto v = lookup_pag_alloc_vertex(alloc, no_insn_hdl, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = alloc;
        g[v].context = no_insn_hdl;
        g[v].parent = v;
        g[v].points_to_set.push_back(v);
        g[boost::graph_bundle].alloc_vertex_lut.insert({alloc, v});
        return v;
    }

    inline pag_vertex_descriptor make_vertex_for_reg_dot_field(
            const dex_reg_hdl& reg_hdl, const dex_field_hdl& field_hdl,
            const dex_insn_hdl& context, pointer_assignment_graph& g)
    {
        pag_reg_dot_field rdf{reg_hdl, field_hdl};
        if (auto v = lookup_pag_reg_dot_field_vertex(rdf, context, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = rdf;
        g[v].context = context;
        g[v].parent = v;
        g[boost::graph_bundle].reg_dot_field_vertex_lut.insert({rdf, v});
        return v;
    }

    inline pag_vertex_descriptor
    make_vertex_for_alloc_dot_field(const dex_insn_hdl& insn_hdl,
                                    const dex_field_hdl& field_hdl,
                                    pointer_assignment_graph& g)
    {
        pag_alloc_dot_field adf{insn_hdl, field_hdl};
        if (auto v = lookup_pag_alloc_dot_field_vertex(adf, no_insn_hdl, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = adf;
        g[v].context = no_insn_hdl;
        g[v].parent = v;
        g[boost::graph_bundle].alloc_dot_field_vertex_lut.insert({adf, v});
        return v;
    }

    inline pag_vertex_descriptor
    make_vertex_for_static_field(const dex_field_hdl& hdl,
                                 pointer_assignment_graph& g)
    {
        pag_static_field sf{hdl};
        if (auto v = lookup_pag_static_field_vertex(sf, no_insn_hdl, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = sf;
        g[v].context = no_insn_hdl;
        g[v].parent = v;
        g[boost::graph_bundle].static_field_vertex_lut.insert({sf, v});
        return v;
    }

    inline pag_vertex_descriptor
    make_vertex_for_reg_dot_array(const dex_reg_hdl& hdl,
                                  const dex_insn_hdl& context,
                                  pointer_assignment_graph& g)
    {
        pag_reg_dot_array rda{hdl};
        if (auto v = lookup_pag_reg_dot_array_vertex(rda, context, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = rda;
        g[v].context = context;
        g[v].parent = v;
        g[boost::graph_bundle].reg_dot_array_vertex_lut.insert({rda, v});
        return v;
    }

    inline pag_vertex_descriptor
    make_vertex_for_alloc_dot_array(const dex_insn_hdl& insn_hdl,
                                    pointer_assignment_graph& g)
    {
        pag_alloc_dot_array ada{insn_hdl};
        if (auto v = lookup_pag_alloc_dot_array_vertex(ada, no_insn_hdl, g)) {
            return *v;
        }

        auto v = add_vertex(g);
        g[v].vertex = ada;
        g[v].context = no_insn_hdl;
        g[v].parent = v;
        g[boost::graph_bundle].alloc_dot_array_vertex_lut.insert({ada, v});
        return v;
    }
}

namespace jitana {
    namespace detail {
        template <typename PAG>
        class vertex_printer : public boost::static_visitor<void> {
        public:
            vertex_printer(std::ostream& os, pag_vertex_descriptor v,
                           const PAG& g, const virtual_machine* vm)
                    : os_(os), v_(v), g_(g), vm_(vm)
            {
            }

            void operator()(const pag_reg& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\nin "
                        << escape(vm_->make_jvm_hdl(x.hdl.insn_hdl.method_hdl));
                }
                print_common_label();
                os_ << "color=green,";
                os_ << "URL=\"insn/" << x.hdl.insn_hdl.method_hdl << ".dot\",";
            }

            void operator()(const pag_alloc& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\nin "
                        << escape(vm_->make_jvm_hdl(x.hdl.method_hdl));
                }
                print_common_label();
                os_ << "color=blue,";
                os_ << "URL=\"insn/" << x.hdl.method_hdl << ".dot\",";
            }

            void operator()(const pag_reg_dot_field& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\n" << escape(vm_->make_jvm_hdl(x.field_hdl));
                    os_ << "\\nin " << escape(vm_->make_jvm_hdl(
                                               x.reg_hdl.insn_hdl.method_hdl));
                }
                print_common_label();
                os_ << "color=red,";
                os_ << "URL=\"insn/" << x.reg_hdl.insn_hdl.method_hdl
                    << ".dot\",";
            }

            void operator()(const pag_alloc_dot_field& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\n" << escape(vm_->make_jvm_hdl(x.field_hdl));
                }
                print_common_label();
                os_ << "color=yellow,";
            }

            void operator()(const pag_static_field& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\n" << escape(vm_->make_jvm_hdl(x.hdl));
                }
                print_common_label();
                os_ << "color=orange,";
            }

            void operator()(const pag_reg_dot_array& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                if (vm_) {
                    os_ << "\\nin "
                        << escape(vm_->make_jvm_hdl(x.hdl.insn_hdl.method_hdl));
                }
                print_common_label();
                os_ << "color=black,";
                os_ << "URL=\"insn/" << x.hdl.insn_hdl.method_hdl << ".dot\",";
            }

            void operator()(const pag_alloc_dot_array& x) const
            {
                os_ << "label=\"{";
                os_ << escape(x);
                print_common_label();
                os_ << "color=gray,";
            }

        private:
            template <typename T>
            std::string escape(const T& x) const
            {
                std::stringstream ss;
                ss << x;
                return std::regex_replace(ss.str(), std::regex("<|>"), "\\$&");
            }

            void print_common_label() const
            {
                if (g_[v_].context.idx != 0) {
                    os_ << "|Ctx: " << g_[v_].context;
                    if (vm_) {
                        os_ << "\\l= "
                            << vm_->make_jvm_hdl(g_[v_].context.method_hdl)
                            << "\\l";
                    }
                }
                if (g_[v_].type) {
                    os_ << "|Type: " << *g_[v_].type;
                    if (vm_) {
                        os_ << "\\l= " << vm_->make_jvm_hdl(*g_[v_].type)
                            << "\\l";
                    }
                }
                if (!g_[g_[v_].parent].points_to_set.empty()) {
                    os_ << "|[";
                    for (auto x : g_[g_[v_].parent].points_to_set) {
                        os_ << " " << x;
                    }
                    os_ << " ]";
                }
                if (!g_[v_].virtual_invoke_insns.empty()) {
                    os_ << "|";
                    for (const auto& ih : g_[v_].virtual_invoke_insns) {
                        os_ << ih.first << ":" << ih.second << "()\\n";
                    }
                }
                os_ << "}\"\n";
                os_ << ",";
            }

        private:
            std::ostream& os_;
            pag_vertex_descriptor v_;
            const PAG& g_;
            const virtual_machine* vm_;
        };
    }

    template <typename PAG>
    inline void
    write_graphviz_pointer_assignment_graph(std::ostream& os, const PAG& g,
                                            const virtual_machine* vm = nullptr)
    {
        auto vprop_writer = [&](std::ostream& os, const auto& v) {
#if 1
            os << "[";
            boost::apply_visitor(detail::vertex_printer<PAG>(os, v, g, vm),
                                 g[v].vertex);
            os << "shape=record";
            os << "]";
#else
            os << "[";
            boost::apply_visitor(vertex_printer(os), g[v].vertex);
            os << "shape=point";
            os << "]";
#endif
        };

        auto eprop_writer = [&](std::ostream& os, const auto& e) {
            os << "[";
            switch (g[e].kind) {
            case pag_edge_property::kind_alloc:
                os << "color=blue";
                break;
            case pag_edge_property::kind_assign:
                os << "color=green";
                break;
            case pag_edge_property::kind_istore:
                os << "color=red";
                break;
            case pag_edge_property::kind_iload:
                os << "color=red";
                break;
            case pag_edge_property::kind_sstore:
                os << "color=orange";
                break;
            case pag_edge_property::kind_sload:
                os << "color=orange";
                break;
            case pag_edge_property::kind_astore:
                break;
            case pag_edge_property::kind_aload:
                break;
            }
            os << "]";
        };

        write_graphviz(os, g, vprop_writer, eprop_writer);
    }
}

#endif
