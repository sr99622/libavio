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
#include "Player.h"
#include "Decoder.h"
#include "avio.h"

#define MAX_TIMEOUT 5

static int interrupt_callback(void *ctx)
{
    avio::Reader* reader = (avio::Reader*)ctx;
    time_t diff = time(nullptr) - reader->timeout_start;
    if (diff > MAX_TIMEOUT) {
        reader->timeout_msg = "camera timeout";
        return 1;
    }
    return 0;
}

namespace avio {

Reader::Reader(const char* filename, void* player) : player(player)
{
    try {
        AVDictionary* opts = nullptr;
        int timeout_us = MAX_TIMEOUT * 1000000;

        if (LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(60, 0, 0)) {
            av_dict_set_int(&opts, "timeout", timeout_us, 0);
        }
        else {
            if (LIBAVFORMAT_VERSION_INT > AV_VERSION_INT(59, 0, 0))
                av_dict_set(&opts, "timeout", "5000000", 0);
            else 
                av_dict_set(&opts, "stimeout", "5000000", 0);
        }

        ex.ck(avformat_open_input(&fmt_ctx, filename, nullptr, &opts), CmdTag::AOI);

        av_dict_free(&opts);
        timeout_start = time(nullptr);
        AVIOInterruptCB cb = { interrupt_callback, this };
        fmt_ctx->interrupt_callback = cb;

        ex.ck(avformat_find_stream_info(fmt_ctx, nullptr), CmdTag::AFSI);

        video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audio_stream_index < 0 && player) {
            if (P->infoCallback) P->infoCallback("NO AUDIO STREAM FOUND", P->uri);
        }
    }
    catch (const Exception& e) {
        if (P) P->stopped = true;
        std::string msg(e.what());
        std::string mark("-138");
        if (msg.find(mark) != std::string::npos)
            msg = "No connection to server";
        std::stringstream str;
        str << "Reader constructor exception: " << msg;
        throw std::runtime_error(str.str());
    }
}

Reader::~Reader()
{
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
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
        if (P->infoCallback) P->infoCallback(str.str(), P->uri);
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
    seek_found_pts = seek_target_pts;
    //clear_pkts_cache(0);
    //P->clear_queues();
    //seek();
}

bool Reader::seeking() 
{
    return seek_target_pts != AV_NOPTS_VALUE || seek_found_pts != AV_NOPTS_VALUE;
}

void Reader::close_pipe()
{
    if (pipe) pipe->close();
    pipe = nullptr;
}

Pipe* Reader::createPipe()
{
    Pipe* pipe = nullptr;
    try {
        AudioEncoding audio_encoding = AudioEncoding::NONE;
        if (!strcmp(str_audio_codec(), "aac")) audio_encoding = AudioEncoding::AAC;
        if (!strcmp(str_audio_codec(), "pcm_mulaw") || !strcmp(str_audio_codec(), "pcm_alaw")) audio_encoding = AudioEncoding::G711;
        //if (!strcmp(str_audio_codec(), "adpcm_g726le")) audio_encoding = AudioEncoding::G726;
        // G726 IS NOT SUPPORTED

        if (audio_encoding == AudioEncoding::NONE && !P->disable_audio) {
            std::stringstream str;
            str << "Audio format incompatible: (" << str_audio_codec() << ") only video packets will be written to file";
            if (P->infoCallback) P->infoCallback(str.str(), P->uri);
            P->disable_audio = true;
        }

        pipe = new Pipe(fmt_ctx, video_stream_index, P->disable_audio || (audio_encoding == AudioEncoding::NONE) ? -1 : audio_stream_index);

        pipe->infoCallback = P->infoCallback;
        pipe->errorCallback = P->errorCallback;
        pipe->onvif_frame_rate = P->onvif_frame_rate;
        pipe->open(pipe_out_filename, P->metadata);
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
        str << "Output file creation failure: " << e.what();
        if (P->infoCallback) P->infoCallback(str.str(), P->uri);
        else std::cout << str.str() << std::endl;
    }
    return pipe;
}

void Reader::pipe_write(AVPacket* pkt)
{
    if (!pipe) {
        pipe = createPipe();
        pipe_bytes_written = 0;

        if (pipe) {
            while (pkts_cache.size() > 0) {
                Packet tmp(pkts_cache.front());
                pkts_cache.pop_front();
                pipe_bytes_written += tmp.m_pkt->size;
                pipe->write(tmp.m_pkt);
            }
        }
    }

    if (pipe) {
        Packet tmp(av_packet_clone(pkt));
        pipe->write(tmp.m_pkt);
    }
}

