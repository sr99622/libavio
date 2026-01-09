/********************************************************************
* libavio/include/Packet.hpp
*
* Copyright (c) 2025  Stephen Rhodes
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

#ifndef PACKET_HPP
#define PACKET_HPP

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "Exception.hpp"

namespace avio {

class Packet {
public:
    AVPacket* pkt = nullptr;
    ExceptionChecker ex;

    Packet() {
        ex.ck((pkt = av_packet_alloc()), APA);
    }

    Packet(AVPacket* raw_pkt) {
        if (raw_pkt) {
            ex.ck((pkt = av_packet_alloc()), APA);
            av_packet_move_ref(pkt, raw_pkt);
        }
    }

    Packet(const Packet& other) {
        ex.ck((pkt = av_packet_clone(other.pkt)), APC);
    }

    Packet(Packet&& other) noexcept {
        pkt = other.pkt;
        other.pkt = nullptr;
    }

    Packet& operator=(const Packet& other) {
        if (this != &other) {
            if (pkt) av_packet_free(&pkt);
            ex.ck((pkt = av_packet_clone(other.pkt)), APC);
        }
        return *this;
    }

    Packet& operator=(Packet&& other) noexcept {
        if (this != &other) {
            av_packet_free(&pkt);
            pkt = other.pkt;
            other.pkt = nullptr;
        }
        return *this;
    }

    ~Packet() {
        if (pkt) av_packet_free(&pkt);
    }

    bool       is_null()      const { return pkt == nullptr; }
    int64_t    pts()          const { return pkt ? pkt->pts : AV_NOPTS_VALUE; }
    int64_t    dts()          const { return pkt ? pkt->dts : AV_NOPTS_VALUE; }
    int        stream_index() const { return pkt ? pkt->stream_index : -1; }
    int64_t    duration()     const { return pkt ? pkt->duration : 0; }
    int        size()         const { return pkt ? pkt->size : 0; }
    int        flags()        const { return pkt ? pkt->flags : 0; }
    bool       is_key_frame() const { return pkt ? pkt->flags & AV_PKT_FLAG_KEY : false; }
    AVRational time_base()    const { return pkt ? pkt->time_base : av_make_q(0, 0); }
    
};

}

#endif // PACKET_HPP
