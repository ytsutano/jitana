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

#ifndef JITANA_INSN_HPP
#define JITANA_INSN_HPP

#include "jitana/vm_core/hdl.hpp"
#include "jitana/vm_core/idx.hpp"
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
#include <boost/range/iterator_range.hpp>

namespace jitana {
    namespace detail {
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

            friend std::vector<register_idx> defs(const insn_base& x)
            {
                const auto& insn_info = info(x.op);
                std::vector<register_idx> result;

                if (RegSize > 0 && insn_info.sets_register()
                    && !insn_info.sets_result()) {
                    result.push_back(x.regs[0]);
                }
                if (insn_info.sets_result()) {
                    result.push_back(register_idx::idx_result);
                }
                unique_sort(result);
                return result;
            }

            friend std::vector<register_idx> uses(const insn_base& x)
            {
                std::vector<register_idx> result;
                const auto& regs = x.regs;
                if (x.is_regs_range()) {
                    result.resize(
                            static_cast<int32_t>(regs.back() - regs.front()));
                    std::iota(begin(result), end(result), regs.front());
                }
                if (RegSize > 0) {
                    auto start = begin(regs);
                    const auto& insn_info = info(x.op);
                    if (insn_info.sets_register() && !insn_info.sets_result()
                        && !insn_info.sets_register_inplace()) {
                        ++start;
                    }
                    result.reserve(RegSize);
                    std::copy(start, end(regs), std::back_inserter(result));
                    unique_sort(result);
                }
                return result;
            }

            friend std::vector<register_idx> regs_vec(const insn_base& x)
            {
                std::vector<register_idx> result;
                const auto& regs = x.regs;
                if (x.is_regs_range()) {
                    result.resize(
                            static_cast<int32_t>(regs.back() - regs.front()));
                    std::iota(begin(result), end(result), regs.front());
                }
                else {
                    std::copy_if(begin(regs), end(regs),
                                 std::back_inserter(result),
                                 [](const auto& v) { return v.valid(); });
                }
                return result;
            }

            friend bool operator==(const insn_base& x, const insn_base& y)
            {
                return std::tie(x.op, x.regs, x.const_val)
                        == std::tie(y.op, y.regs, y.const_val);
            }

            friend bool operator!=(const insn_base& x, const insn_base& y)
            {
                return !(x == y);
            }

            friend bool operator<(const insn_base& x, const insn_base& y)
            {
                return std::tie(x.op, x.regs, x.const_val)
                        < std::tie(y.op, y.regs, y.const_val);
            }

            friend bool operator>(const insn_base& x, const insn_base& y)
            {
                return y < x;
            }

            friend bool operator<=(const insn_base& x, const insn_base& y)
            {
                return !(y < x);
            }

            friend bool operator>=(const insn_base& x, const insn_base& y)
            {
                return !(x < y);
            }

            friend std::ostream& operator<<(std::ostream& os,
                                            const insn_base& x)
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
    }

    struct array_payload {
        size_t element_width;
        size_t size;
        std::vector<uint8_t> data;

        friend bool operator==(const array_payload& x, const array_payload& y)
        {
            return x.data == y.data;
        }

        friend bool operator!=(const array_payload& x, const array_payload& y)
        {
            return !(x == y);
        }

        friend bool operator<(const array_payload& x, const array_payload& y)
        {
            return x.data < y.data;
        }

        friend bool operator>(const array_payload& x, const array_payload& y)
        {
            return y < x;
        }

        friend bool operator<=(const array_payload& x, const array_payload& y)
        {
            return !(y < x);
        }

        friend bool operator>=(const array_payload& x, const array_payload& y)
        {
            return !(x < y);
        }

        friend std::ostream& operator<<(std::ostream& os,
                                        const array_payload& x)
        {
            return os << "[" << x.element_width << "x" << x.data.size() << "]";
        }
    };

