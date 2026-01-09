/********************************************************************
* libavio/include/Player.hpp
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

#ifndef PLAYER_HPP
#define PLAYER_HPP

#define SDL_MAIN_HANDLED

#include <thread>
#include <map>

#include "Packet.hpp"
#include "Frame.hpp"
#include "Filter.hpp"
#include "Exception.hpp"
#include "Display.hpp"
#include "Audio.hpp"
#include "Reader.hpp"
#include "Decoder.hpp"
#include "Drain.hpp"
#include "Writer.hpp"

namespace avio {

class Player {
public:
    std::string uri;
    bool live_stream = true;
    bool headless = true;
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
    std::string str_hw_device_type;
    std::string str_video_filter;
    std::string str_audio_filter;
    std::map<std::string, std::string> metadata;
    int log_level = AV_LOG_QUIET; //AV_LOG_DEBUG
    bool crashed = false;

    std::function<void(float progress, const std::string& uri)> progressCallback = nullptr;
    std::function<void(const Frame&, const std::string& uri)> renderCallback = nullptr;
    std::function<void(const Frame&, const std::string& uri)> pyAudioCallback = nullptr;
    std::function<void(const std::string& uri)> mediaPlayingStarted = nullptr;
    std::function<void(const std::string& uri)> mediaPlayingStopped = nullptr;
    std::function<void(const std::string& uri)> packetDrop = nullptr;
    std::function<void(const std::string& msg, const std::string& uri)> infoCallback = nullptr;
    std::function<void(const std::string& msg, const std::string& uri, bool reconnect)> errorCallback = nullptr;

    bool request_reconnect = true;
    int buffer_size_in_seconds = 1;
    float file_start_from_seek = -1.0;
    int audio_driver_index = 0;
    bool disable_video = false;
    bool disable_audio = false;
    bool hidden = false;
    float volume = 1.0;
    bool mute = false;
    AVRational onvif_frame_rate;

    Reader* reader         = nullptr;
    Decoder* video_decoder = nullptr;
    Decoder* audio_decoder = nullptr;
    Filter* video_filter   = nullptr;
    Filter* audio_filter   = nullptr;
    Display* display       = nullptr;
    Audio* audio           = nullptr;
    Writer* writer         = nullptr;

    Player(const std::string& uri) : uri(uri) { av_log_set_level(log_level); }
    ~Player() { }

    static void clear_callback(void* player) {
        ((Player*)player)->clear_queues();
    }

    void clear_queues() {
        if (!reader) return;
        if (!reader->closed) {
            if (reader->audio_pkts) reader->audio_pkts->clear();
            if (reader->video_pkts) reader->video_pkts->clear();
            if (audio_decoder) {
                audio_decoder->frames->clear();
                AVPacket* flush = av_packet_alloc();
                flush->data = (uint8_t*)"FLUSH";
                audio_decoder->pkts->push(Packet(flush));
            }
            if (video_decoder) {
                video_decoder->frames->clear();
                AVPacket* flush = av_packet_alloc();
                flush->data = (uint8_t*)"FLUSH";
                video_decoder->pkts->push(Packet(flush));
            }
            if (audio_filter) audio_filter->output->clear();
            if (video_filter) video_filter->output->clear();
        }
    }

    void play() {
        std::thread* reader_thread        = nullptr;
        std::thread* video_decoder_thread = nullptr;
        std::thread* audio_decoder_thread = nullptr;
        std::thread* video_filter_thread  = nullptr;
        std::thread* audio_filter_thread  = nullptr;
        std::thread* display_thread       = nullptr;
        std::thread* writer_thread        = nullptr;

        Queue<Packet> video_pkts(128);
        Queue<Packet> audio_pkts(128);
        Queue<Frame>  decoded_video_frames(1);
        Queue<Frame>  decoded_audio_frames(1);
        Queue<Frame>  filtered_video_frames(1);
        Queue<Frame>  filtered_audio_frames(1);
        Queue<Packet> writer_pkts(128);

        try {
            reader = new Reader(uri);
            reader->clear_callback = clear_callback;
            reader->player = this;
            reader->live_stream = live_stream;
            reader->packetDrop = packetDrop;
            reader->infoCallback = infoCallback;
            reader->cache_size_in_seconds = buffer_size_in_seconds;
            reader->disable_audio = disable_audio;
            reader->disable_video = disable_video;

            if (!disable_video && !hidden)
                reader->video_pkts = &video_pkts;
            if (!disable_audio && !hidden)
                reader->audio_pkts = &audio_pkts;

            if (live_stream) {
                writer = new Writer(reader);
                writer->disable_audio = disable_audio;
                writer->disable_video = disable_video;
                writer->input = &writer_pkts;
                if (hidden) {
                    reader->writer_pkts = &writer_pkts;
                }
            }
            
            if (file_start_from_seek > 0.0)
                seek(file_start_from_seek);

            if (reader->has_video() && !disable_video && !hidden) {
                AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
                if (!str_hw_device_type.empty()) {
                    type = av_hwdevice_find_type_by_name(str_hw_device_type.c_str());
                    if (type != AV_HWDEVICE_TYPE_NONE)
                        std::cout << "using hw decoder " << str_hw_device_type << std::endl;
                }
                video_decoder = new Decoder(reader, AVMEDIA_TYPE_VIDEO, &video_pkts, &decoded_video_frames, type);
                if (live_stream)
                    video_decoder->writer_pkts = &writer_pkts;
                video_filter = new Filter(video_decoder, str_video_filter, &decoded_video_frames, &filtered_video_frames);
            }
            if (reader->has_audio() && !disable_audio && !hidden) {
                audio_decoder = new Decoder(reader, AVMEDIA_TYPE_AUDIO, &audio_pkts, &decoded_audio_frames);
                if (live_stream)
                    audio_decoder->writer_pkts = &writer_pkts;
                audio_filter = new Filter(audio_decoder, str_audio_filter, &decoded_audio_frames, &filtered_audio_frames);
            }
            
            reader_thread = new std::thread([&] { while (reader->read()) {} });
            
            if (video_decoder) {
                video_decoder_thread = new std::thread([&] { while (video_decoder->decode()) {} });
                video_filter_thread = new std::thread([&] { while (video_filter->filter()) {} });
            }
            if (audio_decoder) {
                audio_decoder_thread = new std::thread([&] { while (audio_decoder->decode()) {} });
                audio_filter_thread = new std::thread([&] { while (audio_filter->filter()) {} });
            }
            if (writer) {
                writer_thread = new std::thread([&] { while (writer->write()) {} });
            }

            if (reader->has_audio() && !disable_audio && !hidden) {
                audio = new Audio(reader, &filtered_audio_frames, audio_driver_index);
                audio->volume = volume;
                audio->mute = mute;
                audio->pyAudioCallback = pyAudioCallback;
                if (!reader->has_video())
                    audio->progressCallback = progressCallback;
            }

            if (mediaPlayingStarted) {
                mediaPlayingStarted(uri);
            }

            if (reader->has_video() && !disable_video && !hidden) {
                display = new Display(reader, &filtered_video_frames, headless);
                display->renderCallback = renderCallback;
                display->progressCallback = progressCallback;
                if (headless)
                    display_thread = new std::thread([&] { while (display->render()) {} });
                else 
                    while (display->render()) {}
            }

        }
        catch (const std::exception& e) {
            if (errorCallback) {
                crashed = true;
                errorCallback(e.what(), uri, request_reconnect);
                //infoCallback(e.what(), uri);
                if (reader) reader->terminate();
            }
            else {
                std::cout << uri << " player error: " << e.what() << std::endl;
                if (reader) reader->terminate();
            }
        }

        if (display_thread)       display_thread->join();
        if (audio_filter_thread)  audio_filter_thread->join();
        if (audio_decoder_thread) audio_decoder_thread->join();
        if (video_filter_thread)  video_filter_thread->join();
        if (video_decoder_thread) video_decoder_thread->join();
        if (reader_thread)        reader_thread->join();
        if (writer_thread)        writer_thread->join();

        if (display_thread)       { delete display_thread;       display_thread       = nullptr; }
        if (audio_filter_thread)  { delete audio_filter_thread;  audio_filter_thread  = nullptr; }
        if (audio_decoder_thread) { delete audio_decoder_thread; audio_decoder_thread = nullptr; }
        if (video_filter_thread)  { delete video_filter_thread;  video_filter_thread  = nullptr; }
        if (video_decoder_thread) { delete video_decoder_thread; video_filter_thread  = nullptr; }
        if (writer_thread)        { delete writer_thread;        writer_thread        = nullptr; }
        if (reader_thread)        { delete reader_thread;        reader_thread        = nullptr; }

        if (display)              { delete display;              display              = nullptr; }
        if (writer)               { delete writer;               writer               = nullptr; }
        if (video_filter)         { delete video_filter;         video_filter         = nullptr; }
        if (video_decoder)        { delete video_decoder;        video_decoder        = nullptr; }
        if (audio_filter)         { delete audio_filter;         audio_filter         = nullptr; }
        if (audio_decoder)        { delete audio_decoder;        audio_decoder        = nullptr; }

        // not clear that this is actually useful
        if (audio) {
            int count = 0;
            while (!audio->closed) {
                std::cout << "waiting for audio to close" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                count++;
                if (count > 200) {
                    std::cout << "Audio shutdown timeout failure" << std::endl;
                    break;
                }
            } 
            delete audio;
            audio = nullptr;
        }
        //////////////////////////////////////////
        if (reader)               { delete reader;               reader               = nullptr; }

        if (mediaPlayingStopped) {
            std::thread thread([&]() { 
                mediaPlayingStopped(uri);
            });
            thread.detach();
        }
    }

    void start() {
        std::thread thread([&]() { play(); });
        thread.detach();
    }

    void terminate() {
        if (!reader)
            return;
        std::thread thread([&]() { reader->terminate(); });
        thread.detach();
    }

    void seek(float pct) {
        if (!reader) return;
        if (reader->closed) return;
        AVRational time_base = reader->video_time_base();
        if (!reader->has_video())
            time_base = reader->audio_time_base();
        reader->seek_pts = (reader->start_time() + (pct * reader->duration()) / av_q2d(time_base)) / 1000;
        if (reader->paused) {
            reader->clear_callback(reader->player);
            if (display) {
                display->one_shot = true;
            }
            else {
                if (progressCallback) progressCallback(pct, uri);
            }
        }
    }

    int         width()            const { return reader ? reader->width() : -1; }
    int         height()           const { return reader ? reader->height() : -1; }
    bool        isPaused()         const { return reader ? reader->paused : false; }
    bool        isRecording()      const { return reader ? reader->recording : false; }
    bool        isMuted()          const { return audio ? audio->mute : false; }
    bool        hasVideo()         const { return reader ? reader->has_video() : false; }
    bool        hasAudio()         const { return reader ? reader->has_audio() : false; }
    int64_t     duration()         const { return reader ? reader->duration() : 0; }
    int         getVolume()        const { return audio ? (int)(100 * audio->volume) : 0; }
    std::string getAudioCodec()    const { return reader ? reader->str_audio_codec() : "unknown"; }


    std::string getStreamInfo() const {
        return reader ? reader->get_stream_info() : "no stream info available";
    }

    std::string getFFMPEGVersions() const {
        std::stringstream str;
        str << LIBAVCODEC_IDENT << " " 
            << LIBAVFILTER_IDENT << " "
            << LIBAVFORMAT_IDENT << " "
            << LIBAVUTIL_IDENT;
        return str.str();
    }

    std::vector<std::string> getHardwareDecoders() const {
        std::vector<std::string> result;
        enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            result.push_back(av_hwdevice_get_type_name(type));
        return result;
    }

    std::vector<std::string> getAudioDrivers() const {
        std::vector<std::string> result;
        int numDrivers = SDL_GetNumAudioDrivers();
        for (int i = 0; i < numDrivers; i++)
            result.push_back(SDL_GetAudioDriver(i));
        return result;
    }

    bool isCameraStream() const {
        if (uri.rfind("rtsp://", 0) == 0)
            return true;
        if (uri.rfind("http://", 0) == 0)
            return true;
        if (uri.rfind("https://", 0) == 0)
            return true;
        if (uri.rfind("RTSP://", 0) == 0)
            return true;
        if (uri.rfind("HTTP://", 0) == 0)
            return true;
        if (uri.rfind("HTTPS://", 0) == 0)
            return true;

        return false;
    }

    void setMetaData(const std::string& key, const std::string& value) { 
        metadata[key] = value; 
    }

    void togglePaused() { 
        if (reader) reader->paused = !reader->paused; 
    }

    void setVolume(int arg) {
        volume = (float)arg / 100.0f; 
        if (audio) audio->volume = (float)arg / 100.0f; 
    }

    void setMute(bool arg)  {
        mute = arg; 
        if (audio) audio->mute = arg; 
    }

    void clearBuffer() {
        if (reader) {
            if (reader->video_pkts) reader->video_pkts->clear();
            if (reader->audio_pkts) reader->audio_pkts->clear();
        }
    }

    void toggleRecording(const std::string& filename) {
        if (writer) writer->filename = filename;
        if (reader) reader->recording = !reader->recording;
    }

    void startFileBreak(const std::string& filename) {
        if (writer) writer->filename = filename;
        std::thread thread([&]() { file_break(); });
        thread.detach();
    }

    void file_break() {
        if (reader && writer) {
            if (reader->recording) {
                reader->recording = false;
                while (writer->fmt_ctx) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if (!writer)
                        return;
                }
                if (reader) reader->recording = true;
            }
        }
    }

    bool operator==(const Player& other) const {
        bool result = false;
        if (!uri.empty() && !other.uri.empty()) {
            if (uri == other.uri)
                result = true;
        }
        return result;
    }

    std::string toString() const { return uri; };
};

}

#endif // PLAYER_HPP