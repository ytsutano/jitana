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

#ifndef JITANA_ACCESS_FLAGS_HPP
#define JITANA_ACCESS_FLAGS_HPP

#include <cstdint>
#include <ostream>

namespace jitana {
    /// A set of DEX access flags.
    ///
    /// See https://source.android.com/devices/tech/dalvik/dex-format.html.
    enum dex_access_flags {
        acc_public = 0x1,
        acc_private = 0x2,
        acc_protected = 0x4,
        acc_static = 0x8,
        acc_final = 0x10,
        acc_synchronized = 0x20,
        acc_volatile = 0x40,
        acc_bridge = 0x40,
        acc_transient = 0x80,
        acc_varargs = 0x80,
        acc_native = 0x100,
        acc_interface = 0x200,
        acc_abstract = 0x400,
        acc_strict = 0x800,
        acc_synthetic = 0x1000,
        acc_annotation = 0x2000,
        acc_enum = 0x4000,
        acc_constructor = 0x10000,
        acc_declared_synchronized = 0x20000
    };

    inline dex_access_flags make_dex_access_flags(uint32_t x)
    {
        return static_cast<dex_access_flags>(x);
    }

    inline std::ostream& operator<<(std::ostream& os, dex_access_flags x)
    {
        bool printed = false;

        auto print_flag = [&](dex_access_flags flag, const char* name) {
            if (x & flag) {
                if (printed) {
                    os << " ";
                }
                os << name;
                printed = true;
            }
        };

        print_flag(acc_public, "public");
        print_flag(acc_private, "private");
        print_flag(acc_protected, "protected");
        print_flag(acc_static, "static");
        print_flag(acc_final, "final");
        print_flag(acc_synchronized, "synchronized");
        print_flag(acc_volatile, "volatile");
        print_flag(acc_bridge, "bridge");
        print_flag(acc_transient, "transient");
        print_flag(acc_varargs, "varargs");
        print_flag(acc_native, "native");
        print_flag(acc_interface, "interface");
        print_flag(acc_abstract, "abstract");
        print_flag(acc_strict, "strict");
        print_flag(acc_synthetic, "synthetic");
        print_flag(acc_annotation, "annotation");
        print_flag(acc_enum, "enum");
        print_flag(acc_constructor, "constructor");
        print_flag(acc_declared_synchronized, "declared_synchronized");

        return os;
    }
}

#endif
