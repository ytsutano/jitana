Jitana
======

<strong>This tool is still in the early stage of development. It's known to be
incomplet and incorrekt.</strong>

## 1 Overview

- A graph-based static-dynamic hybrid DEX code analysis tool.

### 1.1 Implementation

- Written in C++14.
- Uses the [Boost Graph Library (BGL)](http://www.boost.org/libs/graph/doc/).
    - Existing generic graph algorithms can be applied to Jitana data
      structures.
- Can be used as a library:
    - Run side-by-side with a virtual machine on a target device.
    - Run on a host machine talking to the target device via JDWP.
    - Used from visualization tool.
    - And more...

## 2 Coding Guidelines

- Use modern C++.
    - Prefer value semantics over pointer semantics.
        - Never use `new` or `delete`.
        - Never use pointers with implied ownership.
        - Don't abuse smart pointers.
        - Avoid Java-style intrusive inheritance which implies pointer
          semantics. Prefer generic algorithm for static polymorphism, and type
          erasure for dynamic polymorphism.
    - Write less.
        - Use the standard generic algorithms.
- Any C++ code *must* be formatted using `clang-format` with provided
  `.clang-format` file before committing to the repository.

## 3 Building

Jitana uses CMake which supports out-of-source build. In this document, the
following directory structure is assumed:

    .
    ├── jitana (source code downloaded)
    ├── dex    (DEX files)
    └── build  (build directory you make)

### 3.1 macOS

Install all the dependencies first. Then

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ../jitana
    make -j8

### 3.2 Ubuntu

Please refer to [Compiling Jitana on Linux](doc/jitana_on_ubuntu.md).

### 3.3 Windows

Please refer to [Compiling Jitana on Windows](doc/jitana_on_windows.md).

## 4 Design

Jitana is implemented using the [generic
programming](https://en.wikipedia.org/wiki/Generic_programming) techniques
rather than the traditional object-oriented techniques due to the following
ideas:

- Specialization of data structures to a specific system. Jitana defines data
  structures that closely map to (a) the data structures used in the actual
  system implementations including the virtual machine and the operating
  system, (b) the environment of execution such as network or file system
  information flow, or even (c) the things outside of computers such as social
  connections of users. This makes interleaving and extension, not just the
  combination as previously done, of static and dynamic analysis possible.
- Generalization of algorithms. By defining the data types such that they all
  model the appropriate concepts defined in the Boost Graph Library (BGL), we
  can still use the same implementation of algorithms on different data types
  using the generic programming techniques. A new algorithm can be implemented
  by composing existing algorithms such as the ones provided by the Standard
  Template Library (STL), the Adobe Source Library (ASL), and the Boost C++
  Libraries. It may also be reused in different contexts outside of Jitana.

### 4.1 Graph Data Structures

[The VM Design](dot_design.svg).

#### `jitana::virtual_machine`

- <strong>[Loader Graph]</strong> `jitana::loader_graph loaders()`
    - <em>[Graph Property]</em> `jitana::loader_graph_property`
    - <em>[Edge Property]</em> `jitana::loader_edge_property = jitana::any_edge_property`
    - <em>[Vertex Property]</em> `jitana::loader_vertex_property`
        - `jitana::class_loader loader`
            - `jitana::dex_file`
- <strong>[Class Graph]</strong> `jitana::class_graph classes()`
    - <em>[Graph Property]</em> `jitana::class_graph_property`
        - `std::unordered_map<jitana::jvm_type_hdl, jitana::class_vertex_descriptor> jvm_hdl_to_vertex`
        - `std::unordered_map<jitana::dex_type_hdl, jitana::class_vertex_descriptor> hdl_to_vertex`
    - <em>[Edge Property]</em> `jitana::class_edge_property = jitana::any_edge_property`
    - <em>[Vertex Property]</em> `jitana::class_vertex_property`
        - `jitana::dex_type_hdl hdl`
        - `jitana::jvm_type_hdl jvm_hdl`
        - `jitana::dex_access_flags access_flags`
        - `std::vector<jitana::dex_field_hdl> static_fields`
        - `std::vector<jitana::dex_field_hdl> instance_fields`
        - `std::vector<jitana::dex_method_hdl> dtable`
        - `std::vector<jitana::dex_method_hdl> vtable`
        - `uint16_t static_size`
        - `uint16_t instance_size`
- <strong>[Method Graph]</strong> `jitana::method_graph methods()`
    - <em>[Graph Property]</em> `jitana::method_graph_property`
        - `std::unordered_map<jitana::jvm_method_hdl, jitana::method_vertex_descriptor> jvm_hdl_to_vertex`
        - `std::unordered_map<jitana::dex_method_hdl, jitana::method_vertex_descriptor> hdl_to_vertex`
    - <em>[Edge Property]</em> `jitana::method_edge_property = jitana::any_edge_property`
    - <em>[Vertex Property]</em> `jitana::method_vertex_property`
        - `jitana::dex_method_hdl hdl`
        - `jitana::jvm_method_hdl jvm_hdl`
        - `jitana::dex_type_hdl class_hdl`
        - `jitana::dex_access_flags access_flags`
        - `std::vector<jitana::method_param> params`
        - <strong>[Instruction Graph]</strong> `jitana::insn_graph insns`
            - <em>[Graph Property]</em> `jitana::insn_graph_property`
                - `std::unordered_map<uint16_t, jitana::insn_vertex_descriptor> offset_to_vertex`
                - `jitana::dex_method_hdl hdl`
                - `jitana::jvm_method_hdl jvm_hdl`
                - `std::vector<jitana::try_catch_block> try_catches`
                - `size_t registers_size`
                - `size_t ins_size`
                - `size_t outs_size`
                - `uint32_t insns_off`
            - <em>[Edge Property]</em> `jitana::insn_edge_property = jitana::any_edge_property`
            - <em>[Vertex Property]</em> `jitana::insn_vertex_property`
                - `jitana::dex_insn_hdl hdl`
                - `jitana::insn insn`
                - `long long counter = 0`
                - `uint32_t off`
                - `int line_num = 0`

### 4.2 Handles

#### 4.2.1 DEX Handles vs. JVM Handles

A handle is used to identify a virtual machine object (class, method,
instruction, etc.) in Jitana. There are two types of handles:

- *DEX handles* are small (4 bytes or less) and closely related with the ID
  system used in the DEX file format. It is useful when communicating with the
  actual virtual machine (Dalvik or ART).
- *JVM handles* are based on string identifiers defined in the Java Virtual
  Machine Specification.

See include/jitana/hdl.hpp for implementation details.

~~~{.cpp}
           +--------------------------------+---------------------------------+
           | DEX Handle                     | JVM Handle                      |
           | (Android specific: int based)  | (General Java: string based)    |
+----------+--------------------------------+---------------------------------+
| Class    |                     struct class_loader_hdl {                    |
| Loader   |                       uint8_t idx;                               |
|          |                     }                                            |
+----------+--------------------------------+---------------------------------+
| DEX      | struct dex_file_hdl {          | N/A                             |
| File     |   class_loader_hdl loader_hdl; | (No concept of DEX file in JVM) |
|          |   uint8_t idx;                 |                                 |
|          | };                             |                                 |
+----------+--------------------------------+---------------------------------+
| Type     | struct dex_type_hdl {          | struct jvm_type_hdl {           |
|          |   dex_file_hdl file_hdl;       |   class_loader_hdl loader_hdl;  |
|          |   uint16_t idx;                |   std::string descriptor;       |
|          | };                             | };                              |
+----------+--------------------------------+---------------------------------+
| Method   | struct dex_method_hdl {        | struct jvm_method_hdl {         |
|          |   dex_file_hdl file_hdl;       |   jvm_type_hdl type_hdl;        |
|          |   uint16_t idx;                |   std::string unique_name;      |
|          | };                             | };                              |
+----------+--------------------------------+---------------------------------+
| Field    | struct dex_field_hdl {         | struct jvm_field_hdl {          |
|          |   dex_file_hdl file;           |   jvm_type_hdl type_hdl;        |
|          |   uint16_t idx;                |   std::string unique_name;      |
|          | };                             | };                              |
+----------+--------------------------------+---------------------------------+
| Instruc- | struct dex_insn_hdl {          | N/A                             |
| tion     |   dex_method_hdl method_hdl;   |                                 |
|          |   uint16_t idx;                |                                 |
|          | };                             |                                 |
+----------+--------------------------------+---------------------------------+
| Register | struct dex_reg_hdl {           | N/A                             |
|          |   dex_insn_hdl insn_hdl;       |                                 |
|          |   uint16_t idx;                |                                 |
|          | };                             |                                 |
+----------+--------------------------------+---------------------------------+
~~~

#### 4.2.2 Handles for Class Loading Initiation vs. Handles for Definition

We have one or more *initiating handles* and a unique *defining handle* for
each VM object:

- Initiating handles are used to find a VM object by calling
  `virtual_machine::find_*()`.
- A defining handle is one of the initiating handles of a VM object that
  defines the identity. Thus a graph node representing a VM object holds its
  defining handle.

These concepts mirror the *defining loaders* and the *initiating loaders*
described in the [JVM
Specification](http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-5.html).

## 5 Usage

### 5.1 Simple Example

For convenience, you should create your own tool under `tools/` so that the
build system can read your `CMakeLists.txt` automatically. You may use
`tools/jitana-graph/` as an example.

~~~~{.cpp}
#include <jitana/jitana.hpp>

int main()
{
    // 1.  Create a virtual machine.
    jitana::virtual_machine vm;

    // 2a.  Create and add a system class loader.
    {
        const auto& filenames = {"dex/system/framework/core.dex",
                                 "dex/system/framework/framework.dex",
                                 "dex/system/framework/framework2.dex",
                                 "dex/system/framework/ext.dex",
                                 "dex/system/framework/conscrypt.dex",
                                 "dex/system/framework/okhttp.dex"};
        jitana::class_loader loader(11, "SystemLoader", begin(filenames),
                                    end(filenames));
        vm.add_loader(loader);
    }

    // 2b. Create and add an application class loader.
    {
        const auto& filenames = {"dex/app/instagram_classes.dex"};
        jitana::loader loader(22, "Instagram", begin(filenames),
                              end(filenames));
        vm.add_loader(loader, 11);
    }

    // 3a. Load a specific class.
    //     You need to specify fully qualified Java binary name.
    vm.find_class({22, "Ljava/lang/BootClassLoader;"}, true);

    // 3b. Or, load everything from a class loader.
    vm.load_all_classes(22);
}
~~~~

## 6 Developer

- Yutaka Tsutano at University of Nebraska-Lincoln.

## 7 License

- See [LICENSE.md](LICENSE.md) for license rights and limitations (ISC).

## 8 External Links

- [Dalvik Bytecode](http://source.android.com/devices/tech/dalvik/dalvik-bytecode.html)
- [Dalvik Executable Format](http://source.android.com/devices/tech/dalvik/dex-format.html)
- [Dalvik Executable Instruction Formats](http://source.android.com/devices/tech/dalvik/instruction-formats.html)
- [JVM Specification Chapter 5](http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-5.html)
