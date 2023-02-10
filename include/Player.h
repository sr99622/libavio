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

    std::vector<std::thread*> ops;

    std::function<int(void)> cbWidth = nullptr;
    std::function<int(void)> cbHeight = nullptr;
    std::function<void(float)> cbProgress = nullptr;
    std::function<void(const Frame&)> renderCallback = nullptr;
    std::function<Frame(Frame&)> cbFrame  = nullptr;
  	std::function<void(const std::string&)> cbInfo = nullptr;
	std::function<void(const std::string&)> cbError = nullptr;
    std::function<void(int64_t)> cbMediaPlayingStarted = nullptr;
    std::function<void(void)> cbMediaPlayingStopped = nullptr;

    uint64_t hWnd = 0;
    std::string uri;
    std::string video_filter = "null";
    std::string audio_filter = "anull";
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;

    bool running = false;

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
    void clear_queues();
    void clear_decoders();
    void run();
    void start();
    static void hook(void* caller);

};


}

#endif // PLAYER_H