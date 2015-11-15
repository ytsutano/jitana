#include <algorithm>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>

#include "jitana/vm_core/virtual_machine.hpp"
#include "jitana/vm_core/dex_file.hpp"
#include "jitana/vm_graph/edge_filtered_graph.hpp"

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
    struct class_finder : boost::default_dfs_visitor {
        virtual_machine& vm;
        const jvm_type_hdl& hdl;
        bool try_load;

        class_finder(virtual_machine& vm, const jvm_type_hdl& hdl,
                     bool try_load)
                : vm(vm), hdl(hdl), try_load(try_load)
        {
        }

        void discover_vertex(const loader_vertex_descriptor& v,
                             const loader_graph& g)
        {
            if (auto found_class = try_load
                        ? g[v].loader.load_class(vm, hdl.descriptor)
                        : g[v].loader.lookup_class(vm, hdl.descriptor)) {
                throw * found_class;
            }
        }
    } vis(*this, hdl, try_load);
    boost::vector_property_map<int> color_map(
            static_cast<unsigned>(num_vertices(loaders_)));
    try {
        boost::depth_first_visit(loaders_, *lv, vis, color_map);
    }
    catch (const class_vertex_descriptor& cv) {
        // Class found: remember the initiating handle, and return.
        classes_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = cv;
        return cv;
    }

    return boost::none;
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
    struct method_finder : boost::default_dfs_visitor {
        virtual_machine& vm;
        const jvm_method_hdl& hdl;

        method_finder(virtual_machine& vm, const jvm_method_hdl& hdl)
                : vm(vm), hdl(hdl)
        {
        }

        void discover_vertex(const loader_vertex_descriptor& v,
                             const loader_graph& g)
        {
            if (auto found_method = g[v].loader.lookup_method(
                        vm, hdl.type_hdl.descriptor, hdl.unique_name)) {
                throw * found_method;
            }
        }
    } vis(*this, hdl);
    boost::vector_property_map<int> color_map(
            static_cast<unsigned>(num_vertices(loaders_)));
    try {
        boost::depth_first_visit(loaders_, *lv, vis, color_map);
    }
    catch (const method_vertex_descriptor& mv) {
        // Method found: remember the initiating handle, and return.
        methods_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = mv;
        return mv;
    }

    return boost::none;
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
    struct field_finder : boost::default_dfs_visitor {
        virtual_machine& vm;
        const jvm_field_hdl& hdl;

        field_finder(virtual_machine& vm, const jvm_field_hdl& hdl)
                : vm(vm), hdl(hdl)
        {
        }

        void discover_vertex(const loader_vertex_descriptor& v,
                             const loader_graph& g)
        {
            if (auto found_field = g[v].loader.lookup_field(
                        vm, hdl.type_hdl.descriptor, hdl.unique_name)) {
                throw * found_field;
            }
        }
    } vis(*this, hdl);
    boost::vector_property_map<int> color_map(
            static_cast<unsigned>(num_vertices(loaders_)));
    try {
        boost::depth_first_visit(loaders_, *lv, vis, color_map);
    }
    catch (const field_vertex_descriptor& fv) {
        // Field found: remember the initiating handle, and return.
        fields_[boost::graph_bundle].jvm_hdl_to_vertex[hdl] = fv;
        return fv;
    }

    return boost::none;
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
        std::stringstream ss;
        ss << "invalid loader handle ";
        ss << loader_hdl;
        throw std::runtime_error(ss.str());
    }

    return loaders_[*lv].loader.load_all_classes(*this);
}

jvm_type_hdl virtual_machine::jvm_hdl(const dex_type_hdl& type_hdl) const
{
    const auto& loader_hdl = type_hdl.file_hdl.loader_hdl;

    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        std::stringstream ss;
        ss << "invalid type handle ";
        ss << type_hdl;
        throw std::runtime_error(ss.str());
    }

    return {loader_hdl, loaders_[*lv].loader.descriptor(type_hdl)};
}

jvm_method_hdl virtual_machine::jvm_hdl(const dex_method_hdl& method_hdl) const
{
    const auto& loader_hdl = method_hdl.file_hdl.loader_hdl;

    auto lv = find_loader_vertex(loader_hdl, loaders_);
    if (!lv) {
        std::stringstream ss;
        ss << "invalid method handle ";
        ss << method_hdl;
        throw std::runtime_error(ss.str());
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
        std::stringstream ss;
        ss << "invalid field handle ";
        ss << field_hdl;
        throw std::runtime_error(ss.str());
    }

    const auto& loader = loaders_[*lv].loader;
    return {{loader_hdl, loader.class_descriptor(field_hdl)},
            loader.name(field_hdl)};
}

