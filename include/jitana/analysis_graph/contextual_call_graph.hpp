#ifndef JITANA_CONTEXTUAL_CALL_GRAPH_HPP
#define JITANA_CONTEXTUAL_CALL_GRAPH_HPP

#include "jitana/jitana.hpp"

namespace jitana {
    using contextual_call_graph
            = boost::adjacency_list<boost::vecS, boost::vecS>;
}

#endif
