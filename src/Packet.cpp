#include <iostream>
#include <sstream>
#include "Packet.h"

namespace avio
{

Packet::Packet() : m_pkt(nullptr) {}

Packet::~Packet()
{
    if (m_pkt) av_packet_free(&m_pkt);
}

Packet::Packet(AVPacket* src)
{
//std::cout << "test 01" << std::endl;
    if (m_pkt) av_packet_free(&m_pkt);
    //m_pkt = av_packet_clone(src);
    m_pkt = src;
//std::cout << "test 02" << std::endl;
}

Packet::Packet(const Packet& other)
{
//std::cout << "test 11" << std::endl;
    if (m_pkt) av_packet_free(&m_pkt);
    m_pkt = av_packet_clone(other.m_pkt);
//std::cout << "test 12" << std::endl;
}

Packet::Packet(Packet&& other) noexcept
{
//std::cout << "test 21" << std::endl;
    if (other.isValid()) {
        if (!m_pkt) m_pkt = av_packet_alloc();
        av_packet_move_ref(m_pkt, other.m_pkt);
    }
    else {
        if (m_pkt) av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
//std::cout << "test 22" << std::endl;
}

Packet& Packet::operator=(const Packet& other)
{
//std::cout << "test 31" << std::endl;
    if (m_pkt) av_packet_free(&m_pkt);
    m_pkt = av_packet_clone(other.m_pkt);
//std::cout << "test 32" << std::endl;
    return *this;
}

Packet& Packet::operator=(Packet&& other) noexcept
{
//std::cout << "test 41" << std::endl;
    if (other.isValid()) {
        if (!m_pkt) m_pkt = av_packet_alloc();
        av_packet_move_ref(m_pkt, other.m_pkt);
    }
    else {
        if (m_pkt) av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
//std::cout << "test 42" << std::endl;
    return *this;
}

std::string Packet::description() const
{
    std::stringstream str;
    if (isValid()) {
    str << " index: " << m_pkt->stream_index
        << " flags: " << m_pkt->flags
        << " pts: " << m_pkt->pts
        << " dts: " << m_pkt->dts
        << " size: " << m_pkt->size
        << " duration: " << m_pkt->duration;

    }
    else {
        str << "INVALID PKT" << std::endl;
    }

    return str.str();
}

}