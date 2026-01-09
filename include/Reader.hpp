/********************************************************************
* libavio/include/Reader.hpp
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

#ifndef READER_HPP
#define READER_HPP

#include <iostream>
#include <functional>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "Packet.hpp"
#include "Queue.hpp"
#include "Filter.hpp"
#include "Exception.hpp"

struct CallbackParams {
    time_t timeout_start = time(nullptr);
    bool triggered = false;
};

#define MAX_TIMEOUT 5
static int interrupt_callback(void *ctx) {
    CallbackParams* callback_params = (CallbackParams*)ctx;
    time_t diff = time(nullptr) - callback_params->timeout_start;
    if (diff > MAX_TIMEOUT) {
        callback_params->triggered = true;
        return 1;
    }
    return 0;
}

namespace avio {

class Reader {
public:
    std::string uri;
    Queue<Packet>* video_pkts = nullptr;
    Queue<Packet>* audio_pkts = nullptr;
    Queue<Packet>* writer_pkts = nullptr;
    int video_stream_index = -1;
    int audio_stream_index = -1;
    AVFormatContext* fmt_ctx = nullptr;
    AVPacket* pkt = nullptr;
    //time_t timeout_start = time(nullptr);
    int64_t last_audio_rts = INT64_MAX;
    int64_t last_video_rts = INT64_MAX;
    int64_t last_audio_pts = AV_NOPTS_VALUE;
    int64_t last_video_pts = AV_NOPTS_VALUE;
    bool terminated = false;
    bool closed = false;
    AVPixelFormat output_pix_fmt = AV_PIX_FMT_NONE;
    ExceptionChecker ex;
    int cache_size_in_seconds = 10;
    bool recording = false;
    bool live_stream = true;
    bool paused = false;
    bool disable_video = false;
    bool disable_audio = false;
    CallbackParams callback_params;

    std::function<void(const std::string& uri)> packetDrop = nullptr;
    std::function<void(const std::string& msg, const std::string& uri)> infoCallback = nullptr;

    std::function<void(void*)> clear_callback = nullptr;
    void* player = nullptr;
    int64_t seek_pts = AV_NOPTS_VALUE;

    Reader(const std::string& uri) : uri(uri) {
        AVDictionary* opts = nullptr;
        int timeout_us = MAX_TIMEOUT * 1000000;
        av_dict_set_int(&opts, "timeout", timeout_us, 0);
        ex.ck(avformat_open_input(&fmt_ctx, uri.c_str(), nullptr, &opts), AOI);
        av_dict_free(&opts);
        AVIOInterruptCB cb = { interrupt_callback, &callback_params };
        fmt_ctx->interrupt_callback = cb;
        ex.ck(avformat_find_stream_info(fmt_ctx, nullptr), AFSI);
        video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        ex.ck((pkt = av_packet_alloc()), APA);
    }

    ~Reader() {
        if (fmt_ctx) {
            avformat_close_input(&fmt_ctx);
            avformat_free_context(fmt_ctx);
        }
        if (pkt) av_packet_free(&pkt);
    }

    int read() {
        try {
            callback_params.timeout_start = time(nullptr);

            if (seek_pts != AV_NOPTS_VALUE) {
                clear_callback(player);
                int flags = AVSEEK_FLAG_FRAME;
                int seek_index = video_stream_index;
                int64_t last_pts = last_video_pts;
                if (!has_video()) {
                    seek_index = audio_stream_index;
                    last_pts = last_audio_pts;
                }
                if (seek_pts < last_pts)
                    flags |= AVSEEK_FLAG_BACKWARD;
                av_seek_frame(fmt_ctx, seek_index, seek_pts, flags);
                ex.eof(av_read_frame(fmt_ctx, pkt), ARF);
                clear_callback(player);
                seek_pts = AV_NOPTS_VALUE;
            }
            else {
                ex.eof(av_read_frame(fmt_ctx, pkt), ARF);
            }
            if (closed)
                return 0;

            if (writer_pkts) {
                writer_pkts->push(Packet(pkt));
            }
            else {
                if (pkt->stream_index == video_stream_index && video_pkts) {
                    last_video_pts = pkt->pts;
                    if (packetDrop && video_pkts->full()) {
                        packetDrop(uri);
                    }
                    else {
                        video_pkts->push(Packet(pkt));
                    }
                }
                else if (pkt->stream_index == audio_stream_index && audio_pkts) {
                    last_audio_pts = pkt->pts;
                    audio_pkts->push(Packet(pkt));
                }
                else {
                    Packet term(pkt);
                }
            }
        }
        catch (const std::exception& e) {
            if (!strcmp(e.what(), "EOF")) {
                if (callback_params.triggered) {
                    infoCallback("Reader terminated by timeout", uri);
                }
                closed = true;
                seek_pts = AV_NOPTS_VALUE;
                if (video_pkts) video_pkts->push(Packet(nullptr));
                if (audio_pkts) audio_pkts->push(Packet(nullptr));
                if (writer_pkts) writer_pkts->push(Packet(nullptr));
            }
            else {
                std::cout << uri << " read exception " << e.what() << std::endl;
                terminate();
            }
        }
        return closed ? 0 : 1;
    }

    void terminate() {
        if (video_pkts && !closed && !terminated) {
            video_pkts->clear();
            video_pkts->push(Packet(nullptr));
            video_pkts = nullptr;
        }
        if (audio_pkts && !closed && !terminated) {
            audio_pkts->clear();
            audio_pkts->push(Packet(nullptr));
            audio_pkts = nullptr;
        }
        if (writer_pkts) {
            writer_pkts->push(Packet(nullptr));
            writer_pkts = nullptr;
        }
        closed = true;
        terminated = true;
    }

    int64_t real_time(int stream_index, int64_t pts) {
        // result is returned in milliseconds
        int64_t result = -1;
        if (stream_index < fmt_ctx->nb_streams) {
            AVStream* stream = fmt_ctx->streams[stream_index];
            if (stream && (pts != AV_NOPTS_VALUE)) {
                double factor = 1000 * av_q2d(stream->time_base);
                int64_t start_pts = ((stream->start_time == AV_NOPTS_VALUE) ? 0 : stream->start_time);
                result = factor * (pts - start_pts);
            }
        }
        return result;
    }

    int64_t pts_from_real_time(int stream_index, int64_t real_time) {
        // input real_time in milliseconds
        int64_t result = AV_NOPTS_VALUE;
        if (stream_index < fmt_ctx->nb_streams) {
            AVStream* stream = fmt_ctx->streams[stream_index];
            if (stream) {
                double factor = 1000 * av_q2d(stream->time_base);
                int64_t start_pts = ((stream->start_time == AV_NOPTS_VALUE) ? 0 : stream->start_time);
                result = (int64_t)(((double)real_time / factor) + start_pts);
            }
        }
        return result;
    }

    void update_rt(int stream_index, int64_t rts) {
        if (stream_index == audio_stream_index)
            last_audio_rts = rts;
        if (stream_index == video_stream_index)
            last_video_rts = rts;
    }

    int64_t duration()   const { return fmt_ctx->duration * AV_TIME_BASE / 1000000000; }
    int64_t start_time() const { return ((fmt_ctx->start_time == AV_NOPTS_VALUE) ? 0 : fmt_ctx->start_time) * AV_TIME_BASE / 1000000000; }

    bool has_video() const { return ((video_stream_index >= 0)); }
    int           width()           const { return has_video() ? fmt_ctx->streams[video_stream_index]->codecpar->width : -1; }
    int           height()          const { return has_video() ? fmt_ctx->streams[video_stream_index]->codecpar->height : -1; }
    AVRational    frame_rate()      const { return has_video() ? fmt_ctx->streams[video_stream_index]->avg_frame_rate : av_make_q(0, 0); }
    double        fps()             const { return has_video() ? av_q2d(frame_rate()) : -1.0; }
    AVPixelFormat pix_fmt()         const { return has_video() ? (AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format : AV_PIX_FMT_NONE; }
    std::string   str_pix_fmt()     const { return has_video() ? get_string(av_get_pix_fmt_name((AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format)) : "invalid"; }
    AVCodecID     video_codec()     const { return has_video() ? fmt_ctx->streams[video_stream_index]->codecpar->codec_id : AV_CODEC_ID_NONE; }
    std::string   str_video_codec() const { return has_video() ? avcodec_get_name(fmt_ctx->streams[video_stream_index]->codecpar->codec_id) : "invalid"; }
    int64_t       video_bit_rate()  const { return has_video() ? fmt_ctx->streams[video_stream_index]->codecpar->bit_rate : -1; }
    AVRational    video_time_base() const { return has_video() ? fmt_ctx->streams[video_stream_index]->time_base : av_make_q(0, 0); }

    bool has_audio() const { return ((audio_stream_index >= 0)); }
    int            channels()           const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->codecpar->ch_layout.nb_channels : -1; }
    int            sample_rate()        const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->codecpar->sample_rate : -1; }
    int            frame_size()         const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->codecpar->frame_size : -1; }
    AVSampleFormat sample_format()      const { return has_audio() ? (AVSampleFormat)fmt_ctx->streams[video_stream_index]->codecpar->format : AV_SAMPLE_FMT_NONE; }
    std::string    str_sample_format()  const { return has_audio() ? get_string(av_get_sample_fmt_name((AVSampleFormat)fmt_ctx->streams[audio_stream_index]->codecpar->format)) : "invalid"; }
    AVCodecID      audio_codec()        const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->codecpar->codec_id : AV_CODEC_ID_NONE; }
    std::string    str_audio_codec()    const { return has_audio() ? avcodec_get_name(fmt_ctx->streams[audio_stream_index]->codecpar->codec_id) : "invalid"; }
    int64_t        audio_bit_rate()     const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->codecpar->bit_rate : -1; }
    AVRational     audio_time_base()    const { return has_audio() ? fmt_ctx->streams[audio_stream_index]->time_base : av_make_q(0, 0); }
    std::string    str_channel_layout() const {
        char result[256] = { 0 };
        if (has_audio())
            av_channel_layout_describe(&fmt_ctx->streams[audio_stream_index]->codecpar->ch_layout, result, 256);
        return std::string(result);
    }

    std::string get_string(const char* arg) const {
        if (arg)
            return std::string(arg);
        return std::string("invalid");
    }

    std::string get_stream_info() const {
        std::stringstream str;
        if (has_video()) {
            str  << "<h4>Video Stream Parameters</h4>"
                << "Video Codec: " << str_video_codec() << "<br>"
                << "Pixel Format: " << str_pix_fmt() << "<br>"
                << "Resolution: " << width() << " x " << height() << "<br>"
                << "Frame Rate: " << av_q2d(frame_rate())
                //<< "Bitrate: " << video_bit_rate()
            ;
            if (disable_video)
                str << "<br><b>* Video has been disabled</b>";
        }
        else {
            if (video_stream_index < 0)
                str << "<br><b>No Video Stream Found<b>";
        }
        if (has_audio()) {
            str << "<h4>Audio Stream Parameters</h4>"
                << "Audio Codec: " << str_audio_codec() << "<br>"
                << "Sample Format: " << str_sample_format() << "<br>"
                << "Channel Layout: " << str_channel_layout() << "<br>"
                << "Channels: " << channels() << "<br>"
                << "Sample Rate: " << sample_rate() << "<br>"
                << "Time Base: " << audio_time_base().num << " : " << audio_time_base().den
                //<< "Bitrate: " << audio_bit_rate() << "<br>"
                //<< "Frame Size: " << frame_size()
            ;
            if (disable_audio)
                str << "<br><b>* Audio has been disabled</b>";
        }
        else {
            if (audio_stream_index < 0)
                str << "<br><b>No Audio Stream Found</b>";
        }
        return str.str();
    }
};

}
#endif // READER_HPP