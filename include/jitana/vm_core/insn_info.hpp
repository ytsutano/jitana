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

#ifndef JITANA_INSN_INFO_HPP
#define JITANA_INSN_INFO_HPP

#include "jitana/vm_core/opcode.hpp"

#include <cstddef>
#include <cstdint>

namespace jitana {
    namespace detail {
        /// A set of instruction properties stored as a bit field. This will be
        /// used as a part of the insn_info class for lookup.
        enum class insn_props : uint16_t {
            none = 0,
            can_throw = 1 << 0,
            odex_only = 1 << 1,
            can_continue = 1 << 2,
            sets_result = 1 << 3,
            sets_register = 1 << 4,
            sets_wide_register = 1 << 5,
            reads_wide_register = 1 << 6,
            odexed_instance_quick = 1 << 7,
            odexed_instance_volatile = 1 << 8,
            odexed_static_volatile = 1 << 9,
            can_initialize_reference = 1 << 10,
            can_return = 1 << 11,
            can_branch = 1 << 11,
            can_switch = 1 << 12,
            can_invoke = 1 << 13
        };

        /// Returns logical OR.
        constexpr insn_props operator|(insn_props lhs, insn_props rhs)
        {
            return insn_props(uint16_t(lhs) | uint16_t(rhs));
        }

        /// Returns logical AND.
        constexpr insn_props operator&(insn_props lhs, insn_props rhs)
        {
            return insn_props(uint16_t(lhs) & uint16_t(rhs));
        }

        /// Returns true if the properties has the mask bits on; false,
        /// otherwise.
        constexpr bool has(insn_props properties, insn_props mask)
        {
            return (properties & mask) != insn_props::none;
        }
    }

    /// A format ID.
    enum class insn_fmt_id {
        fmt_00x,
        fmt_10x,
        fmt_12x,
        fmt_11n,
        fmt_11x,
        fmt_10t,
        fmt_20t,
        fmt_20bc,
        fmt_22x,
        fmt_21t,
        fmt_21s,
        fmt_21h,
        fmt_21c,
        fmt_23x,
        fmt_22b,
        fmt_22t,
        fmt_22s,
        fmt_22c,
        fmt_22cs,
        fmt_30t,
        fmt_32x,
        fmt_31i,
        fmt_31t,
        fmt_31c,
        fmt_35c,
        fmt_35ms,
        fmt_35mi,
        fmt_3rc,
        fmt_3rms,
        fmt_3rmi,
        fmt_51l
    };

    class insn_info {
    private:
        const opcode op_;
        const char* mnemonic_;
        const insn_fmt_id fmt_id_;
        const uint8_t size_;
        const detail::insn_props props_;

    public:
        insn_info(opcode op, const char* mnemonic, insn_fmt_id fmt_id,
                  uint8_t size, detail::insn_props props)
                : op_(op),
                  mnemonic_(mnemonic),
                  fmt_id_(fmt_id),
                  size_(size),
                  props_(props)
        {
        }

        opcode op() const
        {
            return op_;
        }

        const char* mnemonic() const
        {
            return mnemonic_;
        }

        insn_fmt_id format_id() const
        {
            return fmt_id_;
        }

        uint8_t size() const
        {
            return size_;
        }

        /// Returns true if the instruction can throw an exception; false,
        /// otherwise.
        bool can_throw() const
        {
            return has(props_, detail::insn_props::can_throw);
        }

        /// Returns true if the instruction is used in ODEX file only; false,
        /// otherwise.
        bool odex_only() const
        {
            return has(props_, detail::insn_props::odex_only);
        }

        /// Returns true if the instruction can continue to its next
        /// instruction; false, otherwise.
        bool can_continue() const
        {
            return has(props_, detail::insn_props::can_continue);
        }

        /// Returns true if the instruction sets the result register; false,
        /// otherwise.
        bool sets_result() const
        {
            return has(props_, detail::insn_props::sets_result);
        }

        /// Returns true if the instruction sets register(s); false, otherwise.
        bool sets_register() const
        {
            return has(props_, detail::insn_props::sets_register);
        }

        bool sets_register_inplace() const
        {
            switch (op_) {
            case opcode::op_check_cast:
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
                return true;
            default:
                return false;
            }
        }

        /// Returns true if the instruction sets wide register(s); false,
        /// otherwise.
        bool sets_wide_register() const
        {
            return has(props_, detail::insn_props::sets_wide_register);
        }

        /// Returns true if the instruction reads wide register(s); false,
        /// otherwise.
        bool reads_wide_register() const
        {
            return has(props_, detail::insn_props::reads_wide_register);
        }

        /// Returns true if the instruction is ODEX quick; false, otherwise.
        bool odexed_instance_quick() const
        {
            return has(props_, detail::insn_props::odexed_instance_quick);
        }

        /// Returns true if the instruction is ODEX volatile; false,
        /// otherwise.
        bool odexed_instance_volatile() const
        {
            return has(props_, detail::insn_props::odexed_instance_volatile);
        }

        /// Returns true if the instruction is ODEX static volatile; false,
        /// otherwise.
        bool odexed_static_volatile() const
        {
            return has(props_, detail::insn_props::odexed_static_volatile);
        }

        /// Returns true if the instruction can initialize a reference; false,
        /// otherwise.
        bool can_initialize_reference() const
        {
            return has(props_, detail::insn_props::can_initialize_reference);
        }

        /// Returns true if the instruction can return from the method; false,
        /// otherwise.
        bool can_return() const
        {
            return has(props_, detail::insn_props::can_return);
        }

        /// Returns true if the instruction can branch; false, otherwise.
        bool can_branch() const
        {
            return has(props_, detail::insn_props::can_branch);
        }

        /// Returns true if the instruction can switch; false, otherwise.
        bool can_switch() const
        {
            return has(props_, detail::insn_props::can_switch);
        }

        /// Returns true if the instruction can invoke; false, otherwise.
        bool can_invoke() const
        {
            return has(props_, detail::insn_props::can_invoke);
        }

        /// Returns true if the instruction can virtually invoke; false,
        /// otherwise.
        bool can_virtually_invoke() const
        {
            switch (op_) {
            case opcode::op_invoke_virtual:
            case opcode::op_invoke_interface:
            case opcode::op_invoke_virtual_range:
            case opcode::op_invoke_interface_range:
            case opcode::op_invoke_virtual_quick:
            case opcode::op_invoke_virtual_quick_range:
                return true;
            default:
                return false;
            }
        }

        /// Returns true if the instruction can directly invoke; false,
        /// otherwise.
        bool can_directly_invoke() const
        {
            switch (op_) {
            case opcode::op_invoke_super:
            case opcode::op_invoke_direct:
            case opcode::op_invoke_static:
            case opcode::op_invoke_super_range:
            case opcode::op_invoke_direct_range:
            case opcode::op_invoke_static_range:
            case opcode::op_invoke_object_init_range:
            case opcode::op_execute_inline:
            case opcode::op_invoke_super_quick:
            case opcode::op_execute_inline_range:
            case opcode::op_invoke_super_quick_range:
                return true;
            default:
                return false;
            }
        }
    };

    /// Returns the information about the instruction with the opcode.
    const insn_info& info(opcode op);
}

#endif
