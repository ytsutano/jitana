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
