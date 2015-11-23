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

#include "jitana/vm_core/dex_file.hpp"
#include "jitana/vm_core/insn_info.hpp"

#include <cassert>

#include <boost/range/iterator_range.hpp>

using namespace jitana;
using namespace jitana::detail;

dex_file::dex_file(dex_file_hdl hdl, std::string filename,
                   const uint8_t* file_begin)
        : hdl_(std::move(hdl)), file_(std::make_shared<mapped_file>())
{
    file_->name = std::move(filename);

    file_->begin = file_begin;
    file_->length = 0;

    load_dex_file();
}

dex_file::dex_file(dex_file_hdl hdl, std::string filename)
        : hdl_(std::move(hdl)), file_(std::make_shared<mapped_file>())
{
    file_->name = std::move(filename);
    file_->file.open(file_->name);

    // Get the file length.
    file_->length = file_->file.end() - file_->file.begin();

    // Get the file content.
    file_->begin = reinterpret_cast<const uint8_t*>(file_->file.data());

    load_dex_file();
}

void dex_file::load_dex_file()
{
    auto reader = stream_reader{file_->begin};

    // Check the DEX magic string.
    auto magic = reinterpret_cast<const char*>(file_->begin);
    if (std::strncmp(magic, "dex\n035", 8) == 0) {
        // Unoptimized DEX file.
        opt_header_ = nullptr;
        dex_begin_ = file_->begin;
    }
    else if (std::strncmp(magic, "dey\n036", 8) == 0) {
        // Optimized DEX file.
        opt_header_ = &reader.get<dex_opt_header>();
        dex_begin_ = file_->begin + opt_header_->dex_off;
    }
    else {
        std::stringstream ss;
        ss << "unsupported file type found in ";
        ss << file_->name;
        throw std::runtime_error(ss.str());
    }

    // Set the pointer to the DEX header.
    reader.set_base(dex_begin_);
    header_ = &reader.get<dex_header>();

    ids_.reader_ = reader;

    // Set the pointer to the string ID array.
    reader.move_head(header_->string_ids_off);
    auto string_ids_begin = &reader.get<dex_string_id>();
    ids_.string_ids_ = boost::make_iterator_range(
            string_ids_begin, string_ids_begin + header_->string_ids_size);

    // Set the pointer to the type ID array.
    reader.move_head(header_->type_ids_off);
    auto type_ids_begin = &reader.get<dex_type_id>();
    ids_.type_ids_ = boost::make_iterator_range(
            type_ids_begin, type_ids_begin + header_->type_ids_size);

    // Set the pointer to the prototype ID array.
    reader.move_head(header_->proto_ids_off);
    auto proto_ids_begin = &reader.get<dex_proto_id>();
    ids_.proto_ids_ = boost::make_iterator_range(
            proto_ids_begin, proto_ids_begin + header_->proto_ids_size);

    // Set the pointer to the field ID array.
    reader.move_head(header_->field_ids_off);
    auto field_ids_begin = &reader.get<dex_field_id>();
    ids_.field_ids_ = boost::make_iterator_range(
            field_ids_begin, field_ids_begin + header_->field_ids_size);

    // Set the pointer to the method ID array.
    reader.move_head(header_->method_ids_off);
    auto method_ids_begin = &reader.get<dex_method_id>();
    ids_.method_ids_ = boost::make_iterator_range(
            method_ids_begin, method_ids_begin + header_->method_ids_size);

    // Set the pointer to the class definition array.
    reader.move_head(header_->class_defs_off);
    auto class_defs_begin = &reader.get<dex_class_def>();
    class_defs_ = boost::make_iterator_range(
            class_defs_begin, class_defs_begin + header_->class_defs_size);

    // Compute the lookup tables.
    for (const auto& def : class_defs_) {
        // Update the class definition lookup table.
        class_def_lut_[ids_.descriptor(def.class_idx())] = &def;

        // Update the code offset lookup table.
        if (def.class_data_off() != 0) {
            reader.move_head(def.class_data_off());

            const auto& static_fields_size = reader.get_uleb128();
            const auto& instance_fields_size = reader.get_uleb128();
            const auto& direct_methods_size = reader.get_uleb128();
            const auto& virtual_methods_size = reader.get_uleb128();

            // Ignore static fields.
            auto static_field_idx = dex_field_idx{0};
            for (size_t i = 0; i < static_fields_size; ++i) {
                static_field_idx += reader.get_uleb128();
                reader.get_uleb128();
            }

            // Ignore instance fields.
            auto instance_field_idx = dex_field_idx{0};
            for (size_t i = 0; i < instance_fields_size; ++i) {
                instance_field_idx += reader.get_uleb128();
                reader.get_uleb128();
            }

            auto direct_method_idx = dex_method_idx{0};
            for (size_t i = 0; i < direct_methods_size; ++i) {
                direct_method_idx += reader.get_uleb128();
                reader.get_uleb128();
                auto code_off = reader.get_uleb128();
                if (code_off != 0) {
                    code_off_lut_.emplace_back(code_off, direct_method_idx);
                }
            }

            auto virtual_method_idx = dex_method_idx{0};
            for (size_t i = 0; i < virtual_methods_size; ++i) {
                virtual_method_idx += reader.get_uleb128();
                reader.get_uleb128();
                auto code_off = reader.get_uleb128();
                if (code_off != 0) {
                    code_off_lut_.emplace_back(code_off, virtual_method_idx);
                }
            }

            std::sort(begin(code_off_lut_), end(code_off_lut_));
        }
    }
}

