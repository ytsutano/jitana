#ifndef JITANA_DEX_RAW_TYPES_HPP
#define JITANA_DEX_RAW_TYPES_HPP

#include "jitana/core/idx.hpp"
#include "jitana/core/opcode.hpp"
#include "jitana/core/access_flags.hpp"

namespace jitana {
    /// A string ID.
    struct dex_string_id {
    private:
        uint32_t string_data_off_;

    public:
        uint32_t string_data_off() const
        {
            return string_data_off_;
        }
    };
    static_assert(std::is_pod<dex_string_id>::value, "");

    /// A type ID.
    struct dex_type_id {
    private:
        uint32_t descriptor_idx_;

    public:
        dex_string_idx descriptor_idx() const
        {
            return descriptor_idx_;
        }
    };
    static_assert(std::is_pod<dex_type_id>::value, "");

    /// A prototype ID.
    struct dex_proto_id {
    private:
        uint32_t shorty_idx_;
        uint32_t return_type_idx_;
        uint32_t parameters_off_;

    public:
        dex_string_idx shorty_idx() const
        {
            return shorty_idx_;
        }

        dex_type_idx return_type_idx() const
        {
            return return_type_idx_;
        }

        uint32_t parameters_off() const
        {
            return parameters_off_;
        }
    };
    static_assert(std::is_pod<dex_proto_id>::value, "");

    /// A field ID.
    struct dex_field_id {
    private:
        uint16_t class_idx_;
        uint16_t type_idx_;
        uint32_t name_idx_;

    public:
        dex_type_idx class_idx() const
        {
            return class_idx_;
        }

        dex_type_idx type_idx() const
        {
            return type_idx_;
        }

        dex_string_idx name_idx() const
        {
            return name_idx_;
        }
    };
    static_assert(std::is_pod<dex_field_id>::value, "");

    /// A method ID.
    struct dex_method_id {
    private:
        uint16_t class_idx_;
        uint16_t proto_idx_;
        uint32_t name_idx_;

    public:
        dex_type_idx class_idx() const
        {
            return class_idx_;
        }

        dex_proto_idx proto_idx() const
        {
            return proto_idx_;
        }

        dex_string_idx name_idx() const
        {
            return name_idx_;
        }
    };
    static_assert(std::is_pod<dex_method_id>::value, "");

    namespace detail {
        struct dex_opt_header {
            uint8_t magic[8];
            uint32_t dex_off;
            uint32_t dex_size;
            uint32_t deps_off;
            uint32_t deps_size;
            uint32_t opt_off;
            uint32_t opt_size;
            uint32_t flags;
            uint32_t checksum;
        } __attribute__((packed));
        static_assert(std::is_pod<dex_opt_header>::value, "");

        struct dex_header {
            uint8_t magic[8];
            uint32_t checksum;
            uint8_t signature[20];
            uint32_t file_size;
            uint32_t header_size;
            uint32_t endian_tag;
            uint32_t link_size;
            uint32_t link_off;
            uint32_t map_off;
            uint32_t string_ids_size;
            uint32_t string_ids_off;
            uint32_t type_ids_size;
            uint32_t type_ids_off;
            uint32_t proto_ids_size;
            uint32_t proto_ids_off;
            uint32_t field_ids_size;
            uint32_t field_ids_off;
            uint32_t method_ids_size;
            uint32_t method_ids_off;
            uint32_t class_defs_size;
            uint32_t class_defs_off;
            uint32_t data_size;
            uint32_t data_off;
        } __attribute__((packed));
        static_assert(std::is_pod<dex_header>::value, "");

        struct dex_class_def {
        private:
            uint32_t class_idx_;
            uint32_t access_flags_;
            uint32_t superclass_idx_;
            uint32_t interfaces_off_;
            uint32_t source_file_idx_;
            uint32_t annotations_off_;
            uint32_t class_data_off_;
            uint32_t static_vals_off_;

