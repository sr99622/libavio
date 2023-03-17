/********************************************************************
* libavio/include/Player.h
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
#include "Encoder.h"
#include "Writer.h"
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
    Encoder*  videoEncoder = nullptr;
    Encoder*  audioEncoder = nullptr;
    Writer*   writer       = nullptr;

    Queue<Packet>* vpq_reader;
    Queue<Frame>*  vfq_decoder;
    Queue<Frame>*  vfq_filter;
    Queue<Frame>*  vfq_display;
    Queue<Packet>* apq_reader;
    Queue<Frame>*  afq_decoder;
    Queue<Frame>*  afq_filter;
    Queue<Frame>*  afq_display;

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
    bool post_encode = false;
    bool hw_encoding = false;
    int keyframe_cache_size = 1;

    int vpq_size = 0;
    int apq_size = 0;

    bool disable_video = false;
    bool disable_audio = false;

    Player() { av_log_set_level(AV_LOG_PANIC); }
    ~Player() { }

    bool isPaused();
    bool isPiping();
    bool isEncoding();
    bool isRecording();
    void togglePaused();
    void togglePiping(const std::string& filename);
    void toggleEncoding(const std::string& filename);
    void toggleRecording(const std::string& filename);
    void key_event(int keyCode);
    void clear_queues();
    void clear_decoders();
    void run();
    void start();
    void seek(float arg);
    void setMute(bool arg);
    void setVolume(int arg);
    bool checkForStreamHeader();

    std::string getVideoCodec() const {
        std::string result = "Unknown codec";
        if (reader)
            result = reader->str_video_codec();
        return result;
    }

    int getVideoWidth() {
        int result = 0;
        if (reader)
            result = reader->width();
        return result;
    }

    int getVideoHeight() {
        int result = 0;
        if (reader)
            result = reader->height();
        return result;
    }

    int getVideoFrameRate() {
        int result = 0;
        if (reader)
            result = (int)av_q2d(reader->frame_rate());
        return result;
    }

    int getVideoBitrate() {
        int result = 0;
        if (reader)
            result = reader->video_bit_rate();
        return result;
    }

};


}

#endif // PLAYER_H