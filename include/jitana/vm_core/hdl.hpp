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

#ifndef JITANA_HANDLE_HPP
#define JITANA_HANDLE_HPP

#include "jitana/vm_core/idx.hpp"

#include <iostream>
#include <cstdint>
#include <boost/variant.hpp>

namespace jitana {
    struct class_loader_hdl {
        uint8_t idx;

        class_loader_hdl()
        {
        }

        class_loader_hdl(uint8_t idx) : idx(idx)
        {
        }

        explicit operator uint8_t() const
        {
            return idx;
        }

        explicit operator unsigned() const
        {
            return idx;
        }

        friend bool operator==(const class_loader_hdl& x,
                               const class_loader_hdl& y)
        {
            return x.idx == y.idx;
        }

        friend bool operator!=(const class_loader_hdl& x,
                               const class_loader_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const class_loader_hdl& x,
                              const class_loader_hdl& y)
        {
            return x.idx < y.idx;
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const class_loader_hdl& x)
        {
            return os << unsigned(x);
        }
    };

    /// A handle to a DEX file.
    struct dex_file_hdl {
        class_loader_hdl loader_hdl;
        uint8_t idx;

        explicit operator uint16_t() const
        {
            auto lh = static_cast<uint8_t>(loader_hdl);
            return static_cast<uint16_t>(lh) << 8 | idx;
        }

        friend bool operator==(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return x.loader_hdl == y.loader_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return x.loader_hdl < y.loader_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os, const dex_file_hdl& x)
        {
            return os << unsigned(x.loader_hdl) << "_" << unsigned(x.idx);
        }
    };

    struct dex_type_hdl {
        dex_file_hdl file_hdl;
        uint16_t idx;

        dex_type_hdl()
        {
        }

        dex_type_hdl(const dex_file_hdl& file_hdl, uint16_t idx)
                : file_hdl(file_hdl), idx(idx)
        {
        }

        explicit operator uint32_t() const
        {
            auto fh = static_cast<uint16_t>(file_hdl);
            return static_cast<uint32_t>(fh) << 16 | idx;
        }

        friend bool operator==(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return x.file_hdl == y.file_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return x.file_hdl < y.file_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_type_hdl& x, const dex_type_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os, const dex_type_hdl& x)
        {
            return os << x.file_hdl << "_t" << x.idx;
        }
    };

    struct dex_method_hdl {
        dex_file_hdl file_hdl;
        uint16_t idx;

        dex_method_hdl()
        {
        }

        dex_method_hdl(const dex_file_hdl& file_hdl, uint16_t idx)
                : file_hdl(file_hdl), idx(idx)
        {
        }

        explicit operator uint32_t() const
        {
            auto fh = static_cast<uint16_t>(file_hdl);
            return static_cast<uint32_t>(fh) << 16 | idx;
        }

        friend bool operator==(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return x.file_hdl == y.file_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return x.file_hdl < y.file_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_method_hdl& x, const dex_method_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const dex_method_hdl& x)
        {
            return os << x.file_hdl << "_m" << x.idx;
        }
    };

    struct dex_field_hdl {
        dex_file_hdl file_hdl;
        uint16_t idx;

        dex_field_hdl()
        {
        }

        dex_field_hdl(const dex_file_hdl& file_hdl, uint16_t idx)
                : file_hdl(file_hdl), idx(idx)
        {
        }

        explicit operator uint32_t() const
        {
            auto fh = static_cast<uint16_t>(file_hdl);
            return static_cast<uint32_t>(fh) << 16 | idx;
        }

        friend bool operator==(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return x.file_hdl == y.file_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return x.file_hdl < y.file_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_field_hdl& x, const dex_field_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const dex_field_hdl& x)
        {
            return os << x.file_hdl << "_f" << x.idx;
        }
    };

    struct dex_insn_hdl {
        dex_method_hdl method_hdl;
        uint16_t idx;

        dex_insn_hdl()
        {
        }

        dex_insn_hdl(const dex_method_hdl& method_hdl, uint16_t idx)
                : method_hdl(method_hdl), idx(idx)
        {
        }

        explicit operator uint64_t() const
        {
            auto mh = static_cast<uint32_t>(method_hdl);
            return static_cast<uint64_t>(mh) << 16 | idx;
        }

        friend bool operator==(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return x.method_hdl == y.method_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return x.method_hdl < y.method_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_insn_hdl& x, const dex_insn_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os, const dex_insn_hdl& x)
        {
            return os << x.method_hdl << "_i" << x.idx;
        }
    };

    struct dex_reg_hdl {
        dex_insn_hdl insn_hdl;
        uint16_t idx;

        dex_reg_hdl()
        {
        }

        dex_reg_hdl(const dex_insn_hdl& insn_hdl, uint16_t idx)
                : insn_hdl(insn_hdl), idx(idx)
        {
        }

        explicit operator uint64_t() const
        {
            return static_cast<uint64_t>(insn_hdl) << 32 | idx;
        }

        friend bool operator==(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return x.insn_hdl == y.insn_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return x.insn_hdl < y.insn_hdl && x.idx < y.idx;
        }

        friend bool operator>(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return y < x;
        }

        friend bool operator<=(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os, const dex_reg_hdl& x)
        {
            return os << x.insn_hdl << "_" << register_idx(int16_t(x.idx));
        }
    };

    struct jvm_type_hdl {
        class_loader_hdl loader_hdl;
        std::string descriptor;

