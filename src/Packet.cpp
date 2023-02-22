/********************************************************************
* libavio/src/Packet.cpp
*
* Copyright (c) 2022  Stephen Rhodes
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*********************************************************************/

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