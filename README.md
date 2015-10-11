Jitana
======

<strong>This tool is still in the early stage of development. It's known to be incomplet and incorrekt.</strong>

## Overview

* A graph-based static-dynamic hybrid DEX code analysis tool.

### Implementation

* Written in C++14.
* Uses the [Boost Graph Library (BGL)](http://www.boost.org/libs/graph/doc/).
    * Supports generic programming.
* Can be used as a library:
    * Run side-by-side with a virtual machine on a target device.
    * Run on a host machine talking to the target device via JDWP.
    * Used from visualization tool.
    * And more...

## Coding Guidelines

* Use modern C++.
    * Prefer value semantics over pointer semantics.
        * Never use `new` or `delete`.
        * Never use pointers with implied ownership.
        * Don't abuse smart pointers.
        * Avoid Java-style intrusive inheritance which implies pointer semantics. Prefer generic algorithm for static polymorphism, and  type erasure for dynamic polymorphism.
    * Use the standard generic algorithms.
* Any C++ code *must* be formatted using `clang-format` with provided `.clang-format` file before committing to the repository.

## Building

Jitana uses CMake which supports out-of-source build. In this document, the following directory structure is assumed:

    .
    ├── jitana (source code downloaded)
    ├── dex    (DEX files)
    └── build  (build directory you make)

### OS X

Install all the dependencies first. Then

    mkdir build
    cd build
    cmake ../jitana
    make -j8

### Ubuntu

Jitana needs GCC 4.9 On Ubuntu 14.04, only GCC 4.8 is provided. In addition to all the dependencies needed, you might need to install g++-4.9 or clang.

    sudo apt-get install g++-4.9

Then

    mkdir build
    cd build
    CXX=usr/bin/g++-4.9 cmake ../jitana
    make -j8

## Design

### VM Structure

[The VM Design](dot_design.svg).

* `jitana::virtual_machine`
    * <strong>[Graph]</strong> `jitana::loader_graph loaders_`
        * <em>[Graph Property]</em> `jitana::loader_graph_property`
        * <em>[Vertex Property]</em> `jitana::loader_vertex_property`
            * `jitana::class_loader loader`
                * `jitana::dex_file`
        * <em>[Edge Property]</em> `jitana::loader_edge_property = jitana::any_edge_property`
    * <strong>[Graph]</strong> `jitana::class_graph classes_`
        * <em>[Graph Property]</em> `jitana::class_graph_property`
            * `std::unordered_map<jitana::jvm_type_hdl, jitana::class_vertex_descriptor> jvm_hdl_to_vertex`
            * `std::unordered_map<jitana::dex_type_hdl, jitana::class_vertex_descriptor> hdl_to_vertex`
        * <em>[Vertex Property]</em> `jitana::class_vertex_property`
            * `jitana::dex_type_hdl hdl`
            * `std::string descriptor`
            * `jitana::dex_access_flags access_flags`
            * `std::vector<jitana::dex_field_hdl> static_fields`
            * `std::vector<jitana::dex_field_hdl> instance_fields`
            * `std::vector<jitana::dex_method_hdl> dtable`
            * `std::vector<jitana::dex_method_hdl> vtable`
            * `uint16_t static_size`
            * `uint16_t instance_size`
        * <em>[Edge Property]</em> `jitana::class_edge_property = jitana::any_edge_property`
    * <strong>[Graph]</strong> `jitana::method_graph methods_`
        * <em>[Graph Property]</em> `jitana::method_graph_property`
            * `std::unordered_map<jitana::jvm_method_hdl, jitana::method_vertex_descriptor> jvm_hdl_to_vertex`
            * `std::unordered_map<jitana::dex_method_hdl, jitana::method_vertex_descriptor> hdl_to_vertex`
        * <em>[Vertex Property]</em> `jitana::method_vertex_property`
            * `jitana::dex_method_hdl hdl`
            * `jitana::dex_type_hdl class_hdl`
            * `std::string unique_name`
            * `jitana::dex_access_flags access_flags`
            * `std::vector<jitana::method_param> params`
            * <strong>[Graph]</strong> `jitana::insn_graph insns`
                * <em>[Graph Property]</em> `jitana::insn_graph_property`
                    * `std::unordered_map<uint16_t, jitana::insn_vertex_descriptor> offset_to_vertex`
                    * `jitana::dex_method_hdl hdl`
                    * `jitana::jvm_method_hdl jvm_hdl`
                    * `std::vector<jitana::try_catch_block> try_catches`
                    * `size_t registers_size`
                    * `size_t ins_size`
                    * `size_t outs_size`
                    * `uint32_t insns_off`
                * <em>[Vertex Property]</em> `jitana::insn_vertex_property`
                    * `jitana::dex_insn_hdl hdl`
                    * `jitana::insn insn`
                    * `long long counter = 0`
                    * `uint32_t off`
                    * `int line_num = 0`
                * <em>[Edge Property]</em> `jitana::insn_edge_property = jitana::any_edge_property`
        * <em>[Edge Property]</em> `jitana::method_edge_property = jitana::any_edge_property`

### Handles

A handle is used to identify a virtual machine object. There are two types of handles: DEX Handle and JVM Handle.

* DEX handles are small (4 bytes or less) and closely related with the ID system used in the DEX file format. It is useful when communicating with the actual virtual machine (Dalvik or ART).
* JVM handles are based on string identifiers defined in the Java Virtual Machine Specification.
* We have one or more *initiating handles* and a unique *defining handle* for each object. Initiating handles are used to find a VM object by calling `virtual_machine::find_*()`. A defining handle is one of the initiating handles which defines the identity of the VM object. Thus a graph node representing a VM object holds its defining handle. These concepts are very similar to the *defining loaders* and the *initiating loaders* described in the [JVM Specification](http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-5.html).

See include/jitana/hdl.hpp for implementation details.

<table>
    <tr>
        <th></th>
        <th>DEX Handle</th>
        <th>JVM Handle</th>
    </tr>
    <tr>
        <th>Class Loader</th>
        <td colspan="2"><code>
            using class_loader_hdl = uint8_t;
        </code></td>
    </tr>
    <tr>
        <th>DEX File</th>
        <td>
<pre>
struct dex_file_hdl {
    class_loader_hdl loader_hdl;
    uint8_t idx;
};
</pre>
        </td>
        <td>N/A<br />(No concept of a DEX file in JVM)</td>
    </tr>
    <tr>
        <th>Type</th>
        <td>
<pre>
struct dex_type_hdl {
    dex_file_hdl file_hdl;
    uint16_t idx;
};
</pre>
        </td>
        <td>
<pre>
struct jvm_type_hdl {
    class_loader_hdl loader_hdl;
    std::string descriptor;
};
</pre>
        </td>
    </tr>
    <tr>
        <th>Method</th>
        <td>
<pre>
struct dex_method_hdl {
    dex_file_hdl file_hdl;
    uint16_t idx;
};
</pre>
        </td>
        <td>
<pre>
struct jvm_method_hdl {
    jvm_type_hdl type_hdl;
    std::string unique_name;
};
</pre>
        </td>
    </tr>
    <tr>
        <th>Field</th>
        <td>
<pre>
struct dex_field_hdl {
    dex_file_hdl file;
    uint16_t idx;
};
</pre>
        </td>
        <td>
<pre>
struct jvm_field_hdl {
    jvm_type_hdl type_hdl;
    std::string unique_name;
};
</pre>
        </td>
    </tr>
    <tr>
        <th>Instruction</th>
        <td>
<pre>
struct dex_insn_hdl {
    dex_method_hdl method_hdl;
    uint16_t idx;
};
</pre>
        </td>
        <td>N/A</td>
    </tr>
    <tr>
        <th>Register</th>
        <td>
<pre>
struct dex_reg_hdl {
    dex_method_hdl method_hdl;
    uint16_t idx;
};
</pre>
        </td>
        <td>N/A</td>
    </tr>
</table>

## Usage

### Simple Example

~~~~{.cpp}
#include <jitana/jitana.hpp>

int main()
{
    // 1.  Create a virtual machine.
    jitana::virtual_machine vm;

    // 2a.  Create and add a system class loader.
    {
        const auto& filenames = {
            "dex/system/framework/core.dex",
            "dex/system/framework/framework.dex",
            "dex/system/framework/framework2.dex",
            "dex/system/framework/ext.dex",
            "dex/system/framework/conscrypt.dex",
            "dex/system/framework/okhttp.dex",
        };
        jitana::class_loader loader(11, begin(filenames), end(filenames));
        vm.add_loader(loader);
    }

    // 2b. Create and add an application class loader.
    {
        const auto& filenames = {
            "dex/app/instagram_classes.dex",
        };
        jitana::loader loader(22, begin(filenames), end(filenames));
        vm.add_loader(loader, 11);
    }

    // 3a. Load a specific class.
    //     You need to specify fully qualified Java binary name.
    vm.find_class({22, "Ljava/lang/BootClassLoader;"}, true);

    // 3b. Or, load everything from a class loader.
    vm.load_all_classes(22);
}
~~~~

## Developer

* Yutaka Tsutano at University of Nebraska-Lincoln.

## License

* TBD.

## External Links

* [Bytecode for the Dalvik VM](http://source.android.com/devices/tech/dalvik/dalvik-bytecode.html)
* [.dex &mdash; Dalvik Executable Format](http://source.android.com/devices/tech/dalvik/dex-format.html)
* [Dalvik VM Instruction Formats](http://source.android.com/devices/tech/dalvik/instruction-formats.html)
* [JVM Specification Chapter 5](http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-5.html)