        public:
            dex_type_idx class_idx() const
            {
                return class_idx_;
            }

            dex_access_flags access_flags() const
            {
                return make_dex_access_flags(access_flags_);
            }

            dex_type_idx superclass_idx() const
            {
                return superclass_idx_;
            }

            uint32_t interfaces_off() const
            {
                return interfaces_off_;
            }

            dex_string_idx source_file_idx() const
            {
                return source_file_idx_;
            }

            uint32_t annotations_off() const
            {
                return annotations_off_;
            }
            uint32_t class_data_off() const
            {
                return class_data_off_;
            }

            uint32_t static_values_off() const
            {
                return static_vals_off_;
            }
        } __attribute__((packed));
        static_assert(std::is_pod<dex_class_def>::value, "");

        struct dex_code_header {
            uint16_t registers_size;
            uint16_t ins_size;
            uint16_t outs_size;
            uint16_t tries_size;
            uint32_t debug_info_off;
            uint32_t insns_size;
        } __attribute__((packed));
        static_assert(std::is_pod<dex_code_header>::value, "");

        struct dex_try_item {
            uint32_t start_addr;
            uint16_t insn_count;
            uint16_t handler_off;
        } __attribute__((packed));
        static_assert(std::is_pod<dex_try_item>::value, "");

        /// Format 10x: <tt>op</tt>.
        struct dex_fmt_10x {
            opcode op;
            uint8_t byte1;
        } __attribute__((packed));

        /// Format 12x: <tt>op vA, vB</tt>.
        struct dex_fmt_12x {
            opcode op;
            unsigned reg_a : 4;
            unsigned reg_b : 4;
        } __attribute__((packed));

        /// Format 11n: <tt>op vA, #+B</tt>.
        struct dex_fmt_11n {
            opcode op;
            unsigned reg_a : 4;
            signed imm_b : 4;
        } __attribute__((packed));

        /// Format 11x: <tt>op vAA</tt>.
        struct dex_fmt_11x {
            opcode op;
            uint8_t reg_a;
        } __attribute__((packed));

        /// Format 10t: <tt>op +AA</tt>.
        struct dex_fmt_10t {
            opcode op;
            int8_t roff_a;
        } __attribute__((packed));

        /// Format 20t: <tt>op +AAAA</tt>.
        struct dex_fmt_20t {
            opcode op;
            int8_t byte1;

            int16_t roff_a;
        } __attribute__((packed));

        /// Format 20bc: <tt>op AA, kind\@BBBB</tt>.
        struct dex_fmt_20bc {
            opcode op;
            uint8_t err_a;

            uint16_t idx_b;
        } __attribute__((packed));

        /// Format 22x: <tt>op vAA, vBBBB</tt>.
        struct dex_fmt_22x {
            opcode op;
            uint8_t reg_a;

            uint16_t reg_b;
        } __attribute__((packed));

        /// Format 21t: <tt>op vAA, +BBBB</tt>.
        struct dex_fmt_21t {
            opcode op;
            uint8_t reg_a;

            int16_t roff_b;
        } __attribute__((packed));

        /// Format 21s: <tt>op vAA, #+BBBB</tt>.
        struct dex_fmt_21s {
            opcode op;
            uint8_t reg_a;

            int16_t imm_b;
        } __attribute__((packed));

        /// Format 21h: <tt>op vAA, #+BBBB0000</tt> or
        /// <tt>op vAA, #+BBBB000000000000</tt>.
        struct dex_fmt_21h {
            opcode op;
            uint8_t reg_a;

            uint16_t imm_hi_b;
        } __attribute__((packed));

        /// Format 21c: <tt>op vAA, kind\@BBBB</tt>.
        struct dex_fmt_21c {
            opcode op;
            uint8_t reg_a;

            uint16_t idx_b;
        } __attribute__((packed));

        /// Format 23x: <tt>op vAA, vBB, vCC</tt>.
        struct dex_fmt_23x {
            opcode op;
            uint8_t reg_a;

