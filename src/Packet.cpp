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
    if (m_pkt) av_packet_free(&m_pkt);
    //m_pkt = av_packet_clone(src);
    m_pkt = src;
}

Packet::Packet(const Packet& other)
{
    if (m_pkt) av_packet_free(&m_pkt);
    m_pkt = av_packet_clone(other.m_pkt);
}

Packet::Packet(Packet&& other) noexcept
{
    if (other.isValid()) {
        if (!m_pkt) m_pkt = av_packet_alloc();
        av_packet_move_ref(m_pkt, other.m_pkt);
    }
    else {
        if (m_pkt) av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
}

Packet& Packet::operator=(const Packet& other)
{
    if (m_pkt) av_packet_free(&m_pkt);
    m_pkt = av_packet_clone(other.m_pkt);
    return *this;
}

Packet& Packet::operator=(Packet&& other) noexcept
{
    if (other.isValid()) {
        if (!m_pkt) m_pkt = av_packet_alloc();
        av_packet_move_ref(m_pkt, other.m_pkt);
    }
    else {
        if (m_pkt) av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
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