    // clang-format off
    struct insn_nop              : detail::insn_base<0, boost::blank  > { using insn_base::insn_base; };
    struct insn_move             : detail::insn_base<2, boost::blank  > { using insn_base::insn_base; };
    struct insn_return           : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_const            : detail::insn_base<1, int32_t       > { using insn_base::insn_base; };
    struct insn_const_wide       : detail::insn_base<1, int64_t       > { using insn_base::insn_base; };
    struct insn_const_string     : detail::insn_base<1, std::string   > { using insn_base::insn_base; };
    struct insn_const_class      : detail::insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_monitor_enter    : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_monitor_exit     : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_check_cast       : detail::insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_instance_of      : detail::insn_base<2, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_array_length     : detail::insn_base<2, boost::blank  > { using insn_base::insn_base; };
    struct insn_new_instance     : detail::insn_base<1, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_new_array        : detail::insn_base<2, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_filled_new_array : detail::insn_base<5, dex_type_hdl  > { using insn_base::insn_base; };
    struct insn_fill_array_data  : detail::insn_base<1, array_payload > { using insn_base::insn_base; };
    struct insn_throw            : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_goto             : detail::insn_base<0, int32_t       > { using insn_base::insn_base; };
    struct insn_switch           : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_cmp              : detail::insn_base<3, boost::blank  > { using insn_base::insn_base; };
    struct insn_if               : detail::insn_base<2, boost::blank  > { using insn_base::insn_base; };
    struct insn_if_z             : detail::insn_base<1, boost::blank  > { using insn_base::insn_base; };
    struct insn_aget             : detail::insn_base<3, boost::blank  > { using insn_base::insn_base; };
    struct insn_aput             : detail::insn_base<3, boost::blank  > { using insn_base::insn_base; };
    struct insn_iget             : detail::insn_base<2, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_iget_quick       : detail::insn_base<2, uint16_t      > { using insn_base::insn_base; };
    struct insn_iput             : detail::insn_base<2, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_iput_quick       : detail::insn_base<2, uint16_t      > { using insn_base::insn_base; };
    struct insn_sget             : detail::insn_base<1, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_sput             : detail::insn_base<1, dex_field_hdl > { using insn_base::insn_base; };
    struct insn_invoke           : detail::insn_base<5, dex_method_hdl> { using insn_base::insn_base; };
    struct insn_invoke_quick     : detail::insn_base<5, uint16_t      > { using insn_base::insn_base; };
    struct insn_unary_arith_op   : detail::insn_base<2, boost::blank  > { using insn_base::insn_base; };
    struct insn_binary_arith_op  : detail::insn_base<3, boost::blank  > { using insn_base::insn_base; };
    struct insn_aug_assign_op    : detail::insn_base<2, boost::blank  > { using insn_base::insn_base; };
    struct insn_const_arith_op   : detail::insn_base<2, int16_t       > { using insn_base::insn_base; };

    struct insn_entry : detail::insn_base<5, boost::blank> { using insn_base::insn_base; };
    struct insn_exit  : detail::insn_base<1, boost::blank> { using insn_base::insn_base; };
    // clang-format on

    inline std::ostream& operator<<(std::ostream& os, const insn_entry& x)
    {
        return os << "ENTRY " << x.regs.front() << "-" << x.regs.back();
    }

    inline std::ostream& operator<<(std::ostream& os, const insn_exit& x)
    {
        os << "EXIT";
        for (const auto& r : x.regs) {
            if (r.valid()) {
                os << " " << r;
            }
        }
        return os;
    }

    inline std::vector<register_idx> defs(const insn_entry& x)
    {
        std::vector<register_idx> result;
        if (x.is_regs_range()) {
            const auto& regs = x.regs;
            result.resize(static_cast<int32_t>(regs.back() - regs.front() + 1));
            std::iota(begin(result), end(result), regs.front());
        }
        return result;
    }

    inline std::vector<register_idx> uses(const insn_entry&)
    {
        return {};
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
        struct extract_regs
                : public boost::static_visitor<
                          boost::iterator_range<const register_idx*>> {
            template <typename T>
            boost::iterator_range<const register_idx*>
            operator()(const T& x) const
            {
                return boost::make_iterator_range(begin(x.regs), end(x.regs));
            }
        };
    }

    inline boost::iterator_range<const register_idx*> regs(const insn& x)
    {
        return boost::apply_visitor(detail::extract_regs(), x);
    }

    namespace detail {
        struct extract_regs_vec
                : public boost::static_visitor<std::vector<register_idx>> {
            template <typename T>
            std::vector<register_idx> operator()(const T& x) const
            {
                return regs_vec(x);
            }
        };
    }

    inline std::vector<register_idx> regs_vec(const insn& x)
    {
        return boost::apply_visitor(detail::extract_regs_vec(), x);
    }

    namespace detail {
        template <typename RetType>
        struct extract_const_val
                : public boost::static_visitor<const RetType*> {
            template <typename T>
            const RetType* operator()(const T& x) const
            {
                return std::is_convertible<decltype(x.const_val),
                                           RetType>::value
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
                : public boost::static_visitor<std::vector<register_idx>> {
            template <typename T>
            std::vector<register_idx> operator()(const T& x) const
            {
                return defs(x);
            }
        };
    }

    inline std::vector<register_idx> defs(const insn& x)
    {
        return boost::apply_visitor(detail::apply_defs(), x);
    }

    namespace detail {
        struct apply_uses
                : public boost::static_visitor<std::vector<register_idx>> {
            template <typename T>
            std::vector<register_idx> operator()(const T& x) const
            {
                return uses(x);
            }
        };
    }

    inline std::vector<register_idx> uses(const insn& x)
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