            uint8_t reg_b;
            uint8_t reg_c;
        } __attribute__((packed));

        /// Format 22b: <tt>op vAA, vBB, #+CC</tt>.
        struct dex_fmt_22b {
            opcode op;
            uint8_t reg_a;

            uint8_t reg_b;
            int8_t imm_c;
        } __attribute__((packed));

        /// Format 22t: <tt>op vA, vB, +CCCC</tt>.
        struct dex_fmt_22t {
            opcode op;
            unsigned reg_a : 4;
            unsigned reg_b : 4;

            int16_t roff_c;
        } __attribute__((packed));

        /// Format 22s: <tt>op vA, vB, #+CCCC</tt>.
        struct dex_fmt_22s {
            opcode op;
            unsigned reg_a : 4;
            unsigned reg_b : 4;

            int16_t imm_c;
        } __attribute__((packed));

        /// Format 22c: <tt>op vA, vB, kind\@CCCC</tt>.
        struct dex_fmt_22c {
            opcode op;
            unsigned reg_a : 4;
            unsigned reg_b : 4;

            uint16_t idx_c;
        } __attribute__((packed));

        /// Format 22cs: <tt>op vA, vB, fieldoff\@CCCC</tt>.
        struct dex_fmt_22cs {
            opcode op;
            unsigned reg_a : 4;
            unsigned reg_b : 4;

            uint16_t off_c;
        } __attribute__((packed));

        /// Format 30t: <tt>op +AAAAAAAA</tt>.
        struct dex_fmt_30t {
            opcode op;
            int8_t byte1;

            int32_t roff_a;
        } __attribute__((packed));

        /// Format 32x: <tt>op vAAAA, vBBBB</tt>.
        struct dex_fmt_32x {
            opcode op;
            int8_t byte1;

            uint16_t reg_a;
            uint16_t reg_b;
        } __attribute__((packed));

        /// Format 31i: <tt>op vAA, #+BBBBBBBB</tt>.
        struct dex_fmt_31i {
            opcode op;
            uint8_t reg_a;

            int32_t imm_b;
        } __attribute__((packed));

        /// Format 31t: <tt>op vAA, +BBBBBBBB</tt>.
        struct dex_fmt_31t {
            opcode op;
            uint8_t reg_a;

            int32_t roff_b;
        } __attribute__((packed));

        /// Format 31c: <tt>op vAA, string\@BBBBBBBB</tt>.
        struct dex_fmt_31c {
            opcode op;
            uint8_t reg_a;

            uint32_t idx_b;
        } __attribute__((packed));

        /// Format 35c: <tt>op {vC, vD, vE, vF, vG}[size=A], meth\@BBBB</tt>.
        struct dex_fmt_35c {
            opcode op;
            unsigned reg_g : 4;
            unsigned size_a : 4;

            uint16_t idx_b;

            unsigned reg_c : 4;
            unsigned reg_d : 4;
            unsigned reg_e : 4;
            unsigned reg_f : 4;
        } __attribute__((packed));

        /// Format 35ms: <tt>op {vC, vD, vE, vF, vG}[size=A],
        /// vtaboff\@BBBB</tt>.
        struct dex_fmt_35ms {
            opcode op;
            unsigned reg_g : 4;
            unsigned size_a : 4;

            uint16_t off_b;

            unsigned reg_c : 4;
            unsigned reg_d : 4;
            unsigned reg_e : 4;
            unsigned reg_f : 4;
        } __attribute__((packed));

        /// Format 35mi: <tt>op {vC, vD, vE, vF, vG}[size=A], inline\@BBBB</tt>.
        struct dex_fmt_35mi {
            opcode op;
            unsigned reg_g : 4;
            unsigned size_a : 4;

            uint16_t off_b;

