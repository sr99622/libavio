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

#ifndef PROCESS_H
#define PROCESS_H

#include "Exception.h"
#include "Queue.h"
#include "Reader.h"
#include "Decoder.h"
#include "Encoder.h"
#include "Writer.h"
#include "Pipe.h"
#include "Filter.h"
#include "Display.h"
#include <iomanip>
#include <functional>
#include <map>

namespace avio
{


typedef std::map<std::string, Queue<AVPacket*>*> PKT_Q_MAP;
typedef std::map<std::string, Queue<Frame>*> FRAME_Q_MAP;

class Process
{

public:
    Reader*   reader       = nullptr;
    Decoder*  videoDecoder = nullptr;
    Decoder*  audioDecoder = nullptr;
    Filter*   videoFilter  = nullptr;
    Filter*   audioFilter  = nullptr;
    Writer*   writer       = nullptr;
    Encoder*  videoEncoder = nullptr;
    Encoder*  audioEncoder = nullptr;
    Display*  display      = nullptr;
    void*     widget       = nullptr;

    PKT_Q_MAP pkt_queues;
    FRAME_Q_MAP frame_queues;
    std::vector<std::string> pkt_q_names;
    std::vector<std::string> frame_q_names;
    std::vector<std::string> frame_q_drain_names;
    std::vector<std::string> pkt_q_drain_names;
    std::vector<std::string> merge_filenames;
    
    int interleaved_q_size = 0;
    std::string interleaved_q_name;
    bool muxing = false;
    std::string mux_video_q_name;
    std::string mux_audio_q_name;

    std::vector<std::thread*> ops;

    std::function<void(Process*)> cameraTimeoutCallback = nullptr;
    std::function<void(Process*, const std::string&)> openWriterFailedCallback = nullptr;

    bool running = false;

    Process() { av_log_set_level(AV_LOG_PANIC); }
    ~Process() { }

    bool isPaused();
    void setMute(bool arg);
    void setVolume(int arg);
    void togglePaused();
    void seek(float arg);
    void toggle_pipe_out(const std::string& filename);
    void key_event(int keyCode);
    void add_reader(Reader& reader_in);
    void add_decoder(Decoder& decoder_in);
    void add_filter(Filter& filter_in);
    void add_encoder(Encoder& encoder_in);
    void add_display(Display& display_in);
    void add_frame_drain(const std::string& frame_q_name);
    void add_packet_drain(const std::string& pkt_q_name);
    void cleanup();
    void run();

};


}

#endif // PROCESS_H