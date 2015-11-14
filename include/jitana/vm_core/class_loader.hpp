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

#ifndef JITANA_CLASS_LOADER_HPP
#define JITANA_CLASS_LOADER_HPP

#include "jitana/vm_core/hdl.hpp"
#include "jitana/vm_graph/class_graph.hpp"
#include "jitana/vm_graph/method_graph.hpp"
#include "jitana/vm_graph/field_graph.hpp"

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace jitana {
    class virtual_machine;
    class dex_file;
}

namespace jitana {
    class class_loader {
    public:
        class_loader()
        {
        }

        template <typename I>
        class_loader(const class_loader_hdl& hdl, std::string name,
                     I filenames_first, I filenames_last)
                : hdl_(hdl),
                  name_(std::move(name)),
                  impl_(std::make_shared<impl>(hdl, filenames_first,
                                               filenames_last))
        {
        }

        void add_file(const std::string& filename);

        const class_loader_hdl& hdl() const
        {
            return hdl_;
        }

        const std::string& name() const
        {
            return name_;
        }

        const std::vector<dex_file>& dex_files() const
        {
            return impl_->dex_files;
        }

        /// Loads a class with the specified descriptor.
        boost::optional<class_vertex_descriptor>
        load_class(virtual_machine& vm, const std::string& descriptor) const;

        boost::optional<class_vertex_descriptor>
        lookup_class(virtual_machine& vm, const std::string& descriptor) const;

        boost::optional<method_vertex_descriptor>
        lookup_method(virtual_machine& vm, const std::string& descriptor,
                      const std::string& unique_name) const;

        boost::optional<field_vertex_descriptor>
        lookup_field(virtual_machine& vm, const std::string& descriptor,
                     const std::string& name) const;

        bool load_all_classes(virtual_machine& vm) const;

        std::string descriptor(const dex_type_hdl& hdl) const;

        std::string class_descriptor(const dex_method_hdl& hdl) const;

        std::string unique_name(const dex_method_hdl& hdl) const;

        std::string class_descriptor(const dex_field_hdl& hdl) const;

        std::string name(const dex_field_hdl& hdl) const;

    private:
        class_loader_hdl hdl_;
        std::string name_;
        struct impl {
            std::vector<dex_file> dex_files;

            template <typename I>
            impl(class_loader_hdl hdl, I filenames_first, I filenames_last)
            {
                dex_file_hdl file_hdl;
                file_hdl.loader_hdl = hdl;
                std::for_each(filenames_first, filenames_last,
                              [&](const auto& s) {
                                  file_hdl.idx = dex_files.size();
                                  dex_files.emplace_back(file_hdl, s);
                              });
            }
        };
        std::shared_ptr<impl> impl_;
    };
}

#endif