void Reader::fill_pkts_cache(AVPacket* pkt)
{
    int fps = 0;
    int number_of_frames_to_buffer = 0;
    bool keyframe = false;
    if (pkt->stream_index == video_stream_index) {
        if (pkt->flags) {
            keyframe = true;

            // frame rate is often inaccurate in stream, prefer onvif
            fps = (int)av_q2d(P->onvif_frame_rate);
            if (!fps) fps = (int)av_q2d(frame_rate());
            if (!fps) fps = 30;
            number_of_frames_to_buffer = fps * P->buffer_size_in_seconds;

            if (number_of_frames_to_buffer > 0) {
                if (number_of_frames_to_buffer < num_video_pkts_in_cache() -1) {
                    int video_frame_count = 0;
                    bool trimming = false;
                    int idx = pkts_cache.size();
                    while (idx > 0) {
                        idx--;
                        AVPacket* tmp = pkts_cache[idx];

                        if (trimming) {
                            if (tmp->flags) {
                                break;
                            }
                        }

                        if (tmp->stream_index == video_stream_index) {
                            video_frame_count++;
                            if (video_frame_count == number_of_frames_to_buffer)
                                trimming = true;
                        }
                    }

                    for (int j = 0; j < idx; j++) {
                        AVPacket* jnk = pkts_cache.front();
                        pkts_cache.pop_front();
                        av_packet_free(&jnk);
                    }
                }
            }
        }
    }

    AVPacket* tmp = av_packet_clone(pkt);
    // for some reason, flags is corrupted by this point on Dahua style
    tmp->flags = 0;
    if (keyframe) {
        tmp->flags = 1;
        keyframe = false;
    }
    pkts_cache.push_back(tmp);

    // backstop to prevent unbounded cache growth
    while (pkts_cache.size() > 200 * P->buffer_size_in_seconds) {
        AVPacket* jnk = pkts_cache.front();
        pkts_cache.pop_front();
        av_packet_free(&jnk);
    }

}

int Reader::num_video_pkts_in_cache()
{
    int result = 0;
    for (int i = 0; i < pkts_cache.size(); i++) {
        if (pkts_cache[i]->stream_index == video_stream_index)
            result++;
    }
    return result;
}

void Reader::clear_pkts_cache(int mark)
{
    while (pkts_cache.size() > mark) {
        AVPacket* jnk = pkts_cache.front();
        pkts_cache.pop_front();
        av_packet_free(&jnk);
    }
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
#if LIBAVCODEC_VERSION_MAJOR < 61
        result = fmt_ctx->streams[audio_stream_index]->codecpar->channels;
#else
        result = fmt_ctx->streams[audio_stream_index]->codecpar->ch_layout.nb_channels;
#endif
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

#if LIBAVCODEC_VERSION_MAJOR < 61
uint64_t Reader::channel_layout()
{
    uint64_t result = 0;
    if (audio_stream_index >= 0) 
        result = fmt_ctx->streams[audio_stream_index]->codecpar->channel_layout;
    return result;
}
#endif

std::string Reader::str_channel_layout()
{
    char result[256] = { 0 };
    if (audio_stream_index >= 0) {

#if LIBAVCODEC_VERSION_MAJOR < 61
        uint64_t cl = fmt_ctx->streams[audio_stream_index]->codecpar->channel_layout;
        av_get_channel_layout_string(result, 256, channels(), cl);
#else
        AVChannelLayout cl = fmt_ctx->streams[audio_stream_index]->codecpar->ch_layout;
        av_channel_layout_describe(&cl, result, 256);
#endif

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

const char* Reader::metadata(const std::string& key)
{
    const char* result = "";
    AVDictionaryEntry* tag = nullptr;
    tag = av_dict_get(fmt_ctx->metadata, (const char*)key.c_str(), tag, AV_DICT_IGNORE_SUFFIX);
    if (tag) {
        result = tag->value;
    }
    return result;
}

std::string Reader::getStreamInfo()
{
    std::stringstream str;
    if (has_video()) {
        if (P->disable_video) {
            str << "<center><h4>Video stream is disabled</h4></center>";
        }
        else {
            str << "\n"
                << "<table>"
                << "<th colspan='2'>Video Stream Parameters</th>"
                << "<tr><td>Video Codec:</td><td>" << str_video_codec() << "</td></tr>"
                << "<tr><td>Pixel Format:</td><td>" << str_pix_fmt() << "</td></tr>"
                << "<tr><td>Resolution:</td><td>" << width() << " x " << height() << "</td></tr>"
                << "<tr><td>Frame Rate:</td><td>" << av_q2d(frame_rate()) << "</td></tr>"
                << "</table>";
        }
    }
    else {
        str << "<center><h4>No Video Stream Found</h4></center>";
    }
    if (has_audio()) {
        if (P->disable_audio) {
            str << "<center><h4>Audio stream is disabled</h4></center>";
        }
        else {
            str << "\n"
                << "<table>"
                << "<th colspan='2'>Audio Stream Parameters</th>"
                << "<tr><td>Audio Codec:</td><td>" << str_audio_codec() << "</td></tr>"
                << "<tr><td>Sample Format:</td><td>" << str_sample_format() << "</td></tr>"
                << "<tr><td>Channels:</td><td>" << str_channel_layout() << "</td></tr>"
                << "<tr><td>Sample Rate:</td><td>" << sample_rate() << "</td></tr>"
                << "</table>";
        }
    }
    else {
        str << "<center><h4>No Audio Stream Found</h4></center>";
    }
    return str.str();
}

}
