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

#ifndef JITANA_GRAPH_COMMON_HPP
#define JITANA_GRAPH_COMMON_HPP

#include "jitana/vm_core/hdl.hpp"
#include "jitana/vm_core/access_flags.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/free.hpp>

BOOST_TYPE_ERASURE_FREE((has_print_graphviz_attr), print_graphviz_attr, 2)

namespace jitana {
    using any_edge_property = boost::type_erasure::
            any<boost::mpl::
                        vector<boost::type_erasure::copy_constructible<>,
                               boost::type_erasure::relaxed,
                               has_print_graphviz_attr<void(
                                       std::ostream&,
                                       const boost::type_erasure::_self&)>>>;
}

#endif
