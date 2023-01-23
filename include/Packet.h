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