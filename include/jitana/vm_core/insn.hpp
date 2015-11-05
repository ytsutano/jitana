#ifndef JITANA_INSN_HPP
#define JITANA_INSN_HPP

#include "jitana/vm_core/hdl.hpp"
#include "jitana/vm_core/idx.hpp"
#include "jitana/vm_core/variable.hpp"
#include "jitana/vm_core/insn_info.hpp"
#include "jitana/vm_core/opcode.hpp"
#include "jitana/algorithm/unique_sort.hpp"

#include <iostream>
#include <vector>
#include <array>
#include <type_traits>
#include <numeric>

#include <boost/variant.hpp>
#include <boost/mpl/vector/vector40.hpp>

namespace jitana {
    struct no_const_val {
        friend std::ostream& operator<<(std::ostream& os, const no_const_val& x)
        {
            return os;
        }
    };

    struct array_payload {
        size_t element_width;
        size_t size;
        std::vector<uint8_t> data;

        friend std::ostream& operator<<(std::ostream& os,
                                        const array_payload& x)
        {
            return os << "[" << x.element_width << "x" << x.data.size() << "]";
        }
    };

    template <size_t RegSize, typename ConstVal>
    struct insn_base {
        using reg_array_type = std::array<register_idx, RegSize>;

        opcode op;
        reg_array_type regs;
        ConstVal const_val;

        insn_base()
        {
        }

        insn_base(opcode op, reg_array_type regs, const ConstVal& const_val)
                : op(op), regs(regs), const_val(const_val)
        {
        }

        bool is_regs_range() const
        {
            return RegSize >= 3 && !regs[1].valid() && regs.front().valid()
                    && regs.back().valid() && regs.front() <= regs.back();
        }

        friend std::vector<variable> defs(const insn_base& x)
        {
            const auto& insn_info = info(x.op);
            std::vector<variable> result;

            if (x.op == opcode::op_check_cast) {
                return result;
            }

            if (RegSize > 0 && insn_info.sets_register()
                && !insn_info.sets_result()) {
                result.push_back(make_register_variable(x.regs[0]));
            }
            if (insn_info.sets_result()) {
                result.push_back(
                        make_register_variable(register_idx::idx_result));
            }
            unique_sort(result);
            return result;
        }

        friend std::vector<variable> uses(const insn_base& x)
        {
            std::vector<variable> result;
            const auto& regs = x.regs;
            if (x.is_regs_range()) {
                result.resize(static_cast<int32_t>(regs.back() - regs.front()));
                std::iota(begin(result), end(result), regs.front());
            }
            else if (RegSize > 0) {
                auto start = begin(regs);
                const auto& insn_info = info(x.op);
                if (insn_info.sets_register() && !insn_info.sets_result()
                    && !insn_info.sets_register_inplace()) {
                    ++start;
                }
                result.reserve(RegSize);
                std::copy_if(start, end(regs), std::back_inserter(result),
                             [](const variable& var) { return var.valid(); });
                unique_sort(result);
            }
            return result;
        }

        friend std::ostream& operator<<(std::ostream& os, const insn_base& x)
        {
            os << info(x.op).mnemonic();
            if (x.is_regs_range()) {
                os << " " << x.regs.front() << "-" << x.regs.back();
            }
            else {
                for (const auto& r : x.regs) {
                    if (r.valid()) {
                        os << " " << r;
                    }
                }
            }
            os << " [" << x.const_val << "]";
            return os;
        }
    };

    // clang-format off
    struct insn_nop              : insn_base<0, no_const_val  > { using insn_base::insn_base; };
    struct insn_move             : insn_base<2, no_const_val  > { using insn_base::insn_base; };
    struct insn_return           : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_const            : insn_base<1, int32_t       > { using insn_base::insn_base; };
    struct insn_const_wide       : insn_base<1, int64_t       > { using insn_base::insn_base; };
    struct insn_const_string     : insn_base<1, std::string   > { using insn_base::insn_base; };
    struct insn_const_class      : insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_monitor_enter    : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_monitor_exit     : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_check_cast       : insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_instance_of      : insn_base<2, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_array_length     : insn_base<2, no_const_val  > { using insn_base::insn_base; };
    struct insn_new_instance     : insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_new_array        : insn_base<2, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_filled_new_array : insn_base<5, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_fill_array_data  : insn_base<1, array_payload > { using insn_base::insn_base; };
    struct insn_throw            : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_goto             : insn_base<0, int32_t       > { using insn_base::insn_base; };
    struct insn_switch           : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_cmp              : insn_base<3, no_const_val  > { using insn_base::insn_base; };
    struct insn_if               : insn_base<2, no_const_val  > { using insn_base::insn_base; };
    struct insn_if_z             : insn_base<1, no_const_val  > { using insn_base::insn_base; };
    struct insn_aget             : insn_base<3, no_const_val  > { using insn_base::insn_base; };
    struct insn_aput             : insn_base<3, no_const_val  > { using insn_base::insn_base; };
    struct insn_iget             : insn_base<2, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_iget_quick       : insn_base<2, uint16_t      > { using insn_base::insn_base; };
    struct insn_iput             : insn_base<2, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_iput_quick       : insn_base<2, uint16_t      > { using insn_base::insn_base; };
    struct insn_sget             : insn_base<1, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_sput             : insn_base<1, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_invoke           : insn_base<5, dex_method_hdl> { using insn_base::insn_base; };
    struct insn_invoke_quick     : insn_base<5, uint16_t      > { using insn_base::insn_base; };
    struct insn_unary_arith_op   : insn_base<2, no_const_val  > { using insn_base::insn_base; };
    struct insn_binary_arith_op  : insn_base<3, no_const_val  > { using insn_base::insn_base; };
    struct insn_aug_assign_op    : insn_base<2, no_const_val  > { using insn_base::insn_base; };
    struct insn_const_arith_op   : insn_base<2, int16_t       > { using insn_base::insn_base; };

