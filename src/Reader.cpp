/********************************************************************
* libavio/src/Reader.cpp
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

extern "C"
{
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>
}

#include "Reader.h"

#define MAX_TIMEOUT 5

time_t timeout_start = time(nullptr);
std::string timeout_msg;

static int interrupt_callback(void *ctx)
{
    avio::Reader* reader = (avio::Reader*)ctx;
    time_t diff = time(nullptr) - timeout_start;

    if (diff > MAX_TIMEOUT) {
        timeout_msg = "camera timeout";
        return 1;
    }
    return 0;
}

namespace avio {

Reader::Reader(const char* filename)
{
    try {
        AVDictionary* opts = nullptr;

        if (LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(59, 0, 0)) 
            av_dict_set(&opts, "timeout", "5000000", 0);
        else 
            av_dict_set(&opts, "stimeout", "5000000", 0);

        ex.ck(avformat_open_input(&fmt_ctx, filename, nullptr, &opts), CmdTag::AOI);
    
        av_dict_free(&opts);
        timeout_start = time(nullptr);
        AVIOInterruptCB cb = { interrupt_callback, this };
        fmt_ctx->interrupt_callback = cb;

        ex.ck(avformat_find_stream_info(fmt_ctx, nullptr), CmdTag::AFSI);

        video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    }
    catch (const Exception& e) {
        std::string msg(e.what());
        std::string mark("-138");
        if (msg.find(mark) != std::string::npos)
            msg = "No connection to server";
        std::stringstream str;
        str << "Reader constructor exception: " << msg;
        throw Exception(str.str());
    }
}

Reader::~Reader()
{
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
}

AVPacket* Reader::read()
{
    int ret = 0;
    AVPacket* pkt = av_packet_alloc();

    try {
        if (!fmt_ctx) throw Exception("fmt_ctx null");
        timeout_start = time(nullptr);
        ex.ck(ret = av_read_frame(fmt_ctx, pkt), CmdTag::ARF);
        timeout_start = time(nullptr);
    }
    catch (const Exception& e) {
        av_packet_free(&pkt);
        pkt = nullptr;
        if (ret != AVERROR_EOF) {
            std::stringstream str;
            str << "Packet read error: ";
            if (!timeout_msg.empty())
                str << timeout_msg;
            else
                str << e.what();
            throw Exception(str.str());
        }
    }

    return pkt;
}

AVPacket* Reader::seek()
{
    int flags = AVSEEK_FLAG_FRAME;
    if (seek_target_pts < last_video_pts)
        flags |= AVSEEK_FLAG_BACKWARD;

    try {
        ex.ck(av_seek_frame(fmt_ctx, seek_stream_index(), seek_target_pts, flags), CmdTag::ASF);
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Reader seek exception: " << e.what();
        if (infoCallback) infoCallback(str.str());
        else std::cout << str.str() << std::endl;
        return nullptr;
    }

    AVPacket* pkt = nullptr;
    while (pkt = read()) {
        if (pkt->stream_index == seek_stream_index()) {
            seek_found_pts = pkt->pts;
            break;
        }
    }
    seek_target_pts = AV_NOPTS_VALUE;

    return pkt;
}

void Reader::request_seek(float pct)
{
    seek_target_pts = (start_time() + (pct * duration()) / av_q2d(fmt_ctx->streams[seek_stream_index()]->time_base)) / 1000;
}

bool Reader::seeking() 
{
    return seek_target_pts != AV_NOPTS_VALUE || seek_found_pts != AV_NOPTS_VALUE;
}

void Reader::clear_pkts_cache(int mark)
{
    while (pkts_cache.size() > mark) {
        AVPacket* jnk = pkts_cache.front();
        pkts_cache.pop_front();
        av_packet_free(&jnk);
    }
}

void Reader::close_pipe()
{
    if (pipe) pipe->close();
    pipe = nullptr;
}

void Reader::pipe_write(AVPacket* pkt)
{
    try {
        if (!pipe) {
            if (strcmp(str_audio_codec(), "aac")) {
                std::stringstream str;
                str << "Warning, " << str_audio_codec() 
                << " audio codec is not supported for recording, only video packets will be recorded."
                << "\nOnly aac codec is supported for this function";
                if (infoCallback) infoCallback(str.str());
                disable_audio = true;
            }
            pipe = new Pipe(fmt_ctx, (disable_video ? -1 : video_stream_index), (disable_audio ? -1 : audio_stream_index));
            pipe->infoCallback = infoCallback;
            pipe->errorCallback = errorCallback;
            pipe->open(pipe_out_filename);
            while (pkts_cache.size() > 0) {
                AVPacket* tmp = pkts_cache.front();
                pkts_cache.pop_front();
                pipe->write(tmp);
                av_packet_free(&tmp);
            }
        }

        if (pipe) {
            AVPacket* tmp = av_packet_clone(pkt);
            pipe->write(tmp);
            av_packet_free(&tmp);
        }
    }
    catch (const Exception& e) {
        if (pipe->opened)
            close_pipe();
        else {
            if (pipe->fmt_ctx) avformat_free_context(pipe->fmt_ctx);
        }
        pipe = nullptr;
        request_pipe_write = false;

        std::stringstream str;
        str << "Record function failure: " << e.what();
        if (errorCallback) errorCallback(str.str());
        else std::cout << str.str() << std::endl;
    }
}

void Reader::fill_pkts_cache(AVPacket* pkt)
{
    if (pkt->stream_index == video_stream_index) {
        if (pkt->flags) {
            if (++keyframe_count >= keyframe_cache_size) {
                clear_pkts_cache(keyframe_marker);
                keyframe_count--;
            }
            keyframe_marker = pkts_cache.size();
        }
    }
    AVPacket* tmp = av_packet_clone(pkt);
    pkts_cache.push_back(tmp);
}

void Reader::start_from(int milliseconds)
{
    request_seek((start_time() + milliseconds) / (float)duration());
}

void Reader::end_at(int milliseconds)
{
    stop_play_at_pts = ((start_time() + milliseconds) / av_q2d(fmt_ctx->streams[seek_stream_index()]->time_base)) / 1000;
}

int Reader::seek_stream_index()
{
    return (video_stream_index >= 0 ? video_stream_index : audio_stream_index);
}

int64_t Reader::start_time()
{
    int64_t start_pts = (fmt_ctx->start_time == AV_NOPTS_VALUE ? 0 : fmt_ctx->start_time);
    return start_pts * AV_TIME_BASE / 1000000000;
}

int64_t Reader::duration()
{
    return fmt_ctx->duration * AV_TIME_BASE / 1000000000;
}

int64_t Reader::bit_rate()
{
    return fmt_ctx->bit_rate;
}

bool Reader::has_video()
{
    return video_stream_index >= 0;
}

int Reader::width()
{
    int result = -1;
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->codecpar->width;
    return result;
}

int Reader::height()
{
    int result = -1;
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->codecpar->height;
    return result;
}

AVRational Reader::frame_rate()
{
    AVRational result = av_make_q(0, 0);
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->avg_frame_rate;
    return result;
}

AVPixelFormat Reader::pix_fmt()
{
    AVPixelFormat result = AV_PIX_FMT_NONE;
    if (video_stream_index >= 0)
        result = (AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format;
    return result;
}

const char* Reader::str_pix_fmt()
{
    const char* result = "unknown pixel format";
    if (video_stream_index >= 0) {
        const char* name = av_get_pix_fmt_name((AVPixelFormat)fmt_ctx->streams[video_stream_index]->codecpar->format);
        if (name)
            result = name;
    }
    return result;
}

AVCodecID Reader::video_codec()
{
    AVCodecID result = AV_CODEC_ID_NONE;
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->codecpar->codec_id;
    return result;
}

const char* Reader::str_video_codec()
{
    const char* result = "unknown codec";
    if (video_stream_index >= 0) {
        result = avcodec_get_name(fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    }
    return result;
}

int64_t Reader::video_bit_rate()
{
    int64_t result = -1;
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->codecpar->bit_rate;
    return result;
}

AVRational Reader::video_time_base()
{
    AVRational result = av_make_q(0, 0);
    if (video_stream_index >= 0)
        result = fmt_ctx->streams[video_stream_index]->time_base;
    return result;
}

bool Reader::has_audio()
{
    return audio_stream_index >= 0;
}

int Reader::channels()
{
    int result = -1;
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->codecpar->channels;
    return result;
}

int Reader::sample_rate()
{
    int result = -1;
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->codecpar->sample_rate;
    return result;
}

int Reader::frame_size()
{
    int result = -1;
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->codecpar->frame_size;
    return result;
}

uint64_t Reader::channel_layout()
{
    uint64_t result = 0;
    if (audio_stream_index >= 0) 
        result = fmt_ctx->streams[audio_stream_index]->codecpar->channel_layout;
    return result;
}

std::string Reader::str_channel_layout()
{
    char result[256] = { 0 };
    if (audio_stream_index >= 0) {
        uint64_t cl = fmt_ctx->streams[audio_stream_index]->codecpar->channel_layout;
        av_get_channel_layout_string(result, 256, channels(), cl);
    }

    return std::string(result);
}

AVSampleFormat Reader::sample_format()
{
    AVSampleFormat result = AV_SAMPLE_FMT_NONE;
    if (audio_stream_index >= 0)
        result = (AVSampleFormat)fmt_ctx->streams[video_stream_index]->codecpar->format;
    return result;
}

const char* Reader::str_sample_format()
{
    const char* result = "unknown sample format";
    if (audio_stream_index >= 0) {
        const char* name = av_get_sample_fmt_name((AVSampleFormat)fmt_ctx->streams[audio_stream_index]->codecpar->format);
        if (name)
            result = name;
    }
    return result;
}

AVCodecID Reader::audio_codec()
{
    AVCodecID result = AV_CODEC_ID_NONE;
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->codecpar->codec_id;
    return result;
}

const char* Reader::str_audio_codec()
{
    const char* result = "unknown codec";
    if (audio_stream_index >= 0) {
        result = avcodec_get_name(fmt_ctx->streams[audio_stream_index]->codecpar->codec_id);
    }
    return result;
}

int64_t Reader::audio_bit_rate()
{
    int64_t result = -1;
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->codecpar->bit_rate;
    return result;
}

AVRational Reader::audio_time_base()
{
    AVRational result = av_make_q(0, 0);
    if (audio_stream_index >= 0)
        result = fmt_ctx->streams[audio_stream_index]->time_base;
    return result;
}

void Reader::showStreamParameters()
{
    std::stringstream str;
    str << "\n";
    if (has_video()) {
        str << "\nVideo Stream Parameters"
            << "\n  Video Codec:  " << str_video_codec()
            << "\n  Pixel Format: " << str_pix_fmt()
            << "\n  Resolution:   " << width() << " x " << height()
            << "\n  Frame Rate:   " << av_q2d(frame_rate());
    }
    else {
        str << "\nNo Video Stream Found";
    }
    if (has_audio()) {
        str << "\nAudio Stream Parameters"
            << "\n  Audio Codec:   " << str_audio_codec()
            << "\n  Sample Format: " << str_sample_format()
            << "\n  Channels:      " << str_channel_layout()
            << "\n  Sample Rate:   " << sample_rate();
    }
    else {
        str << "\nNo Audio Stream Found";
    }
    
    if (infoCallback) infoCallback(str.str());
}

}
