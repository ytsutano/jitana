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

#ifndef JITANA_DEX_FILE_HPP
#define JITANA_DEX_FILE_HPP

#include "jitana/vm_core/virtual_machine.hpp"
#include "jitana/vm_core/dex_raw_types.hpp"
#include "jitana/util/stream_reader.hpp"

#include <string>
#include <memory>
#include <cassert>
#include <unordered_map>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/optional.hpp>
#include <boost/range.hpp>

namespace jitana {
    class dex_ids {
    public:
        friend class dex_file;

        const dex_string_id& operator[](const dex_string_idx& idx) const
        {
            assert(idx.valid());
            return string_ids_[idx.value];
        }

        const dex_type_id& operator[](const dex_type_idx& idx) const
        {
            assert(idx.valid());
            return type_ids_[idx.value];
        }

        const dex_proto_id& operator[](const dex_proto_idx& idx) const
        {
            assert(idx.valid());
            return proto_ids_[idx.value];
        }

        const dex_field_id& operator[](const dex_field_idx& idx) const
        {
            assert(idx.valid());
            return field_ids_[idx.value];
        }

        const dex_method_id& operator[](const dex_method_idx& idx) const
        {
            assert(idx.valid());
            return method_ids_[idx.value];
        }

        const char* c_str(const dex_string_idx& idx) const
        {
            reader_.move_head((*this)[idx].string_data_off());
            reader_.get_uleb128p1();
            return reader_.get_c_str();
        }

        const char* descriptor(const dex_type_idx& idx) const
        {
            return c_str((*this)[idx].descriptor_idx());
        }

        std::string descriptor(const dex_proto_idx& idx) const
        {
            std::string desc;
            desc += '(';
            for (const auto& d : param_descriptors(idx)) {
                desc += d;
            }
            desc += ')';
            desc += return_type(idx);
            return desc;
        }

        std::vector<const char*>
        param_descriptors(const dex_proto_idx& idx) const
        {
            std::vector<const char*> descriptors;
            const auto poff = (*this)[idx].parameters_off();
            if (poff > 0) {
                auto r = reader_;
                r.move_head(poff);
                descriptors.resize(r.get<uint32_t>());
                for (auto& desc : descriptors) {
                    const dex_type_idx type_idx = r.get<uint16_t>();
                    desc = descriptor(type_idx);
                }
            }
            return descriptors;
        }

        std::string unique_name(const dex_method_idx& idx) const
        {
            return name(idx) + proto_descriptor(idx);
        }

        std::string class_descriptor(const dex_method_idx& idx) const
        {
            return descriptor((*this)[idx].class_idx());
        }

        std::string proto_descriptor(const dex_method_idx& idx) const
        {
            return descriptor((*this)[idx].proto_idx());
        }

        std::vector<const char*>
        param_descriptors(const dex_method_idx& idx) const
        {
            return param_descriptors((*this)[idx].proto_idx());
        }

        std::string class_descriptor(const dex_field_idx& idx) const
        {
            return descriptor((*this)[idx].class_idx());
        }

        std::string descriptor(const dex_field_idx& idx) const
        {
            return descriptor((*this)[idx].type_idx());
        }

        const char* return_type(const dex_proto_idx& idx) const
        {
            return descriptor((*this)[idx].return_type_idx());
        }

        const char* return_type(const dex_method_idx& idx) const
        {
            return return_type((*this)[idx].proto_idx());
        }

        const char* shorty(const dex_proto_idx& idx) const
        {
            return c_str((*this)[idx].shorty_idx());
        }

        const char* shorty(const dex_method_idx& idx) const
        {
            return shorty((*this)[idx].proto_idx());
        }

        const char* name(const dex_method_idx& idx) const
        {
            return c_str((*this)[idx].name_idx());
        }

        const char* name(const dex_field_idx& idx) const
        {
            return c_str((*this)[idx].name_idx());
        }

    private:
        boost::iterator_range<const dex_string_id*> string_ids_;
        boost::iterator_range<const dex_type_id*> type_ids_;
        boost::iterator_range<const dex_proto_id*> proto_ids_;
        boost::iterator_range<const dex_field_id*> field_ids_;
        boost::iterator_range<const dex_method_id*> method_ids_;
        stream_reader reader_;
    };

    class dex_file {
    public:
        /// Creates a DexFile instance from a memory-mapped file.
        explicit dex_file(dex_file_hdl hdl, std::string filename,
                          const uint8_t* file_begin);

        /// Creates a DexFile instance by opening a file.
        explicit dex_file(dex_file_hdl hdl, std::string filename);

        dex_file(dex_file&& x) = default;

        dex_file(const dex_file&) = default;

        const dex_file_hdl& hdl() const
        {
            return hdl_;
        }

        const std::string& filename() const
        {
            return file_->name;
        }

        const dex_ids& ids() const
        {
            return ids_;
        }

        boost::optional<class_vertex_descriptor>
        load_class(virtual_machine& vm, const std::string& descriptor) const;

        boost::optional<std::pair<dex_method_hdl, uint32_t>>
        find_method_hdl(uint32_t dex_off) const;

        bool load_all_classes(virtual_machine& vm) const;

    private:
        void load_dex_file();

        insn_graph make_insn_graph(method_vertex_property& mvprop,
                                   uint32_t code_off,
                                   const dex_method_hdl& dex_m_hdl,
                                   const jvm_method_hdl& jvm_m_hdl) const;
        void parse_debug_info(insn_graph& g, method_vertex_property& mvprop,
                              uint32_t debug_info_off) const;

        struct mapped_file {
            boost::iostreams::mapped_file_source file;
            const uint8_t* begin;
            size_t length;
            std::string name;
        };

        const dex_file_hdl hdl_;
        std::shared_ptr<mapped_file> file_;
        const uint8_t* dex_begin_;
        const detail::dex_opt_header* opt_header_;
        const detail::dex_header* header_;
        dex_ids ids_;
        boost::iterator_range<const detail::dex_class_def*> class_defs_;
        std::unordered_map<std::string, const detail::dex_class_def*>
                class_def_lut_;
        std::vector<std::pair<uint32_t, dex_method_idx>> code_off_lut_;
    };
}

#endif
