#ifndef JITANA_SYMBOLIC_EXECUTION_HPP
#define JITANA_SYMBOLIC_EXECUTION_HPP

#include "jitana/jitana.hpp"
#include "jitana/graph/edge_filtered_graph.hpp"

#include <iostream>
#include <unordered_map>

#include <boost/graph/depth_first_search.hpp>
#include <boost/range/iterator_range.hpp>

// #include <stp/c_interface.h>

namespace jitana {
    void symbolic_execution(virtual_machine& vm,
                            const method_vertex_descriptor& v);
}

#endif
