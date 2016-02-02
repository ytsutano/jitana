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

#ifndef JITANA_VIRTUAL_MACHINE_HPP
#define JITANA_VIRTUAL_MACHINE_HPP

#include "jitana/vm_graph/loader_graph.hpp"
#include "jitana/vm_graph/class_graph.hpp"
#include "jitana/vm_graph/method_graph.hpp"
#include "jitana/vm_graph/field_graph.hpp"

#include <unordered_set>
#include <utility>

#include <boost/optional.hpp>

namespace jitana {
    class class_loader;
}

namespace jitana {
    /// A graph-based virtual machine.
    ///
    /// Every non-trivial virtual machine object is represented as a vertex on
    /// a graph. Most of the relationships between the virtual machine objects
    /// are represented as edges instead of pointer member variables. This
    /// allows us to analyze the virtual machine state using the generic graph
    /// algorithms.
    ///
    /// The vertices of the graphs are created by jitana::virtual_machine and
    /// its associated classes only. The edges may be created, modified, or
    /// deleted by external analysis classes.
    class virtual_machine {
    public:
        /// Adds the class loader to the virtual machine.
        loader_vertex_descriptor add_loader(class_loader loader);

        /// Adds the class loader to the virtual machine with the specified
        /// parent class loader.
        loader_vertex_descriptor add_loader(class_loader loader,
                                            const class_loader_hdl& parent_hdl);

        /// Finds the class vertex that corresponds to the initializing JVM
        /// type handle.
        boost::optional<class_vertex_descriptor>
        find_class(const jvm_type_hdl& hdl, bool try_load);

        /// Finds the class vertex that corresponds to the initializing DEX
        /// type handle.
        boost::optional<class_vertex_descriptor>
        find_class(const dex_type_hdl& hdl, bool try_load);

        /// Finds the method vertex that corresponds to the initializing JVM
        /// method handle.
        boost::optional<method_vertex_descriptor>
        find_method(const jvm_method_hdl& hdl, bool try_load);

        /// Finds the method vertex that corresponds to the initializing DEX
        /// method handle.
        boost::optional<method_vertex_descriptor>
        find_method(const dex_method_hdl& hdl, bool try_load);

        /// Finds the field vertex that corresponds to the initializing JVM
        /// field handle.
        boost::optional<field_vertex_descriptor>
        find_field(const jvm_field_hdl& hdl, bool try_load);

        /// Finds the field vertex that corresponds to the initializing DEX
        /// field handle.
        boost::optional<field_vertex_descriptor>
        find_field(const dex_field_hdl& hdl, bool try_load);

        /// Finds the instruction vertex that corresponds to the DEX file
        /// offset.
        boost::optional<std::pair<method_vertex_descriptor,
                                  insn_vertex_descriptor>>
        find_insn(const dex_file_hdl& file_hdl, uint32_t offset, bool try_load);

        std::unordered_set<method_vertex_descriptor>
        load_recursive(method_vertex_descriptor v);

        /// Loads all the classes in the specified class loader.
        bool load_all_classes(const class_loader_hdl& loader_hdl);

        const loader_graph& loaders() const
        {
            return loaders_;
        }

        loader_graph& loaders()
        {
            return loaders_;
        }

        const class_graph& classes() const
        {
            return classes_;
        }

        class_graph& classes()
        {
            return classes_;
        }

        const method_graph& methods() const
        {
            return methods_;
        }

        method_graph& methods()
        {
            return methods_;
        }

        const field_graph& fields() const
        {
            return fields_;
        }

        field_graph& fields()
        {
            return fields_;
        }

        /// Returns the JVM type handle from the DEX type handle.
        jvm_type_hdl make_jvm_hdl(const dex_type_hdl& type_hdl) const;

        /// Returns the JVM method handle from the DEX method handle.
        jvm_method_hdl make_jvm_hdl(const dex_method_hdl& method_hdl) const;

        /// Returns the JVM field handle from the DEX field handle.
        jvm_field_hdl make_jvm_hdl(const dex_field_hdl& field_hdl) const;

    private:
        loader_graph loaders_;
        class_graph classes_;
        method_graph methods_;
        field_graph fields_;
    };
}

#endif
