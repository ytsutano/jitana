#ifndef JITANA_HANDLE_HPP
#define JITANA_HANDLE_HPP

#include <iostream>
#include <cstdint>

namespace jitana {
    using class_loader_hdl = uint8_t;

    /// A handle to a DEX file.
    struct dex_file_hdl {
        class_loader_hdl loader_hdl;
        uint8_t idx;

        explicit operator uint16_t() const
        {
            return static_cast<uint16_t>(loader_hdl) << 8 | idx;
        }

        friend bool operator==(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return x.loader_hdl == y.loader_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_file_hdl& x, const dex_file_hdl& y)
        {
            return !(x == y);
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

        friend std::ostream& operator<<(std::ostream& os, const dex_type_hdl& x)
        {
            return os << x.file_hdl << "_" << x.idx;
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

        friend std::ostream& operator<<(std::ostream& os,
                                        const dex_method_hdl& x)
        {
            return os << x.file_hdl << "_" << x.idx;
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

        friend std::ostream& operator<<(std::ostream& os,
                                        const dex_field_hdl& x)
        {
            return os << x.file_hdl << "_" << x.idx;
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

        friend std::ostream& operator<<(std::ostream& os, const dex_insn_hdl& x)
        {
            return os << x.method_hdl << "_" << x.idx;
        }
    };

    struct dex_reg_hdl {
        dex_method_hdl method_hdl;
        uint16_t idx;

        dex_reg_hdl()
        {
        }

        dex_reg_hdl(const dex_method_hdl& method_hdl, uint16_t idx)
                : method_hdl(method_hdl), idx(idx)
        {
        }

        explicit operator uint64_t() const
        {
            auto mh = static_cast<uint32_t>(method_hdl);
            return static_cast<uint64_t>(mh) << 16 | idx;
        }

        friend bool operator==(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return x.method_hdl == y.method_hdl && x.idx == y.idx;
        }

        friend bool operator!=(const dex_reg_hdl& x, const dex_reg_hdl& y)
        {
            return !(x == y);
        }

        friend std::ostream& operator<<(std::ostream& os, const dex_reg_hdl& x)
        {
            return os << x.method_hdl << "_" << x.idx;
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
    struct hash<jitana::dex_type_hdl> {
        size_t operator()(const jitana::dex_type_hdl& hdl) const
        {
            return static_cast<uint32_t>(hdl);
        }
    };

    template <>
    struct hash<jitana::jvm_type_hdl> {
        size_t operator()(const jitana::jvm_type_hdl& hdl) const
        {
            return std::hash<std::string>()(hdl.descriptor) ^ hdl.loader_hdl;
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

#endif