boost::optional<class_vertex_descriptor>
dex_file::load_class(virtual_machine& vm, const std::string& descriptor) const
{
    // Find the class definition from the lookup table.
    auto def_it = class_def_lut_.find(descriptor);
    if (def_it == end(class_def_lut_)) {
        return boost::none;
    }
    const auto& def = *def_it->second;

    // Load the super class.
    boost::optional<class_vertex_descriptor> super_v;
    if (def.superclass_idx().valid()) {
        auto super_hdl = dex_type_hdl{
                hdl_, static_cast<uint16_t>(def.superclass_idx())};
        super_v = vm.find_class(super_hdl, true);
        if (!super_v) {
            return boost::none;
        }
    }

    // Load the interfaces.
    std::vector<class_vertex_descriptor> interface_v_list;
    if (def.interfaces_off() != 0) {
        auto reader = stream_reader{dex_begin_};
        reader.move_head(def.interfaces_off());

        const auto& size = reader.get<uint32_t>();
        for (unsigned i = 0; i < size; ++i) {
            auto interface_hdl = dex_type_hdl{hdl_, reader.get<uint16_t>()};
            auto interface_v = vm.find_class(interface_hdl, true);
            if (!interface_v) {
                return boost::none;
            }
            interface_v_list.push_back(*interface_v);
        }
    }

    // Create the handles for this class.
    auto dex_hdl = dex_type_hdl{hdl_, static_cast<uint16_t>(def.class_idx())};
    auto jvm_hdl = jvm_type_hdl{hdl_.loader_hdl, descriptor};

    // Create field tables.
    auto static_fields = std::vector<dex_field_hdl>{};
    auto instance_fields = std::vector<dex_field_hdl>{};

    // Create method tables.
    auto dtable = std::vector<dex_method_hdl>{}; // For direct methods.
    auto vtable = std::vector<dex_method_hdl>{}; // For virtual methods (vtbl).

    uint16_t static_offset = 0;
    uint16_t instance_offset = 0;

    // Load the class data.
    if (def.class_data_off() != 0) {
        auto reader = stream_reader{dex_begin_};
        reader.move_head(def.class_data_off());

        auto& fg = vm.fields();
        auto& mg = vm.methods();

        const auto& static_fields_size = reader.get_uleb128();
        const auto& instance_fields_size = reader.get_uleb128();
        const auto& direct_methods_size = reader.get_uleb128();
        const auto& virtual_methods_size = reader.get_uleb128();

        auto jvm_sizeof = [](char c) -> uint8_t {
            switch (c) {
            case 'V': // void
            case 'B': // byte
            case 'Z': // boolean
                return 1;
            case 'S': // short
            case 'C': // char
                return 2;
            case 'J': // long
            case 'D': // double
                return 8;
            default:
                return 4;
            }
        };

        // Create static fields.
        {
            if (super_v) {
                // Inherit the static_fields from the superclass.
                static_fields = vm.classes()[*super_v].static_fields;
                static_offset = vm.classes()[*super_v].static_size;

                // Register the JVM handles.
                for (const auto& field_hdl : static_fields) {
                    auto fv = lookup_field_vertex(field_hdl, fg);
                    auto jvm_f_hdl = fg[*fv].jvm_hdl;
                    jvm_f_hdl.type_hdl = jvm_hdl;
                    fg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = *fv;
                }
            }

            auto static_field_idx = dex_field_idx{0};
            for (size_t i = 0; i < static_fields_size; ++i) {
                static_field_idx += reader.get_uleb128();
                auto access_flags = make_dex_access_flags(reader.get_uleb128());
                const auto& name = ids_.name(static_field_idx);
                char type_char = ids_.descriptor(static_field_idx)[0];
                auto size = jvm_sizeof(type_char);

                // Create a field vertex.
                auto dex_f_hdl = dex_field_hdl{
                        hdl_, static_cast<uint16_t>(static_field_idx)};
                auto jvm_f_hdl = jvm_field_hdl{jvm_hdl, name};
                field_vertex_property fvprop;
                fvprop.kind = field_vertex_property::static_field;
                fvprop.hdl = dex_f_hdl;
                fvprop.jvm_hdl = jvm_f_hdl;
                fvprop.class_hdl = dex_hdl;
                fvprop.access_flags = access_flags;
                fvprop.offset = static_offset;
                fvprop.size = size;
                fvprop.type_char = type_char;
                auto fv = add_vertex(fvprop, fg);
                fg[boost::graph_bundle].hdl_to_vertex[dex_f_hdl] = fv;
                fg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = fv;

                static_fields.push_back(dex_f_hdl);

                static_offset += size;
            }
        }

        // Create instance fields.
        {
            if (super_v) {
                // Inherit the instance_fields from the superclass.
                instance_fields = vm.classes()[*super_v].instance_fields;
                instance_offset = vm.classes()[*super_v].instance_size;

                // Register the JVM handles.
                for (const auto& field_hdl : instance_fields) {
                    auto fv = lookup_field_vertex(field_hdl, fg);
                    auto jvm_f_hdl = fg[*fv].jvm_hdl;
                    jvm_f_hdl.type_hdl = jvm_hdl;
                    fg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = *fv;
                }
            }

            auto instance_field_idx = dex_field_idx{0};
            for (size_t i = 0; i < instance_fields_size; ++i) {
                instance_field_idx += reader.get_uleb128();
                auto access_flags = make_dex_access_flags(reader.get_uleb128());
                const auto& name = ids_.name(instance_field_idx);
                char type_char = ids_.descriptor(instance_field_idx)[0];
                auto size = jvm_sizeof(type_char);

                // Create a field vertex.
                auto dex_f_hdl = dex_field_hdl{
                        hdl_, static_cast<uint16_t>(instance_field_idx)};
                auto jvm_f_hdl = jvm_field_hdl{jvm_hdl, name};
                field_vertex_property fvprop;
                fvprop.kind = field_vertex_property::instance_field;
                fvprop.hdl = dex_f_hdl;
                fvprop.jvm_hdl = jvm_f_hdl;
                fvprop.class_hdl = dex_hdl;
                fvprop.access_flags = access_flags;
                fvprop.offset = instance_offset;
                fvprop.size = size;
                fvprop.type_char = type_char;
                auto fv = add_vertex(fvprop, fg);
                fg[boost::graph_bundle].hdl_to_vertex[dex_f_hdl] = fv;
                fg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = fv;

                instance_fields.push_back(dex_f_hdl);

                instance_offset += size;
            }
        }

        // Create direct methods.
        {
            if (super_v) {
                // Inherit the dtable from the superclass.
                const auto& super_dtable = vm.classes()[*super_v].dtable;
                dtable = super_dtable;
                dtable.reserve(super_dtable.size() + virtual_methods_size);

                // Register the JVM handles.
                for (const auto& field_hdl : dtable) {
                    auto mv = lookup_method_vertex(field_hdl, mg);
                    auto jvm_f_hdl = mg[*mv].jvm_hdl;
                    jvm_f_hdl.type_hdl = jvm_hdl;
                    mg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = *mv;
                }
            }
            else {
                // No superclass: start with an empty dtable.
                dtable.reserve(virtual_methods_size);
            }

            auto direct_method_idx = dex_method_idx{0};
            for (size_t i = 0; i < direct_methods_size; ++i) {
                direct_method_idx += reader.get_uleb128();
                auto access_flags = make_dex_access_flags(reader.get_uleb128());
                auto code_off = reader.get_uleb128();
                const auto& unique_name = ids_.unique_name(direct_method_idx);
                const auto& param_descriptors
                        = ids_.param_descriptors(direct_method_idx);

                // Create a method vertex.
                auto dex_m_hdl = dex_method_hdl{
                        hdl_, static_cast<uint16_t>(direct_method_idx)};
                auto jvm_m_hdl = jvm_method_hdl{jvm_hdl, unique_name};
                method_vertex_property mvprop;
                mvprop.hdl = dex_m_hdl;
                mvprop.jvm_hdl = jvm_m_hdl;
                mvprop.class_hdl = dex_hdl;
                mvprop.access_flags = access_flags;
                mvprop.params.reserve(param_descriptors.size());
                for (const auto& d : param_descriptors) {
                    mvprop.params.emplace_back();
                    mvprop.params.back().descriptor = d;
                }
                auto mv = add_vertex(mvprop, mg);
                mg[boost::graph_bundle].hdl_to_vertex[dex_m_hdl] = mv;
                mg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_m_hdl] = mv;

                // Load the instructions.
                mg[mv].insns = make_insn_graph(mg[mv], code_off, dex_m_hdl,
                                               jvm_m_hdl);

                dtable.push_back(dex_m_hdl);
            }
        }

        // Create virtual methods.
        {
            if (super_v) {
                // Inherit the vtable from the superclass.
                const auto& super_vtable = vm.classes()[*super_v].vtable;
                vtable = super_vtable;
                vtable.reserve(super_vtable.size() + virtual_methods_size);

                // Register the JVM handles.
                for (const auto& field_hdl : vtable) {
                    auto mv = lookup_method_vertex(field_hdl, mg);
                    auto jvm_f_hdl = mg[*mv].jvm_hdl;
                    jvm_f_hdl.type_hdl = jvm_hdl;
                    mg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_f_hdl] = *mv;
                }
            }
            else {
                // No superclass: start with an empty vtable.
                vtable.reserve(virtual_methods_size);
            }

            const auto vtab_inherited_size = vtable.size();
            auto virtual_method_idx = dex_method_idx{0};
            for (size_t i = 0; i < virtual_methods_size; ++i) {
                virtual_method_idx += reader.get_uleb128();
                auto access_flags = make_dex_access_flags(reader.get_uleb128());
                auto code_off = reader.get_uleb128();
                const auto& unique_name = ids_.unique_name(virtual_method_idx);
                const auto& param_descriptors
                        = ids_.param_descriptors(virtual_method_idx);

                // Create a method vertex.
                auto dex_m_hdl = dex_method_hdl{
                        hdl_, static_cast<uint16_t>(virtual_method_idx)};
                auto jvm_m_hdl = jvm_method_hdl{jvm_hdl, unique_name};
                method_vertex_property mvprop;
                mvprop.hdl = dex_m_hdl;
                mvprop.jvm_hdl = jvm_m_hdl;
                mvprop.class_hdl = dex_hdl;
                mvprop.access_flags = access_flags;
                for (const auto& d : param_descriptors) {
                    mvprop.params.emplace_back();
                    mvprop.params.back().descriptor = d;
                }
                auto mv = add_vertex(mvprop, mg);
                mg[boost::graph_bundle].hdl_to_vertex[dex_m_hdl] = mv;
                mg[boost::graph_bundle].jvm_hdl_to_vertex[jvm_m_hdl] = mv;

                // Load the instructions.
                mg[mv].insns = make_insn_graph(mvprop, code_off, dex_m_hdl,
                                               jvm_m_hdl);

                const auto vtab_inherited_end
                        = begin(vtable) + vtab_inherited_size;
                auto it = std::find_if(begin(vtable), vtab_inherited_end,
                                       [&](const dex_method_hdl& h) {
                                           auto v = lookup_method_vertex(h, mg);
                                           const auto& n
                                                   = mg[*v].jvm_hdl.unique_name;
                                           return unique_name == n;
                                       });
                if (it != vtab_inherited_end) {
                    // Same entry found: override.
                    auto super_v = lookup_method_vertex(*it, mg);
                    if (!super_v) {
                        std::stringstream ss;
                        ss << "invalid DEX file: ";
                        ss << file_->name;
                        throw std::runtime_error(ss.str());
                    }
                    method_super_edge_property eprop;
                    eprop.interface = false; // FIXME.
                    add_edge(*super_v, mv, eprop, mg);
                }
                else {
                    // New entry.
                    vtable.push_back(dex_m_hdl);
                }
            }
        }
    }

    // Create a vertex in the class graph.
    class_vertex_property vprop;
    vprop.hdl = dex_hdl;
    vprop.jvm_hdl = jvm_hdl;
    vprop.access_flags = def.access_flags();
    vprop.instance_fields = instance_fields;
    vprop.static_fields = static_fields;
    vprop.dtable = dtable;
    vprop.vtable = vtable;
    vprop.static_size = static_offset;
    vprop.instance_size = instance_offset;
    auto v = add_vertex(vprop, vm.classes());
    vm.classes()[boost::graph_bundle].hdl_to_vertex[dex_hdl] = v;
    vm.classes()[boost::graph_bundle].jvm_hdl_to_vertex[jvm_hdl] = v;

    // Add an edge to the super class if it exists.
    if (super_v) {
        add_edge(*super_v, v, class_super_edge_property{false}, vm.classes());
    }

    // Add edges to interfaces.
    for (const auto& interface_v : interface_v_list) {
        add_edge(interface_v, v, class_super_edge_property{true}, vm.classes());
    }

    return v;
}

