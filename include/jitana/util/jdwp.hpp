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

#ifndef JITANA_JDWP_HPP
#define JITANA_JDWP_HPP

#include <cstdint>
#include <memory>
#include <boost/asio.hpp>

namespace jitana {
    using jdwp_command_set = uint8_t;
    using jdwp_command = uint8_t;

    class jdwp_connection {
    public:
        jdwp_connection();
        ~jdwp_connection();
        void connect(const std::string& host, const std::string& port);
        void close();
        int send_command(jdwp_command_set command_set, jdwp_command command);
        void receive_reply_header();

        template <typename Visitor>
        void receive_insn_counters(Visitor& visitor);

        bool connected() const
        {
            return socket_.is_open();
        }

        void write(const void* data, size_t size);
        void write(uint8_t data);
        void write(uint16_t data);
        void write(uint32_t data);

        void read(void* data, size_t size);
        uint8_t read_uint8();
        uint16_t read_uint16();
        uint32_t read_uint32();
        std::string read_string(size_t size);

    private:
        boost::asio::io_service io_service_;
        boost::asio::ip::tcp::socket socket_;
        uint32_t id_ = 0;
    };
}

template <typename Visitor>
void jitana::jdwp_connection::receive_insn_counters(Visitor& visitor)
{
    send_command(225, 1);
    receive_reply_header();

    // Read the payload.
    for (;;) {
        uint32_t source_filename_size = read_uint32();
        if (source_filename_size == 0xffffffff) {
            break;
        }
        std::string source_filename = read_string(source_filename_size);
        std::string filename = read_string(read_uint32());
        visitor.enter_dex_file(source_filename, filename);

        for (;;) {
            auto dex_offset = read_uint32();
            if (dex_offset == 0xffffffff) {
                break;
            }
            auto counter = read_uint16();
            visitor.insn(dex_offset, counter);
        }

        visitor.exit_dex_file();
    }
}

#endif
