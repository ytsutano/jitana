#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "jitana/core/virtual_machine.hpp"
#include "jitana/core/dex_file.hpp"

using namespace jitana;
using namespace jitana::detail;

loader_vertex_descriptor virtual_machine::add_loader(class_loader loader)
{
    loader_vertex_property vp;
    vp.loader = std::move(loader);
    return add_vertex(vp, loaders_);
}

loader_vertex_descriptor
virtual_machine::add_loader(class_loader loader,
                            const class_loader_hdl& parent_hdl)
{
    auto v = add_loader(loader);
    auto p = find_loader_vertex(parent_hdl, loaders_);
    add_edge(v, *p, loader_parent_edge_property(), loaders_);
    return v;
}

boost::optional<class_vertex_descriptor>
virtual_machine::find_class(const jvm_type_hdl& hdl, bool try_load)
{
    // Check the lookup table first.
    if (auto cv = lookup_class_vertex(hdl, classes_)) {
        return cv;
    }

    // Find the starting class loader vertex.
    auto lv = find_loader_vertex(hdl.loader_hdl, loaders_);
    if (!lv) {
        // Invalid loader handle.
        return boost::none;
    }

    // Find the class.
    auto found_class = boost::optional<class_vertex_descriptor>{};
    auto color_map = boost::vector_property_map<int>{
            static_cast<unsigned>(num_vertices(loaders_))};
    auto term_func = [&](loader_vertex_descriptor v, const loader_graph& g) {
        found_class = try_load
                ? g[v].loader.load_class(*this, hdl.descriptor)
                : g[v].loader.lookup_class(*this, hdl.descriptor);
        return found_class;
    };
    boost::depth_first_visit(loaders_, *lv, boost::default_dfs_visitor{},
                             color_map, term_func);

    if (found_class) {
        // Remember the initiating handle.
        classes_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = *found_class;
    }

    return found_class;
}

boost::optional<class_vertex_descriptor>
virtual_machine::find_class(const dex_type_hdl& hdl, bool try_load)
{
    // Check the lookup table first.
    if (auto cv = lookup_class_vertex(hdl, classes_)) {
        return cv;
    }

    auto found_class = find_class(jvm_hdl(hdl), try_load);

    if (found_class) {
        // Remember the initiating handle.
        classes_[boost::graph_bundle].hdl_to_vertex[hdl] = *found_class;
    }

    return found_class;
}

boost::optional<method_vertex_descriptor>
virtual_machine::find_method(const jvm_method_hdl& hdl, bool try_load)
{
    // TODO: Rewrite to make it more efficient.

    // Check the lookup table first.
    if (auto mv = lookup_method_vertex(hdl, methods_)) {
        return mv;
    }

    if (try_load) {
        if (!find_class(hdl.type_hdl, true)) {
            return boost::none;
        }

        // Check the lookup table again.
        if (auto mv = lookup_method_vertex(hdl, methods_)) {
            return mv;
        }
    }

    // Find the starting class loader vertex.
    auto lv = find_loader_vertex(hdl.type_hdl.loader_hdl, loaders_);
    if (!lv) {
        // Invalid loader handle.
        return boost::none;
    }

    // Find the method.
    auto found_method = boost::optional<method_vertex_descriptor>{};
    auto color_map = boost::vector_property_map<int>{
            static_cast<unsigned>(num_vertices(loaders_))};
    auto term_func = [&](loader_vertex_descriptor v, const loader_graph& g) {
        found_method = g[v].loader.lookup_method(*this, hdl.type_hdl.descriptor,
                                                 hdl.unique_name);
        return found_method;
    };
    boost::depth_first_visit(loaders_, *lv, boost::default_dfs_visitor(),
                             color_map, term_func);

    if (found_method) {
        // Remember the initiating handle.
        methods_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = *found_method;
    }

    return found_method;
}

boost::optional<method_vertex_descriptor>
virtual_machine::find_method(const dex_method_hdl& hdl, bool try_load)
{
    // Check the lookup table first.
    if (auto mv = lookup_method_vertex(hdl, methods_)) {
        return mv;
    }

    auto found_method = find_method(jvm_hdl(hdl), try_load);

    if (found_method) {
        // Remember the initiating handle.
        methods_[boost::graph_bundle].hdl_to_vertex[hdl] = *found_method;
    }

    return found_method;
}

