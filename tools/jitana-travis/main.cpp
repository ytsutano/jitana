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

#include <iostream>
#include <unordered_map>
#include <thread>
#include <fstream>

#include <boost/asio.hpp>

#include <jitana/jitana.hpp>
#include <jitana/util/jdwp.hpp>
#include <jitana/analysis/call_graph.hpp>
#include <jitana/analysis/data_flow.hpp>

#include <unistd.h>
#include <sys/wait.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

struct insn_counter {
    long long counter = 0;
    long long delta = 0;
    long long last_accessed = 0;
    boost::optional<std::pair<jitana::method_vertex_descriptor,
                              jitana::insn_vertex_descriptor>> vertices;
};

struct dex_file {
    std::unordered_map<uint32_t, insn_counter> counters;
    jitana::dex_file_hdl hdl;
    bool valid;
};

static bool periodic_output = false;

static jitana::virtual_machine vm;
static jitana::class_loader_hdl system_loader_hdl = 0;
static jitana::class_loader_hdl app_loader_hdl = 1;

/// Instruction stats. Can be passed as a visitor to
/// jitana::jdwp_connection::receive_insn_counters.
struct insn_stats {
public:
    void enter_dex_file(const std::string& source_filename,
                        const std::string& filename)
    {
        auto it = dex_files_.find(filename);
        if (it != end(dex_files_)) {
            current_dex_file_ = &it->second;
        }
        else {
            // Get the basename of the ODEX file.
            std::string basename
                    = {std::find_if(filename.rbegin(), filename.rend(),
                                    [](char c) { return c == '/'; })
                               .base(),
                       end(filename)};
            std::cout << basename << "\n";

            auto lv = find_loader_vertex(app_loader_hdl, vm.loaders());
            if (!lv) {
                throw std::runtime_error("application loader not found");
            }
            auto& loader = vm.loaders()[*lv].loader;

            // Pull the ODEX file from the device.
            const std::string local_filename = "odex/" + basename;
            pid_t pid = ::fork();
            if (pid == 0) {
                // Run the shell script.
                std::cout << "Pulling " << filename << "!!!!!!!!!!!!!!\n";
                ::execl("pull-odex", "pull-odex", filename.c_str(),
                        local_filename.c_str(), nullptr);
            }
            else if (pid > 0) {
                std::cout << "Executing: pid = " << pid << "\n";
                int status = 0;
                ::waitpid(pid, &status, 0);
                std::cout << "Done: status = " << status << "\n";
            }
            else {
                throw std::runtime_error("failed to pull ODEX");
            }

            std::cout << local_filename << "\n";
            std::cout << filename << "\n";

            current_dex_file_ = &dex_files_[filename];
            try {
                loader.add_file(local_filename);
                current_dex_file_->valid = true;
                current_dex_file_->hdl.loader_hdl = app_loader_hdl;
                current_dex_file_->hdl.idx = loader.dex_files().size() - 1;
            }
            catch (...) {
                current_dex_file_->valid = false;
            }
        }

        std::cout << "source_filename = " << source_filename << "\n";
        std::cout << "filename = " << filename << "\n";
        for (auto& c : current_dex_file_->counters) {
            c.second.delta = 0;
            ++c.second.last_accessed;
        }
    }

    void insn(uint32_t offset, uint16_t counter)
    {
        if (current_dex_file_->valid) {
            auto& c = current_dex_file_->counters[offset];
            c.counter += counter;
            c.delta = counter;
            c.last_accessed = 0;
        }
    }

    void exit_dex_file()
    {
    }

    const std::unordered_map<std::string, dex_file>& dex_files() const
    {
        return dex_files_;
    }

    std::unordered_map<std::string, dex_file>& dex_files()
    {
        return dex_files_;
    }

private:
    std::unordered_map<std::string, dex_file> dex_files_;
    dex_file* current_dex_file_;
};

static insn_stats stats;

// Lighting parameters.
static constexpr GLfloat light0_position[] = {500.0f, 200.0f, 500.0f, 1.0f};
static constexpr GLfloat light0_ambient_color[] = {0.5f, 0.5f, 0.5f, 1.0f};
static constexpr GLfloat light0_diffuse_color[] = {0.8f, 0.8f, 0.8f, 1.0f};

