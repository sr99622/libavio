/********************************************************************
* libavio/include/Drain.hpp
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

#ifndef DRAIN_HPP
#define DRAIN_HPP

#include <iostream>
#include <functional>

#include "Queue.hpp"
#include "Packet.hpp"
#include "Frame.hpp"

namespace avio {

template <typename T>
class Drain {
public:
    Queue<T>* q;
    int count = 0;
    bool closed = false;

    std::function<void(Packet&&)> pkt_handle = nullptr;
    std::function<void(Frame&&)> frame_handle = nullptr;

    Drain(Queue<T>* q) : q(q) { }

    int drain() {
        if constexpr(std::is_same_v<T, Frame>) {
            Frame frame = std::move(q->pop());
            if (frame_handle) frame_handle(std::move(frame));
            if (frame.is_null())
                closed = true;
        }
        else if constexpr(std::is_same_v<T, Packet>) {
            Packet pkt = std::move(q->pop());
            if (pkt_handle) pkt_handle(std::move(pkt));
            if (pkt.is_null())
                closed = true;
        }
        else {
            q->pop();
            count++;
            std::cout << "drain count: " << count << std::endl;
        }
        
        return closed ? 0 : 1;
    }
};

}

#endif // DRAIN_HPP