/********************************************************************
* libavio/include/avio.h
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

#ifndef PLAYER_H
#define PLAYER_H

#include "Exception.h"
#include "Queue.h"
#include "Reader.h"
#include "Decoder.h"
#include "Pipe.h"
#include "Filter.h"
#include "Display.h"
#include "Packet.h"
#include <iomanip>
#include <functional>
#include <map>

#define P ((Player*)player)

namespace avio
{

typedef std::map<std::string, Queue<Packet>*> PKT_Q_MAP;
typedef std::map<std::string, Queue<Frame>*> FRAME_Q_MAP;

class Player
{

public:
    Reader*   reader       = nullptr;
    Decoder*  videoDecoder = nullptr;
    Decoder*  audioDecoder = nullptr;
    Filter*   videoFilter  = nullptr;
    Filter*   audioFilter  = nullptr;
    Display*  display      = nullptr;

    PKT_Q_MAP pkt_queues;
    FRAME_Q_MAP frame_queues;
    std::vector<std::string> pkt_q_names;
    std::vector<std::string> frame_q_names;
    
    std::vector<std::thread*> ops;

    bool running = false;

    int width = 0;
    int height = 0;

    Player() { av_log_set_level(AV_LOG_PANIC); }
    ~Player() { }

    bool isPaused();
    bool isPiping();
    void setMute(bool arg);
    void setVolume(int arg);
    void togglePaused();
    void seek(float arg);
    void toggle_pipe_out(const std::string& filename);
    void key_event(int keyCode);
    void add_reader(Reader& reader_in);
    void add_decoder(Decoder& decoder_in);
    void add_filter(Filter& filter_in);
    void add_display(Display& display_in);
    void clear_queues();
    void clear_decoders();
    void cleanup();
    void run();
    void start();
    static void twink(void* caller);

};


}

#endif // PLAYER_H