static int width = 0.0;
static int height = 0.0;
static int window_x = 100;
static int window_y = 100;
static int window_width = 1024;
static int window_height = 700;
static std::string title = "Jitana-TraVis";
static int window;

static double view_angle = -M_PI / 3.0 + M_PI / 2.0;
static double view_altitude = 0.5;
static double view_angle_offset = 0.0;
static double view_altitude_offset = 0.0;
static double view_center_x = 128.0;
static double view_center_x_offset = 0.0;
static double view_center_y = -256.0;
static double view_center_y_offset = 0.0;
static double view_zoom = 0.17;
static double view_zoom_offset = 0.0;

static int drag_start_x = 0;
static int drag_start_y = 0;
static bool dragging = false;
static bool zooming = false;
static bool shifting = false;
static bool full_screen = false;

static void init_opengl();
static void reshape(int width, int height);
static void display();
static void handle_mouse_event(int button, int state, int x, int y);
static void handle_motion_event(int x, int y);
static void handle_keyboard_event(unsigned char c, int x, int y);
static void update_graphs();

void draw_instruction(int index, uint32_t /*address*/,
                      const insn_counter& counter)
{
    constexpr int l = 128;
    float x = 14.0f * (index / l);
    float y = 4.0f * (index % l);
    float z = 0.2;

    // Draw a Red 1x1 Square centered at origin
    glBegin(GL_QUADS);
    {
        if (counter.counter > 0) {
            auto r = 0.8f * std::min(counter.counter + 1, 1000ll) / 1000.0f
                    + 0.2f;
            auto d = 0.8f * std::min(counter.delta, 5ll) / 5.0f + 0.2f;
            // glColor3f(d, 0.2f, r);
            z += std::min(counter.counter + 1, 1000ll) / 400.0f + 1.0f;

            GLfloat color[] = {d, 0.2f, r, 1.0f};
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
        }
        else {
            // glColor3f(0.2f, 0.2f, 0.2f);
            // glColor3f(1.0f, 1.0f, 0.2f);

            GLfloat color[] = {1.0f, 1.0f, 0.2f, 1.0f};
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
        }
        constexpr auto w = 10.0f;
        constexpr auto h = 2.0f;

        glNormal3f(0, 1, 0);
        glVertex3f(x + w / 2, z, y + h / 2);
        glVertex3f(x - w / 2, z, y + h / 2);
        glVertex3f(x - w / 2, z, y - h / 2);
        glVertex3f(x + w / 2, z, y - h / 2);

        glNormal3f(0, 0, 1);
        glVertex3f(x + w / 2, z, y + h / 2);
        glVertex3f(x - w / 2, z, y + h / 2);
        glVertex3f(x - w / 2, 0, y + h / 2);
        glVertex3f(x + w / 2, 0, y + h / 2);

        glNormal3f(0, 0, -1);
        glVertex3f(x + w / 2, z, y - h / 2);
        glVertex3f(x - w / 2, z, y - h / 2);
        glVertex3f(x - w / 2, 0, y - h / 2);
        glVertex3f(x + w / 2, 0, y - h / 2);

        glNormal3f(1, 0, 0);
        glVertex3f(x + w / 2, z, y + h / 2);
        glVertex3f(x + w / 2, z, y - h / 2);
        glVertex3f(x + w / 2, 0, y - h / 2);
        glVertex3f(x + w / 2, 0, y + h / 2);

        glNormal3f(-1, 0, 0);
        glVertex3f(x - w / 2, z, y + h / 2);
        glVertex3f(x - w / 2, z, y - h / 2);
        glVertex3f(x - w / 2, 0, y - h / 2);
        glVertex3f(x - w / 2, 0, y + h / 2);
    }
    glEnd();
}

void init_opengl()
{
    // Set the clear color.
    constexpr GLfloat clearColor[] = {0.0, 0.0, 0.0, 1.0};
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

    // Set the lights.
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient_color);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse_color);

    // Set the shade model.
    glShadeModel(GL_SMOOTH);

    // Enable lighting.
    glEnable(GL_LIGHTING);

    // Enable automatic normalization after transformations.
    glEnable(GL_NORMALIZE);

    // Enable antialiasing.
    glEnable(GL_POLYGON_SMOOTH);
}

