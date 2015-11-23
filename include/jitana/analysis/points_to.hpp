#ifndef JITANA_POINTS_TO_HPP
#define JITANA_POINTS_TO_HPP

#include "jitana/analysis_graph/pointer_assignment_graph.hpp"
#include "jitana/analysis_graph/contextual_call_graph.hpp"

namespace jitana {
    bool update_points_to_graphs(pointer_assignment_graph& pag,
                                 contextual_call_graph& cg, virtual_machine& vm,
                                 const method_vertex_descriptor& mv,
                                 bool on_the_fly_cg = true);
}

#endif
