/********************************************************************
* libavio/include/Packet.h
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

#ifndef PACKET_H
#define PACKET_H

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace avio
{

class Packet
{
public:
    Packet();
    ~Packet();
    Packet(const Packet& other);
    Packet(Packet&& other) noexcept;
    Packet(AVPacket* src);
    Packet& operator=(const Packet& other);
    Packet& operator=(Packet&& other) noexcept;
    bool isValid() const { return m_pkt ? true : false; }
    std::string description() const;

    AVPacket* m_pkt = nullptr;
};

}

#endif // PACKET_H