bool dex_file::load_all_classes(virtual_machine& vm) const
{
    bool loaded_all_classes = true;

    // Load all classes in this DEX file.
    for (const auto& cd : class_defs_) {
        if (!vm.find_class(dex_type_hdl(hdl_, cd.class_idx().value), true)) {
            loaded_all_classes = false;
        }
    }

    return loaded_all_classes;
}

boost::optional<std::pair<dex_method_hdl, uint32_t>>
dex_file::find_method_hdl(uint32_t dex_off) const
{
    auto it = std::upper_bound(
            begin(code_off_lut_), end(code_off_lut_), dex_off,
            [](uint32_t lhs, const auto& rhs) { return lhs < rhs.first; });
    if (it == begin(code_off_lut_)) {
        return boost::none;
    }
    --it;

    // Get the size of the instructions list, in 16-bit code units.
    auto insns_size
            = *reinterpret_cast<const uint32_t*>(dex_begin_ + it->first + 12);

    uint32_t insn_off = dex_off - it->first;
    if (insn_off < 16) {
        // The offset is too small, meaning that it points to the code header.
        return boost::none;
    }

    // Remove the header from the offset.
    insn_off -= 16;

    // Instruction offset is in 16-bit code unit instead of 8.
    insn_off >>= 1;

    if (insn_off >= insns_size) {
        // The offset is too big.
        return boost::none;
    }

    dex_method_hdl method_hdl;
    method_hdl.file_hdl = hdl_;
    method_hdl.idx = it->second.value;

    return std::make_pair(method_hdl, insn_off);
}