            unsigned reg_c : 4;
            unsigned reg_d : 4;
            unsigned reg_e : 4;
            unsigned reg_f : 4;
        } __attribute__((packed));

        /// Format 3rc: <tt>op {vCCCC - vNNNN}, kind\@BBBB</tt>.
        struct dex_fmt_3rc {
            opcode op;
            uint8_t size_a;

            uint16_t idx_b;

            uint16_t reg_c;
        } __attribute__((packed));

        /// Format 3rms: <tt>op {vCCCC - vNNNN}, vtaboff\@BBBB</tt>.
        struct dex_fmt_3rms {
            opcode op;
            uint8_t size_a;

            uint16_t off_b;

            uint16_t reg_c;
        } __attribute__((packed));

        /// Format 3rmi: <tt>op {vCCCC - vNNNN}, inline\@BBBB</tt>.
        struct dex_fmt_3rmi {
            opcode op;
            uint8_t size_a;

            uint16_t off_b;

            uint16_t reg_c;
        } __attribute__((packed));

        /// Format 51l: <tt>op vAA, #+BBBBBBBBBBBBBBBB</tt>.
        struct dex_fmt_51l {
            opcode op;
            uint8_t reg_a;

            int64_t imm_b;
        } __attribute__((packed));

        /// Format packed-switch-payload.
        ///
        /// The size is (size * 2) + 4.
        struct dex_fmt_packed_switch_payload {
            uint16_t ident;
            uint16_t size;
            int32_t first_key;
            int32_t targets[1]; // Struct hack.
        } __attribute__((packed));

        /// Format sparse-switch-payload.
        ///
        /// The size is (size * 4) + 2.
        struct dex_fmt_sparse_switch_payload {
            uint16_t ident;
            uint16_t size;
            int32_t keys_targets[1]; // Struct hack.
        } __attribute__((packed));

        /// Format fill-array-data-payload.
        ///
        /// The size is (size * element_width + 1) / 2 + 4.
        struct dex_fmt_fill_array_data_payload {
            uint16_t ident;
            uint16_t element_width;
            uint32_t size;
            uint8_t data[1]; // Struct hack.
        } __attribute__((packed));

        /// A raw instruction.
        union dex_raw_insn {
            opcode op;

            dex_fmt_10x fmt_10x;
            dex_fmt_12x fmt_12x;
            dex_fmt_11n fmt_11n;
            dex_fmt_11x fmt_11x;
            dex_fmt_10t fmt_10t;
            dex_fmt_20t fmt_20t;
            dex_fmt_20bc fmt_20bc;
            dex_fmt_22x fmt_22x;
            dex_fmt_21t fmt_21t;
            dex_fmt_21s fmt_21s;
            dex_fmt_21h fmt_21h;
            dex_fmt_21c fmt_21c;
            dex_fmt_23x fmt_23x;
            dex_fmt_22b fmt_22b;
            dex_fmt_22t fmt_22t;
            dex_fmt_22s fmt_22s;
            dex_fmt_22c fmt_22c;
            dex_fmt_22cs fmt_22cs;
            dex_fmt_30t fmt_30t;
            dex_fmt_32x fmt_32x;
            dex_fmt_31i fmt_31i;
            dex_fmt_31t fmt_31t;
            dex_fmt_31c fmt_31c;
            dex_fmt_35c fmt_35c;
            dex_fmt_35ms fmt_35ms;
            dex_fmt_35mi fmt_35mi;
            dex_fmt_3rc fmt_3rc;
            dex_fmt_3rms fmt_3rms;
            dex_fmt_3rmi fmt_3rmi;
            dex_fmt_51l fmt_51l;

            dex_fmt_packed_switch_payload fmt_packed_switch_payload;
            dex_fmt_sparse_switch_payload fmt_sparse_switch_payload;
            dex_fmt_fill_array_data_payload fmt_fill_array_data_payload;
        };
        static_assert(std::is_pod<dex_raw_insn>::value, "");
    }
}

#endif
