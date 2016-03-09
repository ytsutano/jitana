/*
 * Copyright (c) 2015, 2016, Yutaka Tsutano
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

#include "jitana/util/jdwp.hpp"

#include <boost/asio/detail/socket_ops.hpp>

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <vector>

using namespace jitana;

jdwp_connection::jdwp_connection() : socket_(io_service_)
{
}

jdwp_connection::~jdwp_connection()
{
    close();
}

void jdwp_connection::connect(const std::string& host, const std::string& port)
{
    using boost::asio::ip::tcp;

    tcp::resolver resolver{io_service_};
    auto endpoint_it = resolver.resolve(tcp::resolver::query(host, port));
    boost::asio::connect(socket_, endpoint_it);

    // Send the handshake message.
    const std::string handshake = "JDWP-Handshake";
    boost::system::error_code ignored_error;
    boost::asio::write(socket_, boost::asio::buffer(handshake), ignored_error);

    // Verify the reply.
    std::array<char, 14> reply;
    boost::asio::read(socket_, boost::asio::buffer(reply));
    if (!std::equal(begin(handshake), end(handshake), begin(reply))) {
        close();
        throw std::runtime_error("JDWP handshake failed");
    }
}

void jdwp_connection::close()
{
    if (connected()) {
        socket_.close();
    }
}

void jdwp_connection::write(const void* data, size_t size)
{
    size_t len = boost::asio::write(socket_, boost::asio::buffer(data, size));
    if (len != size) {
        throw std::runtime_error("socket write failed");
    }
}

void jdwp_connection::write(uint8_t data)
{
    write(&data, 1);
}

void jdwp_connection::write(uint16_t data)
{
    data = boost::asio::detail::socket_ops::host_to_network_short(data);
    write(&data, 2);
}

void jdwp_connection::write(uint32_t data)
{
    data = boost::asio::detail::socket_ops::host_to_network_long(data);
    write(&data, 4);
}

void jdwp_connection::read(void* data, size_t size)
{
    size_t len = boost::asio::read(socket_, boost::asio::buffer(data, size));
    if (len != size) {
        throw std::runtime_error("socket read failed");
    }
}

uint8_t jdwp_connection::read_uint8()
{
    uint8_t data;
    read(&data, 1);
    return data;
}

uint16_t jdwp_connection::read_uint16()
{
    uint16_t data;
    read(&data, 2);
    return boost::asio::detail::socket_ops::host_to_network_short(data);
}

uint32_t jdwp_connection::read_uint32()
{
    uint32_t data;
    read(&data, 4);
    return boost::asio::detail::socket_ops::host_to_network_long(data);
}

std::string jdwp_connection::read_string(size_t size)
{
    std::string data(size, '\0');
    read(&data[0], size);
    return data;
}

std::string jdwp_connection::read_string()
{
    return read_string(read_uint32());
}

int jdwp_connection::send_command(jdwp_command_set command_set,
                                  jdwp_command command)
{
    if (!connected()) {
        throw std::runtime_error("not connected");
    }

    uint32_t packet_length = 11;
    uint8_t flags = 0;

    write(packet_length);
    write(id_);
    write(flags);
    write(command_set);
    write(command);

    ++id_;

    return id_;
}

void jdwp_connection::receive_reply_header(jdwp_reply_header& reply_header)
{
    if (!connected()) {
        throw std::runtime_error("not connected");
    }

    // Read the header.
    reply_header.length = read_uint32();
    reply_header.id = read_uint32();
    reply_header.flags = read_uint8();
    reply_header.error_code = read_uint16();
}