insn_graph dex_file::make_insn_graph(method_vertex_property& mvprop,
                                     uint32_t code_off,
                                     const dex_method_hdl& dex_m_hdl,
                                     const jvm_method_hdl& jvm_m_hdl) const
{
    insn_graph g;
    g[boost::graph_bundle].hdl = dex_m_hdl;
    g[boost::graph_bundle].jvm_hdl = jvm_m_hdl;
    if (code_off == 0) {
        g[boost::graph_bundle].registers_size = 0;
        g[boost::graph_bundle].ins_size = 0;
        g[boost::graph_bundle].outs_size = 0;
        g[boost::graph_bundle].insns_off = 0;
        return g;
    }

    auto reader = stream_reader{dex_begin_};
    reader.move_head(code_off);

    auto raw_header = &reader.get<dex_code_header>();
    auto raw_insns = &reader.peek<uint16_t>();
    {
        auto raw_insn_size_in_bytes = raw_header->insns_size;
        if (raw_insn_size_in_bytes & 0x01) {
            // Add padding.
            ++raw_insn_size_in_bytes;
        }
        raw_insn_size_in_bytes *= 2;
        reader.move_head_forward(raw_insn_size_in_bytes);
    }
    auto raw_tries = &reader.peek<dex_try_item>();
    reader.move_head_forward(sizeof(dex_try_item) * raw_header->tries_size);
    auto raw_encoded_catch_handler_list = &reader.peek<uint8_t>();

    g.m_vertices.reserve(raw_header->insns_size + 2);
    g[boost::graph_bundle].registers_size = raw_header->registers_size;
    g[boost::graph_bundle].ins_size = raw_header->ins_size;
    g[boost::graph_bundle].outs_size = raw_header->outs_size;
    g[boost::graph_bundle].insns_off = (code_off != 0) ? code_off + 16 : 0;

    // Create an entry instruction.
    auto entry_v = add_vertex(g);
    g[entry_v].off = std::numeric_limits<decltype(g[entry_v].off)>::min();
    {
        std::array<register_idx, 5> regs;
        regs.fill(register_idx::idx_unknown);
        if (raw_header->ins_size > 0) {
            regs.front() = raw_header->registers_size - raw_header->ins_size;
            regs.back() = raw_header->registers_size - 1;
        }
        g[entry_v].insn = insn_entry(opcode::op_nop, regs, {});
    }

    // Compute the offset and create vertices.
    for (auto off = uint32_t{0}; off < raw_header->insns_size;) {
        auto raw_insn = reinterpret_cast<const dex_raw_insn*>(&raw_insns[off]);
        if (raw_insn->op == opcode::op_nop) {
            switch (raw_insn->fmt_10x.byte1) {
            case 0x01:
                // packed-switch-payload.
                {
                    const auto& raw = raw_insn->fmt_packed_switch_payload;
                    off += (raw.size * 2) + 4;
                    continue;
                }
                break;
            case 0x02:
                // sparse-switch-payload.
                {
                    const auto& raw = raw_insn->fmt_sparse_switch_payload;
                    off += (raw.size * 4) + 2;
                    continue;
                }
                break;
            case 0x03:
                // fill-array-data-payload.
                {
                    const auto& raw = raw_insn->fmt_fill_array_data_payload;
                    off += (raw.size * raw.element_width + 1) / 2 + 4;
                    continue;
                }
                break;
            default:
                // Normal nop.
                {
                }
            }
        }

        auto v = add_vertex(g);
        g[boost::graph_bundle].offset_to_vertex[off] = v;
        g[v].off = off;
        off += info(raw_insn->op).size();
    }

    // Create an exit instruction.
    auto exit_v = add_vertex(g);
    g[exit_v].off = std::numeric_limits<decltype(g[exit_v].off)>::max();
    g[exit_v].insn = (jvm_m_hdl.return_descriptor()[0] != 'V')
            ? insn_exit(opcode::op_nop, {{register_idx::idx_result}}, {})
            : insn_exit(opcode::op_nop, {{register_idx::idx_unknown}}, {});

    // Parse the try-catch block.
    for (size_t i = 0; i < raw_header->tries_size; ++i) {
        auto tc_reader = stream_reader{raw_encoded_catch_handler_list
                                       + raw_tries[i].handler_off};
        try_catch_block tc;

        // Find the starting vertex.
        auto start_v = lookup_insn_vertex(raw_tries[i].start_addr, g);
        if (!start_v) {
            std::stringstream ss;
            ss << "invalid try-catch block (start_off) in ";
            ss << file_->name;
            throw std::runtime_error(ss.str());
        }
        tc.first = *start_v;

        // Find the last vertex.
        auto last_off = raw_tries[i].start_addr + raw_tries[i].insn_count - 1;
        for (;; --last_off) {
            auto last_v = lookup_insn_vertex(last_off, g);
            if (last_v) {
                tc.last = *last_v;
                break;
            }
        }

        auto catch_handler_size = tc_reader.get_sleb128();
        if (catch_handler_size <= 0) {
            catch_handler_size = -catch_handler_size;
            tc.has_catch_all = true;
        }
        else {
            tc.has_catch_all = false;
        }

        for (auto j = 0; j < catch_handler_size; ++j) {
            dex_type_hdl catch_type_hdl(g[boost::graph_bundle].hdl.file_hdl,
                                        tc_reader.get_uleb128());
            auto catch_off = tc_reader.get_uleb128();
            auto catch_v = lookup_insn_vertex(catch_off, g);
            if (!catch_v) {
                std::stringstream ss;
                ss << "invalid try-catch block (catch) in ";
                ss << file_->name;
                throw std::runtime_error(ss.str());
            }
            tc.hdls.emplace_back(catch_type_hdl, *catch_v);
        }

        if (tc.has_catch_all) {
            auto catch_off = tc_reader.get_uleb128();
            auto catch_v = lookup_insn_vertex(catch_off, g);
            if (!catch_v) {
                std::stringstream ss;
                ss << "invalid try-catch block (catch_all) in ";
                ss << file_->name;
                throw std::runtime_error(ss.str());
            }
            tc.catch_all = *catch_v;
        }

        g[boost::graph_bundle].try_catches.push_back(tc);
    }

    add_edge(entry_v, entry_v + 1, insn_control_flow_edge_property(), g);

    for (const auto& v : boost::make_iterator_range(vertices(g))) {
        auto& prop = g[v];

        if (is_pseudo(prop.insn)) {
            continue;
        }

        auto off = prop.off;
        auto raw_insn = reinterpret_cast<const dex_raw_insn*>(&raw_insns[off]);
        auto op = raw_insn->op;
        const auto& insn_info = info(op);

        switch (op) {
        case opcode::op_nop:
            // nop
            {
                prop.insn = insn_nop(op, {{}}, {});
            }
            break;
        case opcode::op_move:
        case opcode::op_move_wide:
        case opcode::op_move_object:
            // move        vA, vB
            // move-wide   vA, vB
            // move-object vA, vB
            {
                const auto& raw = raw_insn->fmt_12x;

                prop.insn = insn_move(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_move_from16:
        case opcode::op_move_wide_from16:
        case opcode::op_move_object_from16:
            // move/from16        vAA, vBBBB
            // move-wide/from16   vAA, vBBBB
            // move-object/from16 vAA, vBBBB
            {
                const auto& raw = raw_insn->fmt_22x;

                prop.insn = insn_move(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_move_16:
        case opcode::op_move_wide_16:
        case opcode::op_move_object_16:
            // move/16        vAAAA, vBBBB
            // move-wide/16   vAAAA, vBBBB
            // move-object/16 vAAAA, vBBBB
            {
                const auto& raw = raw_insn->fmt_32x;

                prop.insn = insn_move(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_move_result:
        case opcode::op_move_result_wide:
        case opcode::op_move_result_object:
            // move-result        vAA
            // move-result-wide   vAA
            // move-result-object vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_move(
                        op, {{raw.reg_a, register_idx::idx_result}}, {});
            }
            break;
        case opcode::op_move_exception:
            // move-exception vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_move(
                        op, {{raw.reg_a, register_idx::idx_exception}}, {});
            }
            break;
        case opcode::op_return_void:
        case opcode::op_return_void_barrier:
            //  return-void
            // +return-void-barrier
            {
                prop.insn = insn_return(op, {{register_idx::idx_unknown}}, {});

                add_edge(v, exit_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_return:
        case opcode::op_return_wide:
        case opcode::op_return_object:
            // return        vAA
            // return-wide   vAA
            // return-object vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_return(op, {{raw.reg_a}}, {});

                add_edge(v, exit_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_const_4:
            // const/4 vA, #+B
            {
                const auto& raw = raw_insn->fmt_11n;

                prop.insn = insn_const(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const_16:
            // const/16 vAA, #+BBBB
            {
                const auto& raw = raw_insn->fmt_21s;

                prop.insn = insn_const(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const:
            // const vAA, #+BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31i;

                prop.insn = insn_const(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const_high16:
            // const/high16 vAA, #+BBBB0000
            {
                const auto& raw = raw_insn->fmt_21h;
                const int32_t const_val = uint32_t(raw.imm_hi_b) << 16;

                prop.insn = insn_const(op, {{raw.reg_a}}, const_val);
            }
            break;
        case opcode::op_const_wide_16:
            // const-wide/16 vAA, #+BBBB
            {
                const auto& raw = raw_insn->fmt_21s;

                prop.insn = insn_const_wide(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const_wide_32:
            // const-wide/32 vAA, #+BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31i;

                prop.insn = insn_const_wide(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const_wide:
            // const-wide vAA, #+BBBBBBBBBBBBBBBB
            {
                const auto& raw = raw_insn->fmt_51l;

                prop.insn = insn_const_wide(op, {{raw.reg_a}}, raw.imm_b);
            }
            break;
        case opcode::op_const_wide_high16:
            // const-wide/high16 vAA, #+BBBB000000000000
            {
                const auto& raw = raw_insn->fmt_21h;
                const int64_t const_val = uint64_t(raw.imm_hi_b) << 48;

                prop.insn = insn_const_wide(op, {{raw.reg_a}}, const_val);
            }
            break;
        case opcode::op_const_string:
            // const-string vAA, string@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;
                // const char* const_val = "STRING";//find_string(raw.idx_b);
                const char* const_val = ids_.c_str(dex_string_idx(raw.idx_b));

                prop.insn = insn_const_string(op, {{raw.reg_a}}, const_val);
            }
            break;
        case opcode::op_const_string_jumbo:
            // const-string/jumbo vAA, string@BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31c;
                // const char* const_val = "STRING";//find_string(raw.idx_b);
                const char* const_val = ids_.c_str(dex_string_idx(raw.idx_b));

                prop.insn = insn_const_string(op, {{raw.reg_a}}, const_val);
            }
            break;
        case opcode::op_const_class:
            // const-class vAA, type@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;

                prop.insn = insn_const_class(op, {{raw.reg_a}},
                                             {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_monitor_enter:
            // monitor-enter vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_monitor_enter(op, {{raw.reg_a}}, {});
            }
            break;
        case opcode::op_monitor_exit:
            // monitor-exit  vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_monitor_exit(op, {{raw.reg_a}}, {});
            }
            break;
        case opcode::op_check_cast:
            // check-cast vAA, type@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;

                prop.insn = insn_check_cast(op, {{raw.reg_a}},
                                            {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_instance_of:
            // instance-of vA, vB, type@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_instance_of(op, {{raw.reg_a, raw.reg_b}},
                                             {hdl(), raw.idx_c});
            }
            break;
        case opcode::op_array_length:
            // array-length vA, vB
            {
                const auto& raw = raw_insn->fmt_12x;

                prop.insn = insn_array_length(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_new_instance:
            // new-instance vAA, type@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;

                prop.insn = insn_new_instance(op, {{raw.reg_a}},
                                              {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_new_array:
            // new-array vA, vB, type@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_new_array(op, {{raw.reg_a, raw.reg_b}},
                                           {hdl(), raw.idx_c});
            }
            break;
        case opcode::op_filled_new_array:
            // filled-new-array {vC, vD, vE, vF, vG}, type@BBBB
            {
                const auto& raw = raw_insn->fmt_35c;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = raw.reg_d;
                regs[2] = raw.reg_e;
                regs[3] = raw.reg_f;
                regs[4] = raw.reg_g;
                for (size_t i = raw.size_a; i < 5; ++i) {
                    regs[i] = register_idx::idx_unknown;
                }

                prop.insn = insn_filled_new_array(op, regs, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_filled_new_array_range:
            // filled-new-array/range {vCCCC .. vNNNN}, type@BBBB
            {
                const auto& raw = raw_insn->fmt_3rc;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = register_idx::idx_unknown;
                regs[2] = register_idx::idx_unknown;
                regs[3] = register_idx::idx_unknown;
                regs[4] = raw.reg_c + raw.size_a - 1;

                prop.insn = insn_filled_new_array(op, regs, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_fill_array_data:
            // fill-array-data vAA, +BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31t;

                // Read the payload instruction.
                const auto& raw_payload
                        = *reinterpret_cast<const dex_fmt_fill_array_data_payload*>(
                                &raw_insns[off + raw.roff_b]);
                array_payload payload;
                payload.element_width = raw_payload.element_width;
                payload.size = raw_payload.size;
                payload.data.insert(payload.data.begin(), raw_payload.data,
                                    raw_payload.data + raw_payload.size);

                prop.insn = insn_fill_array_data(op, {{raw.reg_a}}, payload);
            }
            break;
        case opcode::op_throw:
            // throw vAA
            {
                const auto& raw = raw_insn->fmt_11x;

                prop.insn = insn_throw(op, {{raw.reg_a}}, {});

                // TODO: handle exceptions correctly.
                // add_edge(v, exit_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_goto:
            // goto +AA
            {
                const auto& raw = raw_insn->fmt_10t;

                prop.insn = insn_goto(op, {{}}, {raw.roff_a});

                const auto target_v = lookup_insn_vertex(off + raw.roff_a, g);
                add_edge(v, *target_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_goto_16:
            // goto/16 +AAAA
            {
                const auto& raw = raw_insn->fmt_20t;

                prop.insn = insn_goto(op, {{}}, {raw.roff_a});

                const auto target_v = lookup_insn_vertex(off + raw.roff_a, g);
                add_edge(v, *target_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_goto_32:
            // goto/32 +AAAAAAAA
            {
                const auto& raw = raw_insn->fmt_30t;

                prop.insn = insn_goto(op, {{}}, {raw.roff_a});

                const auto target_v = lookup_insn_vertex(off + raw.roff_a, g);
                add_edge(v, *target_v, insn_control_flow_edge_property(), g);
            }
            break;
        case opcode::op_packed_switch:
            // packed-switch vAA, +BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31t;

                // Read the payload instruction.
                const auto& raw_payload
                        = *reinterpret_cast<const dex_fmt_packed_switch_payload*>(
                                &raw_insns[off + raw.roff_b]);
                for (int i = 0; i < raw_payload.size; ++i) {
                    if (raw_payload.targets[i] != insn_info.size()) {
                        const auto target_off = off + raw_payload.targets[i];
                        const auto target_v = lookup_insn_vertex(target_off, g);
                        add_edge(v, *target_v,
                                 insn_control_flow_edge_property(
                                         raw_payload.first_key + i),
                                 g);
                    }
                }

                prop.insn = insn_switch(op, {{raw.reg_a}}, {});
            }
            break;
        case opcode::op_sparse_switch:
            // sparse-switch vAA, +BBBBBBBB
            {
                const auto& raw = raw_insn->fmt_31t;

                // Read the payload instruction.
                const auto& raw_payload
                        = *reinterpret_cast<const dex_fmt_sparse_switch_payload*>(
                                &raw_insns[off + raw.roff_b]);
                const int32_t* keys = raw_payload.keys_targets;
                const int32_t* targets = keys + raw_payload.size;
                for (int i = 0; i < raw_payload.size; ++i) {
                    if (targets[i] != insn_info.size()) {
                        const auto target_off = off + targets[i];
                        const auto target_v = lookup_insn_vertex(target_off, g);
                        add_edge(v, *target_v,
                                 insn_control_flow_edge_property(keys[i]), g);
                    }
                }

                prop.insn = insn_switch(op, {{raw.reg_a}}, {});
            }
            break;
        case opcode::op_cmpl_float:
        case opcode::op_cmpg_float:
        case opcode::op_cmpl_double:
        case opcode::op_cmpg_double:
        case opcode::op_cmp_long:
            // cmpl-float  vAA, vBB, vCC
            // cmpg-float  vAA, vBB, vCC
            // cmpl-double vAA, vBB, vCC
            // cmpg-double vAA, vBB, vCC
            // cmp-long    vAA, vBB, vCC
            {
                const auto& raw = raw_insn->fmt_23x;

                prop.insn
                        = insn_cmp(op, {{raw.reg_a, raw.reg_b, raw.reg_c}}, {});
            }
            break;
        case opcode::op_if_eq:
        case opcode::op_if_ne:
        case opcode::op_if_lt:
        case opcode::op_if_ge:
        case opcode::op_if_gt:
        case opcode::op_if_le:
            // if-eq vA, vB, +CCCC
            // if-ne vA, vB, +CCCC
            // if-lt vA, vB, +CCCC
            // if-ge vA, vB, +CCCC
            // if-gt vA, vB, +CCCC
            // if-le vA, vB, +CCCC
            {
                const auto& raw = raw_insn->fmt_22t;
                // insn_idx target_idx = find_idx(off + raw.roff_c);

                prop.insn = insn_if(op, {{raw.reg_a, raw.reg_b}}, {});

                const auto target_off = off + raw.roff_c;
                const auto target_v = lookup_insn_vertex(target_off, g);
                add_edge(v, *target_v, insn_control_flow_edge_property(true),
                         g);
            }
            break;
        case opcode::op_if_eqz:
        case opcode::op_if_nez:
        case opcode::op_if_ltz:
        case opcode::op_if_gez:
        case opcode::op_if_gtz:
        case opcode::op_if_lez:
            // if-eqz vAA, +BBBB
            // if-nez vAA, +BBBB
            // if-ltz vAA, +BBBB
            // if-gez vAA, +BBBB
            // if-gtz vAA, +BBBB
            // if-lez vAA, +BBBB
            {
                const auto& raw = raw_insn->fmt_21t;
                // insn_idx target_idx = find_idx(off + raw.roff_b);

                prop.insn = insn_if_z(op, {{raw.reg_a}}, {});

                const auto target_off = off + raw.roff_b;
                const auto target_v = lookup_insn_vertex(target_off, g);
                add_edge(v, *target_v, insn_control_flow_edge_property(true),
                         g);
            }
            break;
        case opcode::op_aget:
        case opcode::op_aget_wide:
        case opcode::op_aget_object:
        case opcode::op_aget_boolean:
        case opcode::op_aget_byte:
        case opcode::op_aget_char:
        case opcode::op_aget_short:
            // aget         vAA, vBB, vCC
            // aget-wide    vAA, vBB, vCC
            // aget-object  vAA, vBB, vCC
            // aget-boolean vAA, vBB, vCC
            // aget-byte    vAA, vBB, vCC
            // aget-char    vAA, vBB, vCC
            // aget-short   vAA, vBB, vCC
            {
                const auto& raw = raw_insn->fmt_23x;

                prop.insn = insn_aget(op, {{raw.reg_a, raw.reg_b, raw.reg_c}},
                                      {});
            }
            break;
        case opcode::op_aput:
        case opcode::op_aput_wide:
        case opcode::op_aput_object:
        case opcode::op_aput_boolean:
        case opcode::op_aput_byte:
        case opcode::op_aput_char:
        case opcode::op_aput_short:
            // aput         vAA, vBB, vCC
            // aput-wide    vAA, vBB, vCC
            // aput-object  vAA, vBB, vCC
            // aput-boolean vAA, vBB, vCC
            // aput-byte    vAA, vBB, vCC
            // aput-char    vAA, vBB, vCC
            // aput-short   vAA, vBB, vCC
            {
                const auto& raw = raw_insn->fmt_23x;

                prop.insn = insn_aput(op, {{raw.reg_a, raw.reg_b, raw.reg_c}},
                                      {});
            }
            break;
        case opcode::op_iget:
        case opcode::op_iget_wide:
        case opcode::op_iget_object:
        case opcode::op_iget_boolean:
        case opcode::op_iget_byte:
        case opcode::op_iget_char:
        case opcode::op_iget_short:
        case opcode::op_iget_volatile:
        case opcode::op_iget_wide_volatile:
        case opcode::op_iget_object_volatile:
            //  iget                 vA, vB, field@CCCC
            //  iget-wide            vA, vB, field@CCCC
            //  iget-object          vA, vB, field@CCCC
            //  iget-boolean         vA, vB, field@CCCC
            //  iget-byte            vA, vB, field@CCCC
            //  iget-char            vA, vB, field@CCCC
            //  iget-short           vA, vB, field@CCCC
            // +iget-volatile        vA, vB, field@CCCC
            // +iget-wide-volatile   vA, vB, field@CCCC
            // +iget-object-volatile vA, vB, field@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_iget(op, {{raw.reg_a, raw.reg_b}},
                                      {hdl(), raw.idx_c});
            }
            break;
        case opcode::op_iget_quick:
        case opcode::op_iget_wide_quick:
        case opcode::op_iget_object_quick:
            // +iget-quick        vA, vB, field@CCCC
            // +iget-wide-quick   vA, vB, field@CCCC
            // +iget-object-quick vA, vB, field@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_iget_quick(op, {{raw.reg_a, raw.reg_b}},
                                            raw.idx_c);
            }
            break;
        case opcode::op_iput:
        case opcode::op_iput_wide:
        case opcode::op_iput_object:
        case opcode::op_iput_boolean:
        case opcode::op_iput_byte:
        case opcode::op_iput_char:
        case opcode::op_iput_short:
        case opcode::op_iput_volatile:
        case opcode::op_iput_wide_volatile:
        case opcode::op_iput_object_volatile:
            //  iput                 vA, vB, field@CCCC
            //  iput-wide            vA, vB, field@CCCC
            //  iput-object          vA, vB, field@CCCC
            //  iput-boolean         vA, vB, field@CCCC
            //  iput-byte            vA, vB, field@CCCC
            //  iput-char            vA, vB, field@CCCC
            //  iput-short           vA, vB, field@CCCC
            // +iput-volatile        vA, vB, field@CCCC
            // +iput-wide-volatile   vA, vB, field@CCCC
            // +iput-object-volatile vA, vB, field@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_iput(op, {{raw.reg_a, raw.reg_b}},
                                      {hdl(), raw.idx_c});
            }
            break;
        case opcode::op_iput_quick:
        case opcode::op_iput_wide_quick:
        case opcode::op_iput_object_quick:
            // +iput-quick        vA, vB, field@CCCC
            // +iput-wide-quick   vA, vB, field@CCCC
            // +iput-object-quick vA, vB, field@CCCC
            {
                const auto& raw = raw_insn->fmt_22c;

                prop.insn = insn_iput_quick(op, {{raw.reg_a, raw.reg_b}},
                                            raw.idx_c);
            }
            break;
        case opcode::op_sget:
        case opcode::op_sget_wide:
        case opcode::op_sget_object:
        case opcode::op_sget_boolean:
        case opcode::op_sget_byte:
        case opcode::op_sget_char:
        case opcode::op_sget_short:
        case opcode::op_sget_volatile:
        case opcode::op_sget_wide_volatile:
        case opcode::op_sget_object_volatile:
            //  sget                 vAA, field@BBBB
            //  sget-wide            vAA, field@BBBB
            //  sget-object          vAA, field@BBBB
            //  sget-boolean         vAA, field@BBBB
            //  sget-byte            vAA, field@BBBB
            //  sget-char            vAA, field@BBBB
            //  sget-short           vAA, field@BBBB
            // +sget-volatile        vAA, field@BBBB
            // +sget-wide-volatile   vAA, field@BBBB
            // +sget-object-volatile vAA, field@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;

                prop.insn = insn_sget(op, {{raw.reg_a}}, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_sput:
        case opcode::op_sput_wide:
        case opcode::op_sput_object:
        case opcode::op_sput_boolean:
        case opcode::op_sput_byte:
        case opcode::op_sput_char:
        case opcode::op_sput_short:
        case opcode::op_sput_volatile:
        case opcode::op_sput_wide_volatile:
        case opcode::op_sput_object_volatile:
            //  sput                 vAA, field@BBBB
            //  sput-wide            vAA, field@BBBB
            //  sput-object          vAA, field@BBBB
            //  sput-boolean         vAA, field@BBBB
            //  sput-byte            vAA, field@BBBB
            //  sput-char            vAA, field@BBBB
            //  sput-short           vAA, field@BBBB
            // +sput-volatile        vAA, field@BBBB
            // +sput-wide-volatile   vAA, field@BBBB
            // +sput-object-volatile vAA, field@BBBB
            {
                const auto& raw = raw_insn->fmt_21c;

                prop.insn = insn_sput(op, {{raw.reg_a}}, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_invoke_virtual:
        case opcode::op_invoke_super:
        case opcode::op_invoke_direct:
        case opcode::op_invoke_static:
        case opcode::op_invoke_interface:
            // invoke-virtual   {vC, vD, vE, vF, vG}, meth@BBBB
            // invoke-super     {vC, vD, vE, vF, vG}, meth@BBBB
            // invoke-direct    {vC, vD, vE, vF, vG}, meth@BBBB
            // invoke-interface {vC, vD, vE, vF, vG}, meth@BBBB
            // invoke-static    {vC, vD, vE, vF, vG}, meth@BBBB
            {
                const auto& raw = raw_insn->fmt_35c;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = raw.reg_d;
                regs[2] = raw.reg_e;
                regs[3] = raw.reg_f;
                regs[4] = raw.reg_g;
                for (size_t i = raw.size_a; i < 5; ++i) {
                    regs[i] = register_idx::idx_unknown;
                }

                prop.insn = insn_invoke(op, regs, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_invoke_virtual_range:
        case opcode::op_invoke_super_range:
        case opcode::op_invoke_direct_range:
        case opcode::op_invoke_static_range:
        case opcode::op_invoke_interface_range:
        case opcode::op_invoke_object_init_range:
            //  invoke-virtual/range     {vCCCC .. vNNNN}, method@BBBB
            //  invoke-super/range       {vCCCC .. vNNNN}, method@BBBB
            //  invoke-direct/range      {vCCCC .. vNNNN}, method@BBBB
            //  invoke-interface/range   {vCCCC .. vNNNN}, method@BBBB
            //  invoke-static/range      {vCCCC .. vNNNN}, method@BBBB
            // +invoke-object-init/range {vCCCC .. vNNNN}, method@BBBB
            {
                const auto& raw = raw_insn->fmt_3rc;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = register_idx::idx_unknown;
                regs[2] = register_idx::idx_unknown;
                regs[3] = register_idx::idx_unknown;
                regs[4] = raw.reg_c + raw.size_a - 1;

                prop.insn = insn_invoke(op, regs, {hdl(), raw.idx_b});
            }
            break;
        case opcode::op_execute_inline:
        case opcode::op_invoke_virtual_quick:
        case opcode::op_invoke_super_quick:
            // +execute-inline       {vC, vD, vE, vF, vG}, vtaboff@BBBB
            // +invoke-virtual-quick {vC, vD, vE, vF, vG}, vtaboff@BBBB
            // +invoke-super-quick   {vC, vD, vE, vF, vG}, vtaboff@BBBB
            {
                const auto& raw = raw_insn->fmt_35c;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = raw.reg_d;
                regs[2] = raw.reg_e;
                regs[3] = raw.reg_f;
                regs[4] = raw.reg_g;
                for (size_t i = raw.size_a; i < 5; ++i) {
                    regs[i] = register_idx::idx_unknown;
                }

                prop.insn = insn_invoke_quick(op, regs, raw.idx_b);

#if 0
                const auto& raw = raw_insn->fmt_35ms;
                const size_t param_size = raw.size_a;
                const VTabIdx vtab_idx  = raw.off_b;
                const register_idx reg_c = raw.reg_c;
                const register_idx reg_d = raw.reg_d;
                const register_idx reg_e = raw.reg_e;
                const register_idx reg_f = raw.reg_f;
                const register_idx reg_g = raw.reg_g;

                // TODO: Resolve.
                const MethodIdx method_idx = MethodIdx::idx_unknown;

                insn = new InvokeQuick(offset, op, param_size,
                        reg_c, reg_d, reg_e, reg_f, reg_g, method_idx, vtab_idx);
#endif
            }
            break;
        case opcode::op_execute_inline_range:
        case opcode::op_invoke_virtual_quick_range:
        case opcode::op_invoke_super_quick_range:
            // +execute-inline/range       {vCCCC .. vNNNN}, vtaboff@BBBB
            // +invoke-virtual-quick/range {vCCCC .. vNNNN}, vtaboff@BBBB
            // +invoke-super-quick/range   {vCCCC .. vNNNN}, vtaboff@BBBB
            {
                // const auto& raw = raw_insn->fmt_3rms;
                // const size_t array_size      = raw.size_a;
                // const VTabIdx vtab_idx  = raw.off_b;
                // const register_idx reg_c = raw.reg_c;
                //
                // // TODO: Resolve.
                // const MethodIdx method_idx = MethodIdx::idx_unknown;
                //
                // insn = new InvokeQuick(offset, op, array_size,
                //         reg_c, method_idx, vtab_idx);
                const auto& raw = raw_insn->fmt_3rc;

                std::array<register_idx, 5> regs;
                regs[0] = raw.reg_c;
                regs[1] = register_idx::idx_unknown;
                regs[2] = register_idx::idx_unknown;
                regs[3] = register_idx::idx_unknown;
                regs[4] = raw.reg_c + raw.size_a - 1;

                prop.insn = insn_invoke_quick(op, regs, raw.idx_b);
            }
            break;
        case opcode::op_neg_int:
        case opcode::op_not_int:
        case opcode::op_neg_long:
        case opcode::op_not_long:
        case opcode::op_neg_float:
        case opcode::op_neg_double:
        case opcode::op_int_to_long:
        case opcode::op_int_to_float:
        case opcode::op_int_to_double:
        case opcode::op_long_to_int:
        case opcode::op_long_to_float:
        case opcode::op_long_to_double:
        case opcode::op_float_to_int:
        case opcode::op_float_to_long:
        case opcode::op_float_to_double:
        case opcode::op_double_to_int:
        case opcode::op_double_to_long:
        case opcode::op_double_to_float:
        case opcode::op_int_to_byte:
        case opcode::op_int_to_char:
        case opcode::op_int_to_short:
            // neg-int         vA, vB
            // not-int         vA, vB
            // neg-long        vA, vB
            // not-long        vA, vB
            // neg-float       vA, vB
            // neg-double      vA, vB
            // int-to-long     vA, vB
            // int-to-float    vA, vB
            // int-to-double   vA, vB
            // long-to-int     vA, vB
            // long-to-float   vA, vB
            // long-to-double  vA, vB
            // float-to-int    vA, vB
            // float-to-long   vA, vB
            // float-to-double vA, vB
            // double-to-int   vA, vB
            // double-to-long  vA, vB
            // double-to-float vA, vB
            // int-to-byte     vA, vB
            // int-to-char     vA, vB
            // int-to-short    vA, vB
            {
                const auto& raw = raw_insn->fmt_12x;

                prop.insn
                        = insn_unary_arith_op(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_add_int:
        case opcode::op_sub_int:
        case opcode::op_mul_int:
        case opcode::op_div_int:
        case opcode::op_rem_int:
        case opcode::op_and_int:
        case opcode::op_or_int:
        case opcode::op_xor_int:
        case opcode::op_shl_int:
        case opcode::op_shr_int:
        case opcode::op_ushr_int:
        case opcode::op_add_long:
        case opcode::op_sub_long:
        case opcode::op_mul_long:
        case opcode::op_div_long:
        case opcode::op_rem_long:
        case opcode::op_and_long:
        case opcode::op_or_long:
        case opcode::op_xor_long:
        case opcode::op_shl_long:
        case opcode::op_shr_long:
        case opcode::op_ushr_long:
        case opcode::op_add_float:
        case opcode::op_sub_float:
        case opcode::op_mul_float:
        case opcode::op_div_float:
        case opcode::op_rem_float:
        case opcode::op_add_double:
        case opcode::op_sub_double:
        case opcode::op_mul_double:
        case opcode::op_div_double:
        case opcode::op_rem_double:
            // add-int    vAA, vBB, vCC
            // sub-int    vAA, vBB, vCC
            // mul-int    vAA, vBB, vCC
            // div-int    vAA, vBB, vCC
            // rem-int    vAA, vBB, vCC
            // and-int    vAA, vBB, vCC
            // or-int     vAA, vBB, vCC
            // xor-int    vAA, vBB, vCC
            // shl-int    vAA, vBB, vCC
            // shr-int    vAA, vBB, vCC
            // ushr-int   vAA, vBB, vCC
            // add-long   vAA, vBB, vCC
            // sub-long   vAA, vBB, vCC
            // mul-long   vAA, vBB, vCC
            // div-long   vAA, vBB, vCC
            // rem-long   vAA, vBB, vCC
            // and-long   vAA, vBB, vCC
            // or-long    vAA, vBB, vCC
            // xor-long   vAA, vBB, vCC
            // shl-long   vAA, vBB, vCC
            // shr-long   vAA, vBB, vCC
            // ushr-long  vAA, vBB, vCC
            // add-float  vAA, vBB, vCC
            // sub-float  vAA, vBB, vCC
            // mul-float  vAA, vBB, vCC
            // div-float  vAA, vBB, vCC
            // rem-float  vAA, vBB, vCC
            // add-double vAA, vBB, vCC
            // sub-double vAA, vBB, vCC
            // mul-double vAA, vBB, vCC
            // div-double vAA, vBB, vCC
            // rem-double vAA, vBB, vCC
            {
                const auto& raw = raw_insn->fmt_23x;

                prop.insn = insn_binary_arith_op(
                        op, {{raw.reg_a, raw.reg_b, raw.reg_c}}, {});
            }
            break;
        case opcode::op_add_int_2addr:
        case opcode::op_sub_int_2addr:
        case opcode::op_mul_int_2addr:
        case opcode::op_div_int_2addr:
        case opcode::op_rem_int_2addr:
        case opcode::op_and_int_2addr:
        case opcode::op_or_int_2addr:
        case opcode::op_xor_int_2addr:
        case opcode::op_shl_int_2addr:
        case opcode::op_shr_int_2addr:
        case opcode::op_ushr_int_2addr:
        case opcode::op_add_long_2addr:
        case opcode::op_sub_long_2addr:
        case opcode::op_mul_long_2addr:
        case opcode::op_div_long_2addr:
        case opcode::op_rem_long_2addr:
        case opcode::op_and_long_2addr:
        case opcode::op_or_long_2addr:
        case opcode::op_xor_long_2addr:
        case opcode::op_shl_long_2addr:
        case opcode::op_shr_long_2addr:
        case opcode::op_ushr_long_2addr:
        case opcode::op_add_float_2addr:
        case opcode::op_sub_float_2addr:
        case opcode::op_mul_float_2addr:
        case opcode::op_div_float_2addr:
        case opcode::op_rem_float_2addr:
        case opcode::op_add_double_2addr:
        case opcode::op_sub_double_2addr:
        case opcode::op_mul_double_2addr:
        case opcode::op_div_double_2addr:
        case opcode::op_rem_double_2addr:
            // add-int/2addr    vA, vB
            // sub-int/2addr    vA, vB
            // mul-int/2addr    vA, vB
            // div-int/2addr    vA, vB
            // rem-int/2addr    vA, vB
            // and-int/2addr    vA, vB
            // or-int/2addr     vA, vB
            // xor-int/2addr    vA, vB
            // shl-int/2addr    vA, vB
            // shr-int/2addr    vA, vB
            // ushr-int/2addr   vA, vB
            // add-long/2addr   vA, vB
            // sub-long/2addr   vA, vB
            // mul-long/2addr   vA, vB
            // div-long/2addr   vA, vB
            // rem-long/2addr   vA, vB
            // and-long/2addr   vA, vB
            // or-long/2addr    vA, vB
            // xor-long/2addr   vA, vB
            // shl-long/2addr   vA, vB
            // shr-long/2addr   vA, vB
            // ushr-long/2addr  vA, vB
            // add-float/2addr  vA, vB
            // sub-float/2addr  vA, vB
            // mul-float/2addr  vA, vB
            // div-float/2addr  vA, vB
            // rem-float/2addr  vA, vB
            // add-double/2addr vA, vB
            // sub-double/2addr vA, vB
            // mul-double/2addr vA, vB
            // div-double/2addr vA, vB
            // rem-double/2addr vA, vB
            {
                const auto& raw = raw_insn->fmt_12x;

                prop.insn
                        = insn_aug_assign_op(op, {{raw.reg_a, raw.reg_b}}, {});
            }
            break;
        case opcode::op_add_int_lit16:
        case opcode::op_rsub_int:
        case opcode::op_mul_int_lit16:
        case opcode::op_div_int_lit16:
        case opcode::op_rem_int_lit16:
        case opcode::op_and_int_lit16:
        case opcode::op_or_int_lit16:
        case opcode::op_xor_int_lit16:
            // add-int/lit16  vA, vB, #+CCCC
            // rsub-int/lit16 vA, vB, #+CCCC
            // mul-int/lit16  vA, vB, #+CCCC
            // div-int/lit16  vA, vB, #+CCCC
            // rem-int/lit16  vA, vB, #+CCCC
            // and-int/lit16  vA, vB, #+CCCC
            // or-int/lit16   vA, vB, #+CCCC
            // xor-int/lit16  vA, vB, #+CCCC
            {
                const auto& raw = raw_insn->fmt_22s;

                prop.insn = insn_const_arith_op(op, {{raw.reg_a, raw.reg_b}},
                                                {raw.imm_c});
            }
            break;
        case opcode::op_add_int_lit8:
        case opcode::op_rsub_int_lit8:
        case opcode::op_mul_int_lit8:
        case opcode::op_div_int_lit8:
        case opcode::op_rem_int_lit8:
        case opcode::op_and_int_lit8:
        case opcode::op_or_int_lit8:
        case opcode::op_xor_int_lit8:
        case opcode::op_shl_int_lit8:
        case opcode::op_shr_int_lit8:
        case opcode::op_ushr_int_lit8:
            // add-int/lit8  vAA, vBB, #+CC
            // rsub-int/lit8 vAA, vBB, #+CC
            // mul-int/lit8  vAA, vBB, #+CC
            // div-int/lit8  vAA, vBB, #+CC
            // rem-int/lit8  vAA, vBB, #+CC
            // and-int/lit8  vAA, vBB, #+CC
            // or-int/lit8   vAA, vBB, #+CC
            // xor-int/lit8  vAA, vBB, #+CC
            // shl-int/lit8  vAA, vBB, #+CC
            // shr-int/lit8  vAA, vBB, #+CC
            // ushr-int/lit8 vAA, vBB, #+CC
            {
                const auto& raw = raw_insn->fmt_22b;

                prop.insn = insn_const_arith_op(op, {{raw.reg_a, raw.reg_b}},
                                                {raw.imm_c});
            }
            break;
        case opcode::op_throw_verification_error:
            // +throw-verification-error AA kind@BBBB
            {
                prop.insn = insn_nop(opcode::op_nop, {{}}, {});
            }
            break;
        default:
            // Unknown instruction encountered.
            prop.insn = insn_nop(opcode::op_nop, {{}}, {});
        }

        if (insn_info.can_continue()) {
            add_edge(v, v + 1, insn_control_flow_edge_property(), g);
        }
    }

    parse_debug_info(g, mvprop, raw_header->debug_info_off);

    return g;
}

void dex_file::parse_debug_info(insn_graph& g, method_vertex_property& mvprop,
                                uint32_t debug_info_off) const
{
    if (debug_info_off == 0) {
        return;
    }

    // Create a stream reader.
    auto reader = stream_reader{dex_begin_};
    reader.move_head(debug_info_off);

    // Get the starting line number.
    uint32_t line_start = reader.get_uleb128();

    // Get the parameter names.
    size_t parameters_size = reader.get_uleb128();
    if (mvprop.params.size() == parameters_size) {
        for (size_t i = 0; i < parameters_size; ++i) {
            dex_string_idx param_name_idx = reader.get_uleb128p1();
            if (param_name_idx.valid()) {
                mvprop.params[i].name = ids_.c_str(param_name_idx);
            }
        }
    }

    // Opcode.
    enum class dbg_opcode : uint8_t {
        end_sequence,
        advance_pc,
        advance_line,
        start_local,
        start_local_extended,
        end_local,
        restart_local,
        set_prologue_end,
        set_epilogue_begin,
        set_file,
        special_opcode_start
    };

    bool line_num_valid = true;

    // Debug virtual machine registers.
    uint16_t address = 0;
    uint16_t line = line_start;
    bool executing = true;

    // Execute the bytecode.
    do {
        dbg_opcode op = reader.get<dbg_opcode>();

        switch (op) {
        case dbg_opcode::end_sequence:
            // Terminate the debug info sequence.
            executing = false;
            break;
        case dbg_opcode::advance_pc:
            // Advance the address register without emitting a positions entry.
            address += reader.get_uleb128();
            break;
        case dbg_opcode::advance_line:
            // Advance the line register without emitting a positions entry.
            line += reader.get_sleb128();
            break;
        case dbg_opcode::start_local:
            // Introduce a local variable at the current address.
            break;
        case dbg_opcode::start_local_extended:
            // Introduce a local with a type signature at the current address.
            break;
        case dbg_opcode::end_local:
            // Mark the currently-live local variable as out of scope at the
            // current address.
            break;
        case dbg_opcode::restart_local:
            // Re-introduce a local variable at the current address. The name
            // and type are the same as the last local that was live in the
            // specified register.
            break;
        case dbg_opcode::set_prologue_end:
            // Set the prologue_end state machine register, indicating that the
            // next position entry that is added should be considered the end of
            // a method prologue (an appropriate place for a method breakpoint).
            // The prologue_end register is cleared by any special (>= 0x0a)
            // opcode.
            break;
        case dbg_opcode::set_epilogue_begin:
            // Set the epilogue_begin state machine register, indicating that
            // the next position entry that is added should be considered the
            // beginning of a method epilogue (an appropriate place to suspend
            // execution before method exit). The epilogue_begin register is
            // cleared by any special (>= 0x0a) opcode.
            break;
        case dbg_opcode::set_file:
            // All subsequent line number entries make reference to this source
            // file name, instead of the default name specified in code_item.
            {
                // For now, we ignore set_file. So invalidate the line numbers
                // from now on.
                line_num_valid = false;
            }
            break;
        default:
            // Special opcodes.
            {
                // Update the line number and the address.
                constexpr int line_base = -4;
                constexpr int line_range = 15;
                uint8_t adjusted_op = static_cast<uint8_t>(op) - 0x0a;
                line += line_base + (adjusted_op % line_range);
                address += (adjusted_op / line_range);

                if (line_num_valid) {
                    if (const auto v = lookup_insn_vertex(address, g)) {
                        g[*v].line_num = line;
                    }
                }
            }
            break;
        }
    } while (executing);
}