namespace {
    bool
    load_recursive_impl(virtual_machine& vm, method_vertex_descriptor v,
                        std::unordered_set<method_vertex_descriptor>& visited);

    class recursive_loader : public boost::static_visitor<void> {
    public:
        recursive_loader(virtual_machine& vm,
                         std::unordered_set<method_vertex_descriptor>& visited)
                : vm_(vm), visited_(visited)
        {
        }

        void operator()(const insn_const_string& x)
        {
        }

        void operator()(const insn_const_class& x)
        {
        }

        void operator()(const insn_new_instance& x)
        {
            vm_.find_class(x.const_val, true);
        }

        void operator()(const insn_new_array& x)
        {
        }

        void operator()(const insn_filled_new_array& x)
        {
        }

        void operator()(const insn_sget& x)
        {
            auto fv = vm_.find_field(x.const_val, true);
            if (!fv) {
                std::stringstream ss;
                ss << "failed to find field " << vm_.jvm_hdl(x.const_val);
                throw std::runtime_error(ss.str());
            }

            // Try to load <clinit>.
            {
                const auto& fg = vm_.fields();
                auto clinit_mv = vm_.find_method(
                        jvm_method_hdl(fg[*fv].jvm_hdl.type_hdl, "<clinit>()V"),
                        true);
                if (clinit_mv) {
                    load_recursive_impl(vm_, *clinit_mv, visited_);
                }
            }
        }

        void operator()(const insn_sput& x)
        {
            auto fv = vm_.find_field(x.const_val, true);
            if (!fv) {
                std::stringstream ss;
                ss << "failed to find field " << vm_.jvm_hdl(x.const_val);
                throw std::runtime_error(ss.str());
            }

            // Try to load <clinit>.
            {
                const auto& fg = vm_.fields();
                auto clinit_mv = vm_.find_method(
                        jvm_method_hdl(fg[*fv].jvm_hdl.type_hdl, "<clinit>()V"),
                        true);
                if (clinit_mv) {
                    load_recursive_impl(vm_, *clinit_mv, visited_);
                }
            }
        }

        void operator()(const insn_invoke& x)
        {
            auto mv = vm_.find_method(x.const_val, true);
            if (!mv) {
                return;
            }

            const auto& mg = vm_.methods();
            switch (x.op) {
            case opcode::op_invoke_static:
            case opcode::op_invoke_static_range:
                // Try to load <clinit>.
                {
                    auto clinit_mv = vm_.find_method(
                            jvm_method_hdl(mg[*mv].jvm_hdl.type_hdl,
                                           "<clinit>()V"),
                            true);
                    if (clinit_mv) {
                        load_recursive_impl(vm_, *clinit_mv, visited_);
                    }
                }
                break;
            default:
                break;
            }

            const auto& mig
                    = make_edge_filtered_graph<method_super_edge_property>(mg);
            using method_inheritance_graph = decltype(mig);

            std::vector<method_vertex_descriptor> targets;
            struct target_finder : boost::default_dfs_visitor {
                std::vector<method_vertex_descriptor>& targets;

                target_finder(std::vector<method_vertex_descriptor>& targets)
                        : targets(targets)
                {
                }

                void discover_vertex(const method_vertex_descriptor& v,
                                     method_inheritance_graph& g)
                {
                    targets.push_back(v);
                }
            } vis(targets);
            boost::vector_property_map<int> color_map(
                    static_cast<unsigned>(num_vertices(mig)));
            boost::depth_first_visit(mig, *mv, vis, color_map);

            for (auto tv : targets) {
                load_recursive_impl(vm_, tv, visited_);
            }
        }

        void operator()(const insn_invoke_quick& x)
        {
        }

        template <typename T>
        void operator()(const T& x)
        {
        }

    private:
        virtual_machine& vm_;
        std::unordered_set<method_vertex_descriptor>& visited_;
    };

    bool
    load_recursive_impl(virtual_machine& vm, method_vertex_descriptor v,
                        std::unordered_set<method_vertex_descriptor>& visited)
    {
        if (visited.find(v) != end(visited)) {
            return true;
        }
        visited.insert(v);

        auto ig = vm.methods()[v].insns;

        if (num_vertices(ig) == 0) {
            return true;
        }

        recursive_loader rloader(vm, visited);
        for (auto iv : boost::make_iterator_range(vertices(ig))) {
            boost::apply_visitor(rloader, ig[iv].insn);
        }

        return true;
    }
}

std::unordered_set<method_vertex_descriptor>
virtual_machine::load_recursive(method_vertex_descriptor v)
{
    std::unordered_set<method_vertex_descriptor> visited;
    load_recursive_impl(*this, v, visited);
    return visited;
}