void display()
{
    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable the depth buffer.
    glEnable(GL_DEPTH_TEST);

    // Compute the view matrix.
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        auto angle = view_angle + view_angle_offset;
        auto altitude = std::max(view_altitude + view_altitude_offset, 0.0);
        gluLookAt(1000.0 * cos(angle) + view_center_x + view_center_x_offset,
                  1000.0 * altitude,
                  1000.0 * sin(angle) - (view_center_y + view_center_y_offset),
                  view_center_x + view_center_x_offset, 0,
                  -(view_center_y + view_center_y_offset), 0.0, 1.0, 0.0);
    }

#if 0
    int i = 0;
    for (const auto& dex : stats.dex_files()) {
        glPushMatrix();
        {
            int l = 256;
            float x = 14.0f * (i / l);
            float y = 4.0f * (i % l);
            glColor3f(1.0f, 1.0f, 1.0f);
            glRasterPos3d(x, 3.0, y - 4.0);
            for (const auto& c : dex.first) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }
        }
        glPopMatrix();

        // Draw the instructions.
        for (const auto& inst : dex.second.counters) {
            draw_instruction(i++, inst.first, inst.second);
        }

        i += 256 * 4;
        i -= (i % 256);
    }
#else
    int i = 0;
    for (const auto& dex : stats.dex_files()) {
        glPushMatrix();
        {
            // Disable lighting.
            glDisable(GL_LIGHTING);

            int l = 256;
            float x = 14.0f * (i / l);
            float y = 4.0f * (i % l);
            glColor3f(1.0f, 1.0f, 1.0f);
            glRasterPos3d(x, 3.0, y - 4.0);
            for (const auto& c : dex.first) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }

            // Enable lighting.
            glEnable(GL_LIGHTING);
        }
        glPopMatrix();

        auto it = stats.dex_files().find(dex.first);
        if (it == end(stats.dex_files())) {
            std::cerr << "No DEX file named " << dex.first << " is found\n";
        }
        auto& counters = it->second.counters;

        // Draw the instructions.
        const auto& cg = vm.classes();
        const auto& mg = vm.methods();
        for (const auto& cv : boost::make_iterator_range(vertices(cg))) {
            // Ignore a class implemented in a different DEX file for now.
            if (cg[cv].hdl.file_hdl != dex.second.hdl) {
                continue;
            }

            auto draw_method = [&](jitana::method_vertex_descriptor mv) {
                const auto& ig = mg[mv].insns;
                auto insns_off = ig[boost::graph_bundle].insns_off;
                if (insns_off == 0) {
                    return;
                }

                // A method should be defined in the same class.
                if (mg[mv].class_hdl != cg[cv].hdl) {
                    return;
                }

                for (const auto& iv :
                     boost::make_iterator_range(vertices(ig))) {
                    if (!is_basic_block_head(iv, ig)) {
                        continue;
                    }

                    // std::cerr << "insns_off = " << insns_off << ", ";
                    // std::cerr << "ig[iv].off = " << ig[iv].off << "\n";

                    auto addr = insns_off + ig[iv].off * 2;
#if 0
                    const auto it = counters.find(addr);
                    if (it != counters.end()) {
                        draw_instruction(i++, addr, it->second);
                    }
#else
                    draw_instruction(i++, addr, counters[addr]);

                    // Checking
                    auto p = vm.find_insn(dex.second.hdl, addr, false);
                    if (!p) {
                        std::cerr << "method=" << mg[mv].hdl << " ";
                        std::cerr << "insns_off=" << insns_off << " ";
                        std::cerr << "ig[iv].off=" << ig[iv].off << " ";
                        std::cerr << ig[iv].insn << " " << addr << "\n";
                    }
                    else {
                        if (mg[mv].hdl != mg[p->first].hdl) {
                            std::cerr << "************* Bad! ";
                            std::cerr << "method=" << mg[mv].hdl << " ";
                            std::cerr << "ig[iv].off=" << ig[iv].off << " ";
                            std::cerr << "found_method=" << p->first << " ";
                            std::cerr << "found_insn=" << p->second << "\n";
                        }
                    }
#endif
                }

                // ++i;
            };

            for (const auto& mh : cg[cv].vtable) {
                auto mv = vm.find_method(mh, false);
                if (mv) {
                    draw_method(*mv);
                }
            }

            for (const auto& mh : cg[cv].dtable) {
                auto mv = vm.find_method(mh, false);
                if (mv) {
                    draw_method(*mv);
                }
            }
        }

        i += 256 * 4;
        i -= (i % 256);
    }