boost::optional<field_vertex_descriptor>
virtual_machine::find_field(const jvm_field_hdl& hdl, bool try_load)
{
    // TODO: Rewrite to make it more efficient.

    // Check the lookup table first.
    if (auto fv = lookup_field_vertex(hdl, fields_)) {
        return fv;
    }

    if (try_load) {
        if (!find_class(hdl.type_hdl, true)) {
            return boost::none;
        }

        // Check the lookup table again.
        if (auto fv = lookup_field_vertex(hdl, fields_)) {
            return fv;
        }
    }

    // Find the starting class loader vertex.
    auto lv = find_loader_vertex(hdl.type_hdl.loader_hdl, loaders_);
    if (!lv) {
        // Invalid loader handle.
        return boost::none;
    }

    // Find the field.
    auto found_field = boost::optional<field_vertex_descriptor>{};
    auto color_map = boost::vector_property_map<int>{
            static_cast<unsigned>(num_vertices(loaders_))};
    auto term_func = [&](loader_vertex_descriptor v, const loader_graph& g) {
        found_field = g[v].loader.lookup_field(*this, hdl.type_hdl.descriptor,
                                               hdl.unique_name);
        return found_field;
    };
    boost::depth_first_visit(loaders_, *lv, boost::default_dfs_visitor(),
                             color_map, term_func);

    if (found_field) {
        // Remember the initiating handle.
        fields_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = *found_field;
    }

    return found_field;
}

boost::optional<field_vertex_descriptor>
virtual_machine::find_field(const dex_field_hdl& hdl, bool try_load)
{
    // Check the lookup table first.
    if (auto fv = lookup_field_vertex(hdl, fields_)) {
        return fv;
    }

    auto found_field = find_field(jvm_hdl(hdl), try_load);

    if (found_field) {
        // Remember the initiating handle.
        fields_[boost::graph_bundle].hdl_to_vertex[hdl] = *found_field;
    }

    return found_field;
}

boost::optional<std::pair<method_vertex_descriptor, insn_vertex_descriptor>>
virtual_machine::find_insn(const dex_file_hdl& file_hdl, uint32_t offset,
                           bool try_load)
{
    auto lv = find_loader_vertex(file_hdl.loader_hdl, loaders_);
    if (!lv) {
        std::cerr << "loader not found\n";
        return boost::none;
    }

    if (loaders_[*lv].loader.dex_files().size() <= file_hdl.idx) {
        std::cerr << "bad index\n";
        return boost::none;
    }

    auto& dex_file = loaders_[*lv].loader.dex_files()[file_hdl.idx];
    auto p = dex_file.find_method_hdl(offset);
    if (!p) {
        std::cerr << "method_hdl not found\n";
        return boost::none;
    }

    auto method_hdl = p->first;
    auto local_off = p->second;

    auto mv = find_method(method_hdl, try_load);
    if (!mv) {
        std::cerr << "method vertex not found\n";
        return boost::none;
    }

    const auto& ig = methods_[*mv].insns;
    auto iv = lookup_insn_vertex(local_off, ig);
    if (!iv) {
        std::cerr << "insn vertex not found (";
        std::cerr << "method_hdl=" << method_hdl << ", ";
        std::cerr << "offset=" << offset << ", ";
        std::cerr << "insns_off=" << ig[boost::graph_bundle].insns_off << ", ";
        std::cerr << "local_off=" << local_off << ")\n";
        return boost::none;
    }

    return std::make_pair(*mv, *iv);
}

bool virtual_machine::load_all_classes(const class_loader_hdl& loader_hdl)
{
    // Find the starting class loader vertex.
    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        throw std::runtime_error("invalid loader handle");
    }

    return loaders_[*lv].loader.load_all_classes(*this);
}

jvm_type_hdl virtual_machine::jvm_hdl(const dex_type_hdl& type_hdl) const
{
    const auto& loader_hdl = type_hdl.file_hdl.loader_hdl;

    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        throw std::runtime_error("invalid type handle");
    }

    return {loader_hdl, loaders_[*lv].loader.descriptor(type_hdl)};
}

jvm_method_hdl virtual_machine::jvm_hdl(const dex_method_hdl& method_hdl) const
{
    const auto& loader_hdl = method_hdl.file_hdl.loader_hdl;

    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        throw std::runtime_error("invalid method handle");
    }

    const auto& loader = loaders_[*lv].loader;
    return {{loader_hdl, loader.class_descriptor(method_hdl)},
            loader.unique_name(method_hdl)};
}

jvm_field_hdl virtual_machine::jvm_hdl(const dex_field_hdl& field_hdl) const
{
    const auto& loader_hdl = field_hdl.file_hdl.loader_hdl;

    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        throw std::runtime_error("invalid field handle");
    }

    const auto& loader = loaders_[*lv].loader;
    return {{loader_hdl, loader.class_descriptor(field_hdl)},
            loader.name(field_hdl)};
}