    struct insn_entry : insn_base<5, no_const_val> { using insn_base::insn_base; };
    struct insn_exit  : insn_base<0, no_const_val> { using insn_base::insn_base; };
    // clang-format on

    inline std::ostream& operator<<(std::ostream& os, const insn_entry& x)
    {
        return os << "ENTRY " << x.regs.front() << "-" << x.regs.back();
    }

    inline std::ostream& operator<<(std::ostream& os, const insn_exit& x)
    {
        return os << "EXIT";
    }

    inline std::vector<variable> defs(const insn_entry& x)
    {
        std::vector<variable> result;
        if (x.is_regs_range()) {
            const auto& regs = x.regs;
            result.resize(static_cast<int32_t>(regs.back() - regs.front() + 1));
            std::iota(begin(result), end(result), regs.front());
        }
        return result;
    }

    inline std::vector<variable> uses(const insn_entry& x)
    {
        return {};
    }

    inline std::vector<variable> defs(const insn_iput& x)
    {
        return {make_instance_field_variable(x.regs[1], x.const_val.idx)};
    }

    inline std::vector<variable> uses(const insn_iget& x)
    {
        std::vector<variable> result;
        result.push_back(make_register_variable(x.regs[1]));
        result.push_back(
                make_instance_field_variable(x.regs[1], x.const_val.idx));
        unique_sort(result);
        return result;
    }

    inline std::vector<variable> defs(const insn_sput& x)
    {
        return {make_static_field_variable(x.const_val.idx)};
    }

    inline std::vector<variable> uses(const insn_sget& x)
    {
        return {make_static_field_variable(x.const_val.idx)};
    }

    // clang-format off
    using insn = boost::make_variant_over<boost::mpl::vector38<
        insn_nop,
        insn_move,
        insn_return,
        insn_const,
        insn_const_wide,
        insn_const_string,
        insn_const_class,
        insn_monitor_enter,
        insn_monitor_exit,
        insn_check_cast,
        insn_instance_of,
        insn_array_length,
        insn_new_instance,
        insn_new_array,
        insn_filled_new_array,
        insn_fill_array_data,
        insn_throw,
        insn_goto,
        insn_switch,
        insn_cmp,
        insn_if,
        insn_if_z,
        insn_aget,
        insn_aput,
        insn_iget,
        insn_iget_quick,
        insn_iput,
        insn_iput_quick,
        insn_sget,
        insn_sput,
        insn_invoke,
        insn_invoke_quick,
        insn_unary_arith_op,
        insn_binary_arith_op,
        insn_aug_assign_op,
        insn_const_arith_op,
        insn_entry,
        insn_exit
    >>::type;
    // clang-format on

    namespace detail {
        struct extract_op : public boost::static_visitor<opcode> {
            template <typename T>
            opcode operator()(const T& x) const
            {
                return x.op;
            }
        };
    }

    inline opcode op(const insn& x)
    {
        return boost::apply_visitor(detail::extract_op(), x);
    }

    namespace detail {
        template <typename RetType>
        struct extract_const_val
                : public boost::static_visitor<const RetType*> {
            template <typename T>
            const RetType* operator()(const T& x) const
            {
                return std::is_same<decltype(x.const_val), RetType>::value
                        ? reinterpret_cast<const RetType*>(&x.const_val)
                        : nullptr;
            }
        };
    }

    template <typename RetType>
    inline const RetType* const_val(const insn& x)
    {
        return boost::apply_visitor(detail::extract_const_val<RetType>(), x);
    }

    namespace detail {
        struct apply_defs
                : public boost::static_visitor<std::vector<variable>> {
            template <typename T>
            std::vector<variable> operator()(const T& x) const
            {
                return defs(x);
            }
        };
    }

    inline std::vector<variable> defs(const insn& x)
    {
        return boost::apply_visitor(detail::apply_defs(), x);
    }

    namespace detail {
        struct apply_uses
                : public boost::static_visitor<std::vector<variable>> {
            template <typename T>
            std::vector<variable> operator()(const T& x) const
            {
                return uses(x);
            }
        };
    }

    inline std::vector<variable> uses(const insn& x)
    {
        return boost::apply_visitor(detail::apply_uses(), x);
    }

    namespace detail {
        struct check_pseudo : public boost::static_visitor<bool> {
            template <typename T>
            constexpr bool operator()(const T&) const
            {
                return std::is_same<T, insn_entry>::value
                        || std::is_same<T, insn_exit>::value;
            }
        };
    }

    inline bool is_pseudo(const insn& x)
    {
        return boost::apply_visitor(detail::check_pseudo(), x);
    }
}

#endif
