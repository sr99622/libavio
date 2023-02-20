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
#include <functional>

#define P ((Player*)player)

namespace avio
{

class Player
{

public:
    Reader*   reader       = nullptr;
    Decoder*  videoDecoder = nullptr;
    Decoder*  audioDecoder = nullptr;
    Filter*   videoFilter  = nullptr;
    Filter*   audioFilter  = nullptr;
    Display*  display      = nullptr;

    Queue<Packet>* vpq_reader;
    Queue<Frame>*  vfq_decoder;
    Queue<Frame>*  vfq_filter;
    Queue<Packet>* apq_reader;
    Queue<Frame>*  afq_decoder;
    Queue<Frame>*  afq_filter;

    std::function<int(void)> width = nullptr;
    std::function<int(void)> height = nullptr;
    std::function<void(float)> progressCallback = nullptr;
    std::function<void(const Frame&)> renderCallback = nullptr;
    std::function<Frame(Frame&)> pythonCallback  = nullptr;
    std::function<void(int64_t)> cbMediaPlayingStarted = nullptr;
    std::function<void(void)> cbMediaPlayingStopped = nullptr;
  	std::function<void(const std::string&)> infoCallback = nullptr;
	std::function<void(const std::string&)> errorCallback = nullptr;

    uint64_t hWnd = 0;
    std::string uri;
    std::string video_filter = "null";
    std::string audio_filter = "anull";
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;

    bool running = false;
    bool mute = false;
    int volume = 100;
    int last_progress = 0;

    int vpq_size = 0;
    int apq_size = 0;

    bool disable_video = false;
    bool disable_audio = false;

    Player() { av_log_set_level(AV_LOG_PANIC); }
    ~Player() { }

    bool isPaused();
    bool isPiping();
    void togglePaused();
    void togglePiping(const std::string& filename);
    void key_event(int keyCode);
    void clear_queues();
    void clear_decoders();
    void run();
    void start();
    void seek(float arg);
    void setMute(bool arg);
    void setVolume(int arg);
    bool checkForStreamHeader(const char* name);

};


}

#endif // PLAYER_H