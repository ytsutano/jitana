#ifndef JITANA_IDX_HPP
#define JITANA_IDX_HPP

#include <cstdint>
#include <iostream>

namespace jitana {
    /// A template struct for index.
    ///
    /// Follows the Curiously Recurring Template Pattern (CRTP) because we do
    /// not need runtime polymorphic behavior and want to keep it a POD so that
    /// it can be used for the DEX file parsing.
    ///
    /// An index to an array typically has type size_t which is fine for most
    /// cases. In the DEX file, however, we heavily use the indexes to point to
    /// different kinds of IDs such as string_id and type_id. Making the index
    /// strongly typed allows static type checking and overloading possible just
    /// like using iterators.
    ///
    /// In the DEX file, we use the value of <tt>0xffffffff</tt> to indicate
    /// that the index is invalid. This value is -1 if interpreted as a two's
    /// complement 32-bit signed integer. After examining its use in Dalvik, I
    /// have decided to interpret the raw type to be int32_t and consider all
    /// negative numbers invalid for simplicity. This also allows us to use
    /// negative numbers other than -1 to keep special meanings (e.g., return
    /// register).
    template <typename Derived>
    struct idx_base {
        /// The index value.
        int32_t value;

        enum { idx_unknown = -1 };

        /// Keep the default constructor to keep this type a POD.
        idx_base() = default;

        /// Creates an idx_base<Derived> instance with the specified index.
        idx_base(int32_t index) : value(index)
        {
        }

        explicit operator int32_t() const
        {
            return value;
        }

        explicit operator uint16_t() const
        {
            return static_cast<uint16_t>(value);
        }

        bool valid() const
        {
            return value != idx_unknown;
        }

        Derived& operator=(int32_t index)
        {
            value = index;
            return *this;
        }

        Derived& operator+=(const Derived& x)
        {
            value += x.value;
            return *static_cast<Derived*>(this);
        }

        Derived& operator++()
        {
            ++value;
            return *static_cast<Derived*>(this);
        }

        friend Derived operator+(const Derived& x, const Derived& y)
        {
            idx_base result = x;
            return result += y;
        }

        Derived& operator-=(const Derived& x)
        {
            value -= x.value;
            return *static_cast<Derived*>(this);
        }

        Derived& operator--()
        {
            --value;
            return *static_cast<Derived*>(this);
        }

        friend Derived operator-(const Derived& x, const Derived& y)
        {
            idx_base result = x;
            return result -= y;
        }

        friend constexpr bool operator==(const idx_base& x, const idx_base& y)
        {
            return x.value == y.value;
        }

        friend constexpr bool operator!=(const idx_base& x, const idx_base& y)
        {
            return !(x == y);
        }

        friend constexpr bool operator<(const idx_base& x, const idx_base& y)
        {
            return x.value < y.value;
        }

        friend constexpr bool operator>(const idx_base& x, const idx_base& y)
        {
            return y < x;
        }

        friend constexpr bool operator<=(const idx_base& x, const idx_base& y)
        {
            return !(y < x);
        }

        friend constexpr bool operator>=(const idx_base& x, const idx_base& y)
        {
            return !(x < y);
        }

        /// Prints the instance to the stream.
        friend std::ostream& operator<<(std::ostream& os, const idx_base& x)
        {
            return os << x.value;
        }
    };

    // clang-format off
    struct dex_field_idx  : idx_base<dex_field_idx>  { using idx_base::idx_base; };
    struct dex_string_idx : idx_base<dex_string_idx> { using idx_base::idx_base; };
    struct dex_type_idx   : idx_base<dex_type_idx>   { using idx_base::idx_base; };
    struct dex_method_idx : idx_base<dex_method_idx> { using idx_base::idx_base; };
    struct dex_proto_idx  : idx_base<dex_proto_idx>  { using idx_base::idx_base; };
    struct dex_vtab_idx   : idx_base<dex_vtab_idx>   { using idx_base::idx_base; };
    struct dex_insn_idx   : idx_base<dex_insn_idx>   { using idx_base::idx_base; };
    // clang-format on

    /// A register index.
    struct register_idx : idx_base<register_idx> {
        using idx_base::idx_base;

        enum {
            /// The index to the to the result register.
            idx_result = -2,

            /// The index to the to the exception register.
            idx_exception = -3,
        };

        bool is_result() const
        {
            return value == idx_result;
        }

        bool is_exception() const
        {
            return value == idx_exception;
        }

        friend std::ostream& operator<<(std::ostream& os, const register_idx& x)
        {
            switch (x.value) {
            case idx_unknown:
                os << "v?";
                break;
            case idx_result:
                os << "vR";
                break;
            case idx_exception:
                os << "vE";
                break;
            default:
                os << "v" << x.value;
            }
            return os;
        }
    };
    static_assert(std::is_pod<register_idx>::value, "");
}

namespace std {
    template <>
    struct hash<jitana::register_idx> {
        size_t operator()(const jitana::register_idx& idx) const
        {
            return static_cast<size_t>(idx.value);
        }
    };

    template <>
    struct hash<jitana::dex_field_idx> {
        size_t operator()(const jitana::dex_field_idx& idx) const
        {
            return static_cast<size_t>(idx.value);
        }
    };
}

#endif