#endif
    // Update the screen by swapping the buffers.
    glutSwapBuffers();
}

static void handle_mouse_event(int button, int state, int x, int y)
{
    switch (state) {
    case GLUT_DOWN:
        drag_start_x = x;
        drag_start_y = y;
        if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
            shifting = true;
        }
        else {
            if (button == GLUT_RIGHT_BUTTON) {
                zooming = true;
            }
            else {
                dragging = true;
            }
        }
        break;
    case GLUT_UP:
        if (dragging) {
            view_angle += view_angle_offset;
            view_altitude += view_altitude_offset;
            view_altitude = std::max(view_altitude, 0.0);
            dragging = false;
        }
        else if (shifting) {
            view_center_x += view_center_x_offset;
            view_center_y += view_center_y_offset;
            view_center_x_offset = 0.0;
            view_center_y_offset = 0.0;
            shifting = false;
            reshape(width, height);
        }
        else if (zooming) {
            view_zoom += view_zoom_offset;
            view_zoom = std::min(std::max(view_zoom, 0.01), 10000.0);
            zooming = false;
        }
        break;
    }
    view_angle_offset = 0.0;
    view_altitude_offset = 0.0;
    view_zoom_offset = 0.0;

    glutPostRedisplay();
}

static void handle_motion_event(int x, int y)
{
    if (dragging) {
        view_angle_offset = 4.0 * (x - drag_start_x) / width;
        view_altitude_offset = 4.0 * (y - drag_start_y) / height;
        glutPostRedisplay();
    }

    if (zooming) {
        view_zoom_offset = static_cast<double>(y - drag_start_y) / height;
        reshape(width, height);
        glutPostRedisplay();
    }

    if (shifting) {
        const double a = -2000 * view_zoom * (x - drag_start_x) / width;
        const double b = -2000 * view_zoom * (y - drag_start_y) / height;
        view_center_x_offset
                = a * std::sin(view_angle) + b * std::cos(view_angle);
        view_center_y_offset
                = a * std::cos(view_angle) - b * std::sin(view_angle);
        reshape(width, height);
        glutPostRedisplay();
    }
}

static void update_graphs()
{
    jitana::add_call_graph_edges(vm);

    for (auto& p : stats.dex_files()) {
        auto& dex = p.second;
        if (!dex.valid) {
            continue;
        }

        for (auto& c : dex.counters) {
            auto& offset = c.first;
            auto& ictr = c.second;
            auto& vertices = ictr.vertices;
            if (!vertices) {
                vertices = vm.find_insn(dex.hdl, offset, true);
            }
            if (vertices) {
                vm.methods()[vertices->first].insns[vertices->second].counter
                        = ictr.counter;
            }
            else {
                std::cerr << "failed to find the vertex: ";
                std::cerr << dex.hdl << " " << offset << "\n";
            }
        }
    }
}

static void write_graphviz()
{
    std::cout << "writing the graphs... " << std::flush;

    {
        std::ofstream ofs("output/loader_graph.dot");
        write_graphviz_loader_graph(ofs, vm.loaders());
    }

    {
        std::ofstream ofs("output/class_graph.dot");
        write_graphviz_class_graph(ofs, vm.classes());
    }

    {
        std::ofstream ofs("output/method_graph.dot");
        write_graphviz_method_graph(ofs, vm.methods());
    }

    auto mg = vm.methods();
    for (const auto& v : boost::make_iterator_range(vertices(mg))) {
        const auto& mprop = mg[v];
        const auto& ig = mprop.insns;

        if (mprop.class_hdl.file_hdl.loader_hdl == 0) {
            continue;
        }

        if (num_vertices(ig) == 0) {
            continue;
        }

        std::stringstream ss;
        ss << "output/insn/" << mprop.hdl << ".dot";
        std::ofstream ofs(ss.str());
        write_graphviz_insn_graph(ofs, ig);
    }

    std::cout << "done." << std::endl;
}

