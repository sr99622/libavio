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
#include <map>

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
    //Writer*   writer       = nullptr;

    std::thread* reader_thread        = nullptr;
    std::thread* video_decoder_thread = nullptr;
    std::thread* video_filter_thread  = nullptr;
    //std::thread* video_encoder_thread = nullptr;
    std::thread* audio_decoder_thread = nullptr;
    std::thread* audio_filter_thread  = nullptr;
    //std::thread* audio_encoder_thread = nullptr;
    std::thread* display_thread       = nullptr;

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
    std::function<void(float progress, const std::string& uri)> progressCallback = nullptr;
    std::function<void(const Frame&, Player&)> renderCallback = nullptr;
    std::function<Frame(Frame&, Player&)> pyAudioCallback = nullptr;
    std::function<void(const std::string& uri)> mediaPlayingStarted = nullptr;
    std::function<void(const std::string& uri)> mediaPlayingStopped = nullptr;
    std::function<void(const std::string& uri)> packetDrop = nullptr;
    std::function<AudioStatus(void)> getAudioStatus = nullptr;
    std::function<void(AudioStatus)> setAudioStatus = nullptr;

    std::function<void(const std::string& msg, const std::string& uri)> infoCallback = nullptr;
    std::function<void(const std::string& msg, const std::string& uri, bool reconnect)> errorCallback = nullptr;

    uint64_t hWnd = 0;
    std::string uri;
    std::string video_filter = "null";
    std::string audio_filter = "anull";
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;

    bool running = false;
    bool stopped = false;
    bool request_reconnect = true;
    bool mute = false;
    int volume = 100;
    int64_t last_progress = -1;
    int64_t duration = -1;
    bool process_pause = false;
    bool post_encode = false;
    bool hw_encoding = false;
    bool adjust_time_base = false;
    int buffer_size_in_seconds = 1;

    float file_start_from_seek = -1.0;

    int vpq_size = 0;
    int apq_size = 0;

    int audio_driver_index = 0;
    bool disable_video = false;
    bool disable_audio = false;
    bool hidden = false;

    AVRational onvif_frame_rate;

    std::map<std::string, std::string> metadata;

    Player(const std::string& uri) : uri(uri) { av_log_set_level(AV_LOG_PANIC); }
    ~Player();
    bool operator==(const Player& other) const;
    std::string toString() const { return uri; };

    bool isPaused();
    bool isPiping();
    //bool isEncoding();
    bool isRecording();
    bool isMuted();
    void togglePaused();
    void startFileBreakPipe(const std::string& filename);
    void fileBreakPipe();
    int64_t pipeBytesWritten();
    void togglePiping(const std::string& filename);
    //void toggleEncoding(const std::string& filename);
    void toggleRecording(const std::string& filename);
    std::string pipeOutFilename() const;
    void key_event(int keyCode);
    void clear_queues();
    void clear_decoders();
    void unblock_filters();
    void run();
    void doit();
    void start();
    void seek(float arg);
    void setMute(bool arg);
    void setVolume(int arg);
    int  getVolume();
    bool isCameraStream();
    void setMetaData(const std::string& key, const std::string& value);
    std::string getVideoCodec() const;
    int getVideoWidth();
    int getVideoHeight();
    int getVideoFrameRate();
    int getVideoBitrate();
    bool hasAudio();
    bool hasVideo();
    AudioEncoding getAudioEncoding() const;
    std::string getAudioCodec() const;
    std::string getAudioChannelLayout() const;
    int getAudioBitrate();
    int getAudioSampleRate();
    int getAudioFrameSize();
    int getCacheSize();
    void clearCache();
    void signalStopped();
    void closeReader();
    std::string getStreamInfo() const;
    std::string getFFMPEGVersions() const;
    std::vector<std::string> getAudioDrivers() const;
    std::vector<std::string> getVideoDrivers() const;
    bool sync_audio = false;

};


}

#endif // PLAYER_H