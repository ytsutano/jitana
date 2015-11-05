#ifndef JITANA_VARIABLE_HPP
#define JITANA_VARIABLE_HPP

#include "jitana/vm_core/idx.hpp"

#include <cstdint>
#include <iostream>

namespace jitana {
    /// A variable which is a register, a static field, or an instance field.
    ///
    /// It is actually a pair of register_idx and dex_field_idx. Either of them
    /// can take a value of idx_unknown. With this pair we can represent:
    ///
    /// - A register (when reg = register index, field = idx_unknown).
    /// - A static field (when reg = idx_unknown, field = static field index).
    /// - A instance field (when reg = pointer, field = instance field index).
    ///
    /// This concept of a "variable" allows us to write algorithms (such as
    /// dataflow analysis) without thinking about where the variable resides.
    struct variable {
        register_idx reg;
        dex_field_idx field;

        enum variable_kind {
            kind_unknown,
            kind_register,
            kind_instance_field,
            kind_static_field
        };

        variable() = default;

        variable(register_idx reg, dex_field_idx field) : reg(reg), field(field)
        {
        }

        variable(register_idx reg) : variable(reg, dex_field_idx::idx_unknown)
        {
        }

        variable(dex_field_idx field)
                : variable(register_idx::idx_unknown, field)
        {
        }

        variable_kind kind() const
        {
            if (reg.valid()) {
                return field.valid() ? kind_instance_field : kind_register;
            }
            else {
                return field.valid() ? kind_static_field : kind_unknown;
            }
        }

        bool valid() const
        {
            return reg.valid() || field.valid();
        }

        friend constexpr bool operator==(const variable& x, const variable& y)
        {
            return x.reg == y.reg && x.field == y.field;
        }

        friend constexpr bool operator!=(const variable& x, const variable& y)
        {
            return !(x == y);
        }

        friend constexpr bool operator<(const variable& x, const variable& y)
        {
            return x.field < y.field || (x.field == y.field && x.reg < y.reg);
        }

        friend constexpr bool operator>(const variable& x, const variable& y)
        {
            return y < x;
        }

        friend constexpr bool operator<=(const variable& x, const variable& y)
        {
            return !(y < x);
        }

        friend constexpr bool operator>=(const variable& x, const variable& y)
        {
            return !(x < y);
        }

        /// Prints the instance to the stream.
        friend std::ostream& operator<<(std::ostream& os, const variable& x)
        {
            switch (x.kind()) {
            case kind_register:
                os << x.reg;
                break;
            case kind_instance_field:
                os << x.reg << ".[" << x.field << "]";
                break;
            case kind_static_field:
                os << "[" << x.field << "]";
                break;
            default:
                os << "?";
            }
            return os;
        }
    };
    static_assert(std::is_pod<variable>::value, "");

    /// Creates a variable representing a register.
    inline variable make_register_variable(register_idx reg)
    {
        return {reg, dex_field_idx::idx_unknown};
    }

    /// Creates a variable representing a static field.
    inline variable make_static_field_variable(dex_field_idx field)
    {
        return {register_idx::idx_unknown, field};
    }

    /// Creates a variable representing a instance field.
    inline variable make_instance_field_variable(register_idx reg,
                                                 dex_field_idx field)
    {
        return {reg, field};
    }
}

namespace std {
    template <>
    struct hash<jitana::variable> {
        size_t operator()(const jitana::variable& var) const
        {
            return hash<decltype(var.reg)>()(var.reg)
                    * hash<decltype(var.field)>()(var.field);
        }
    };
}

#endif