static void write_traces()
{
    std::ofstream ofs("output/traces.csv");

    ofs << "loader,dex,class_idx,method_idx,";
    ofs << "offset,counter,line_num,";
    ofs << "class descriptor,method unique name\n";

    const auto& cg = vm.classes();
    const auto& mg = vm.methods();
    for (const auto& mv : boost::make_iterator_range(vertices(mg))) {
        const auto& ig = mg[mv].insns;
        for (const auto& iv : boost::make_iterator_range(vertices(ig))) {
            if (!is_basic_block_head(iv, ig)) {
                continue;
            }

            const auto& cv = vm.find_class(mg[mv].class_hdl, false);
            if (!cv) {
                continue;
            }

            const auto& mprop = mg[mv];
            const auto& cprop = cg[*cv];
            const auto& iprop = ig[iv];

            if (mprop.class_hdl.file_hdl.loader_hdl == 0) {
                continue;
            }

            ofs << unsigned(mprop.class_hdl.file_hdl.loader_hdl) << ",";
            ofs << unsigned(mprop.class_hdl.file_hdl.idx) << ",";
            ofs << unsigned(mprop.class_hdl.idx) << ",";
            ofs << unsigned(mprop.hdl.idx) << ",";
            ofs << iprop.off << ",";
            ofs << iprop.counter << ",";
            ofs << iprop.line_num << ",";
            ofs << cprop.jvm_hdl.descriptor << ",";
            ofs << mprop.jvm_hdl.unique_name << "\n";
        }
    }
}

static void write_vtables()
{
    std::ofstream ofs("output/vtables.csv");

    ofs << "class handle, class descriptor, vtable index, ";
    ofs << "super class handle, super class descriptor, ";
    ofs << "method handle, method unique name\n";

    const auto& cg = vm.classes();
    const auto& mg = vm.methods();
    for (const auto& cv : boost::make_iterator_range(vertices(cg))) {
        const auto& vtable = cg[cv].vtable;
        for (unsigned i = 0; i < vtable.size(); ++i) {
            const auto& mh = vtable[i];
            auto mv = vm.find_method(mh, true);
            if (!mv) {
                throw std::runtime_error("invalid vtable entry");
            }

            auto super_cv = vm.find_class(mg[*mv].class_hdl, true);
            if (!super_cv) {
                throw std::runtime_error("invalid vtable entry");
            }

            ofs << cg[cv].hdl << ",";
            ofs << cg[cv].jvm_hdl.descriptor << ",";
            ofs << i << ",";
            ofs << cg[*super_cv].hdl << ",";
            ofs << cg[*super_cv].jvm_hdl.descriptor << ",";
            ofs << mg[*mv].hdl << ",";
            ofs << mg[*mv].jvm_hdl.unique_name << ",";
            ofs << "\n";
        }
    }
}

static void write_dtables()
{
    std::ofstream ofs("output/dtables.csv");

    ofs << "class handle, class descriptor, dtable index, ";
    ofs << "super class handle, super class descriptor, ";
    ofs << "method handle, method unique name\n";

    const auto& cg = vm.classes();
    const auto& mg = vm.methods();
    for (const auto& cv : boost::make_iterator_range(vertices(cg))) {
        const auto& dtable = cg[cv].dtable;
        for (unsigned i = 0; i < dtable.size(); ++i) {
            const auto& mh = dtable[i];
            auto mv = vm.find_method(mh, true);
            if (!mv) {
                throw std::runtime_error("invalid dtable entry");
            }

            auto super_cv = vm.find_class(mg[*mv].class_hdl, true);
            if (!super_cv) {
                throw std::runtime_error("invalid dtable entry");
            }

            ofs << cg[cv].hdl << ",";
            ofs << cg[cv].jvm_hdl.descriptor << ",";
            ofs << i << ",";
            ofs << cg[*super_cv].hdl << ",";
            ofs << cg[*super_cv].jvm_hdl.descriptor << ",";
            ofs << mg[*mv].hdl << ",";
            ofs << mg[*mv].jvm_hdl.unique_name << ",";
            ofs << "\n";
        }
    }
}