        jvm_type_hdl()
        {
        }

        jvm_type_hdl(const class_loader_hdl& loader_hdl, std::string descriptor)
                : loader_hdl(loader_hdl), descriptor(std::move(descriptor))
        {
        }

        friend bool operator==(const jvm_type_hdl& x, const jvm_type_hdl& y)
        {
            return x.loader_hdl == y.loader_hdl && x.descriptor == y.descriptor;
        }

        friend bool operator!=(const jvm_type_hdl& x, const jvm_type_hdl& y)
        {
            return !(x == y);
        }

        friend bool operator<(const jvm_type_hdl& x, const jvm_type_hdl& y)
        {
            return (x.loader_hdl != y.loader_hdl) ? x.loader_hdl < y.loader_hdl
                                                  : x.descriptor < y.descriptor;
        }

        friend std::ostream& operator<<(std::ostream& os, const jvm_type_hdl& x)
        {
            return os << unsigned(x.loader_hdl) << ":" << x.descriptor;
        }
    };

    struct jvm_method_hdl {
        jvm_type_hdl type_hdl;
        std::string unique_name;

        jvm_method_hdl()
        {
        }

        jvm_method_hdl(const jvm_type_hdl& type_hdl, std::string unique_name)
                : type_hdl(type_hdl), unique_name(std::move(unique_name))
        {
        }

        const char* return_descriptor() const
        {
            auto last = unique_name.rfind(')');
            return last != std::string::npos ? &unique_name[last + 1] : "";
        }

        friend bool operator==(const jvm_method_hdl& x, const jvm_method_hdl& y)
        {
            return x.type_hdl == y.type_hdl && x.unique_name == y.unique_name;
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const jvm_method_hdl& x)
        {
            return os << x.type_hdl << "." << x.unique_name;
        }
    };

    struct jvm_field_hdl {
        jvm_type_hdl type_hdl;
        std::string unique_name;

        jvm_field_hdl()
        {
        }

        jvm_field_hdl(const jvm_type_hdl& type_hdl, std::string unique_name)
                : type_hdl(type_hdl), unique_name(std::move(unique_name))
        {
        }

        friend bool operator==(const jvm_field_hdl& x, const jvm_field_hdl& y)
        {
            return x.type_hdl == y.type_hdl && x.unique_name == y.unique_name;
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const jvm_field_hdl& x)
        {
            return os << x.type_hdl << "." << x.unique_name;
        }
    };
}

namespace std {
    template <>
    struct hash<jitana::class_loader_hdl> {
        size_t operator()(const jitana::class_loader_hdl& hdl) const
        {
            return static_cast<uint8_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_file_hdl> {
        size_t operator()(const jitana::dex_file_hdl& hdl) const
        {
            return static_cast<uint16_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_type_hdl> {
        size_t operator()(const jitana::dex_type_hdl& hdl) const
        {
            return static_cast<uint32_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_method_hdl> {
        size_t operator()(const jitana::dex_method_hdl& hdl) const
        {
            return static_cast<uint32_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_field_hdl> {
        size_t operator()(const jitana::dex_field_hdl& hdl) const
        {
            return static_cast<uint32_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_insn_hdl> {
        size_t operator()(const jitana::dex_insn_hdl& hdl) const
        {
            return static_cast<uint64_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::dex_reg_hdl> {
        size_t operator()(const jitana::dex_reg_hdl& hdl) const
        {
            return static_cast<uint64_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::jvm_type_hdl> {
        size_t operator()(const jitana::jvm_type_hdl& hdl) const
        {
            return std::hash<std::string>()(hdl.descriptor)
                    ^ unsigned(hdl.loader_hdl);
        }
    };

    template <>
    struct hash<jitana::jvm_method_hdl> {
        size_t operator()(const jitana::jvm_method_hdl& hdl) const
        {
            // TODO: come up with a better hash function.
            return std::hash<jitana::jvm_type_hdl>()(hdl.type_hdl)
                    ^ std::hash<std::string>()(hdl.unique_name);
        }
    };

    template <>
    struct hash<jitana::jvm_field_hdl> {
        size_t operator()(const jitana::jvm_field_hdl& hdl) const
        {
            // TODO: come up with a better hash function.
            return std::hash<jitana::jvm_type_hdl>()(hdl.type_hdl)
                    ^ std::hash<std::string>()(hdl.unique_name);
        }
    };
}

namespace jitana {
    using any_dex_hdl
            = boost::variant<class_loader_hdl, dex_file_hdl, dex_type_hdl,
                             dex_method_hdl, dex_field_hdl, dex_insn_hdl,
                             dex_reg_hdl>;

    namespace detail {
        struct extract_hash : public boost::static_visitor<size_t> {
            template <typename T>
            size_t operator()(const T& x) const
            {
                return std::hash<uint64_t>()(std::hash<T>()(x));
            }
        };
    }
}

namespace std {
    template <>
    struct hash<jitana::any_dex_hdl> {
        size_t operator()(const jitana::any_dex_hdl& hdl) const
        {
            return hdl.which()
                    + (boost::apply_visitor(jitana::detail::extract_hash(), hdl)
                       << 4);
        }
    };
}

namespace boost {
    template <>
    struct hash<jitana::any_dex_hdl> {
        size_t operator()(const jitana::any_dex_hdl& hdl) const
        {
            return std::hash<jitana::any_dex_hdl>()(hdl);
        }
    };
}

#endif