static void handle_keyboard_event(unsigned char c, int /*x*/, int /*y*/)
{
    switch (c) {
    case 'a':
    case 'A':
        std::cout << "loading all classes... " << std::flush;
        vm.load_all_classes(app_loader_hdl);
        std::cout << " done." << std::endl;
        break;
    case 'f':
    case 'F':
        if (full_screen) {
            glutPositionWindow(window_x, window_y);
            glutReshapeWindow(window_width, window_height);
            full_screen = false;
        }
        else {
            window_x = glutGet(GLUT_WINDOW_X);
            window_y = glutGet(GLUT_WINDOW_Y);
            window_width = glutGet(GLUT_WINDOW_WIDTH);
            window_height = glutGet(GLUT_WINDOW_HEIGHT);
            glutFullScreen();
            full_screen = true;
        }
        break;
    case 'g':
    case 'G':
        update_graphs();
        write_graphviz();
        write_traces();
        write_vtables();
        write_dtables();
        break;
    case 'p':
    case 'P':
        periodic_output = !periodic_output;
        std::cout << "periodic_output = " << periodic_output << "\n";
        break;
    case 'd':
    case 'D':
        // Compute the data-flow.
        std::for_each(vertices(vm.methods()).first,
                      vertices(vm.methods()).second,
                      [&](const jitana::method_vertex_descriptor& v) {
                          add_data_flow_edges(vm.methods()[v].insns);
                      });
        break;
    }
    glutPostRedisplay();
}

static void reshape(int width, int height)
{
    ::width = width;
    ::height = height;

    // Compute the aspect ratio.
    const double aspectRatio = static_cast<double>(width) / height;

    // Compute the projection matrix.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double z = std::min(std::max(view_zoom + view_zoom_offset, 0.01), 10000.0);
    glOrtho(-1000.0 * aspectRatio * z, 1000.0 * aspectRatio * z, -1000.0 * z,
            1000.0 * z, -1400.0, 10000.0);

    // Disable lighting.
    // glDisable(GL_LIGHTING);

    // Set the viewport.
    glViewport(0, 0, width, height);
}

void update(int /*value*/)
{
    jitana::jdwp_connection conn;
    try {
        conn.connect("localhost", "6100");
        conn.receive_insn_counters(stats);
        conn.close();
    }
    catch (std::runtime_error e) {
        std::cerr << "error: " << e.what() << "\n";
    }
    std::cout << std::flush;

    update_graphs();
    if (periodic_output) {
        static int output_cnt = 0;
        if (output_cnt-- == 0) {
            write_graphviz();
            write_traces();
            write_vtables();
            write_dtables();
            output_cnt = 20;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(50, update, 0);
}

int main(int argc, char** argv)
{
    // Create a GLUT window.
    glutInit(&argc, argv);
    glutInitWindowPosition(window_x, window_y);
    glutInitWindowSize(window_width, window_height);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    window = glutCreateWindow(title.empty() ? "Trace Visualizer"
                                            : title.c_str());
    glutMouseFunc(handle_mouse_event);
    glutMotionFunc(handle_motion_event);
    glutKeyboardFunc(handle_keyboard_event);
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);

    // Initialize OpenGL.
    init_opengl();

    {
        std::vector<std::string> filenames
                = {"../../../dex/framework/core.dex",
                   "../../../dex/framework/framework.dex",
                   "../../../dex/framework/framework2.dex",
                   "../../../dex/framework/ext.dex",
                   "../../../dex/framework/conscrypt.dex",
                   "../../../dex/framework/okhttp.dex",
                   "../../../dex/framework/core-junit.dex",
                   "../../../dex/framework/android.test.runner.dex",
                   "../../../dex/framework/android.policy.dex"};
        jitana::class_loader loader(system_loader_hdl, "SystemLoader",
                                    begin(filenames), end(filenames));
        vm.add_loader(loader);
    }

    {
        std::vector<std::string> filenames;
        jitana::class_loader loader(app_loader_hdl, "AppLoader",
                                    begin(filenames), end(filenames));
        vm.add_loader(loader, system_loader_hdl);
    }

    update(0);

    // Execute the main loop.
    glutMainLoop();
}
