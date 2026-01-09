/********************************************************************
* libavio/include/Writer.hpp
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

#ifndef PIPE_HPP
#define PIPE_HPP

#include <map>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "Exception.hpp"
#include "Packet.hpp"
#include "Reader.hpp"
#include "Queue.hpp"

namespace avio {

class Writer {
public:
    Reader* reader;
    std::string filename;
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_ctx = nullptr;
    AVCodecContext* audio_ctx = nullptr;
    AVStream* video_stream = nullptr;
    AVStream* audio_stream = nullptr;
    int64_t video_next_pts;
    int64_t audio_next_pts;
    Queue<Packet>* input = nullptr;
    Queue<Packet> video_cache;
    Queue<Packet> audio_cache;
    bool disable_video = false;
    bool disable_audio = false;
    //std::map<std::string, std::string> metadata;
    ExceptionChecker ex;

    Writer(Reader* reader) : reader(reader) { }

    ~Writer() {
        close();
    }

    void open(const std::string& base_filename /*, std::map<std::string, std::string>& metadata*/) {
        std::string extension = ".mp4";
        if (reader->has_audio() && !disable_audio) {
            if      (reader->audio_codec() == AV_CODEC_ID_PCM_MULAW)  extension = ".mov";
            else if (reader->audio_codec() == AV_CODEC_ID_PCM_ALAW)   extension = ".mov";
            else if (reader->audio_codec() == AV_CODEC_ID_AAC)        extension = ".mp4";
            else {
                extension = ".mp4";
                disable_audio = true;
                std::stringstream str;
                str << "audio codec " << reader->str_audio_codec() << " is not supported, audio recording is disabled" << std::endl;
                std::cout << str.str() << std::endl;
            }
        }
        filename = base_filename + extension;

        ex.ck(avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename.c_str()), AAOC2);
        if (reader->video_stream_index >= 0 && !disable_video) {
            AVStream* stream = reader->fmt_ctx->streams[reader->video_stream_index];
            const AVCodec* encoder = avcodec_find_encoder(stream->codecpar->codec_id);
            if (!encoder) throw std::runtime_error("writer constructor could not find encoder for video stream");
            ex.ck(video_ctx = avcodec_alloc_context3(encoder), AAC3);
            ex.ck(avcodec_parameters_to_context(video_ctx, stream->codecpar), APTC);
            ex.ck(video_stream = avformat_new_stream(fmt_ctx, nullptr), ANS);
            ex.ck(avcodec_parameters_from_context(video_stream->codecpar, video_ctx), APFC);
            video_stream->time_base = reader->fmt_ctx->streams[reader->video_stream_index]->time_base;
        }
        if (reader->audio_stream_index >= 0 && !disable_audio) {
            AVStream* stream = reader->fmt_ctx->streams[reader->audio_stream_index];
            const AVCodec* encoder = avcodec_find_encoder(stream->codecpar->codec_id);
            if (!encoder) throw std::runtime_error("writer constructor could not find encoder for audio stream");
            ex.ck(audio_ctx = avcodec_alloc_context3(encoder), AAC3);
            ex.ck(avcodec_parameters_to_context(audio_ctx, stream->codecpar), APTC);
            ex.ck(audio_stream = avformat_new_stream(fmt_ctx, nullptr), ANS);
            ex.ck(avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx), APFC);
            audio_stream->time_base = reader->fmt_ctx->streams[reader->audio_stream_index]->time_base;
        }


        ex.ck(avio_open(&fmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE), AO);
        /*
        std::map<std::string, std::string>::iterator it;
        for(it = metadata.begin(); it != metadata.end(); ++it)
            av_dict_set(&fmt_ctx->metadata, (const char*)it->first.c_str(), (const char*)it->second.c_str(), 0);
        AVDictionary* options = nullptr;
        av_dict_set(&options, "movflags", "use_metadata_tags", 0);
        ex.ck(avformat_write_header(fmt_ctx, &options), AWH);
        */
        ex.ck(avformat_write_header(fmt_ctx, nullptr), AWH);

        video_next_pts = 0;
        audio_next_pts = 0;
    }

    void adjust_pts(AVPacket* pkt) {
        if (pkt->stream_index == reader->video_stream_index) {
            pkt->stream_index = video_stream->index;
            pkt->pts = video_next_pts;
            pkt->dts = video_next_pts;
            video_next_pts += pkt->duration;
        }
        else if (pkt->stream_index == reader->audio_stream_index) {
            pkt->stream_index = audio_stream->index;
            pkt->pts = audio_next_pts;
            pkt->dts = audio_next_pts;
            audio_next_pts += pkt->duration;
        }
    }

    void write_packet(AVPacket* pkt) {
        if (!pkt) return;
        try {
            if (((pkt->stream_index == reader->video_stream_index) && !disable_video) || ((pkt->stream_index == reader->audio_stream_index) && !disable_audio)) {
                adjust_pts(pkt);
                ex.ck(av_interleaved_write_frame(fmt_ctx, pkt), AIWF);
            }
        }
        catch (const std::exception& e) {
            std::cout << "packet write exception: " << e.what() << std::endl;
        }
    }

    void write_cache() {
        size_t video_ptr = 0;
        size_t audio_ptr = 0;

        while (video_ptr < video_cache.size() && audio_ptr < audio_cache.size()) {
            int64_t audio_rt = reader->real_time(reader->audio_stream_index, audio_cache.at(audio_ptr)->pts());
            int64_t video_rt = reader->real_time(reader->video_stream_index, video_cache.at(video_ptr)->pts());
            if (video_rt > audio_rt && audio_rt != -1) {
                while (audio_ptr < audio_cache.size()) {
                    if (reader->real_time(reader->audio_stream_index, audio_cache.at(audio_ptr)->pts()) > video_rt)
                        break;
                    Packet tmp = *(audio_cache.at(audio_ptr));
                    write_packet(tmp.pkt);
                    audio_ptr++;
                }
            }
            else {
                if (video_ptr < video_cache.size()) {
                    Packet tmp = *(video_cache.at(video_ptr));
                    write_packet(tmp.pkt);
                    video_ptr++;
                }
                else {
                    while (audio_ptr < audio_cache.size()) {
                        Packet tmp = *(audio_cache.at(audio_ptr));
                        write_packet(tmp.pkt);
                        audio_ptr++;
                    }
                }
            }
        }

        if (video_cache.size() && !audio_cache.size()) {
            while (video_ptr < video_cache.size()) {
                Packet tmp = *(video_cache.at(video_ptr));
                write_packet(tmp.pkt);
                video_ptr++;
            }
        }

        if (!video_cache.size() && audio_cache.size()) {
            while (audio_ptr < audio_cache.size()) {
                Packet tmp = *(audio_cache.at(audio_ptr));
                write_packet(tmp.pkt);
                audio_ptr++;
            }
        }
    }

    int write() {
        Packet pkt = input->pop();
        //if (reader->recording && !reader->closed && !reader->terminated && !pkt.is_null()) {
        // there's an issue here with how the stream closes, either video or audio could send
        // a null packet first when using post decode mode
        if (reader->recording && !pkt.is_null()) {
            try {
                if (!fmt_ctx) {
                    open(filename);
                    write_cache();
                }
                Packet tmp = pkt;
                write_packet(tmp.pkt);
            }
            catch (const std::exception& e) {
                std::cout << "error writing to " << filename << ": " << e.what() << std::endl;
            }
        }
        else {
            if (fmt_ctx) close();
        }

        if (pkt.is_null()) {
            return 0;
        }

        push_cache_pkt(std::move(pkt));
        return 1;        
    }

    void push_cache_pkt(Packet&& pkt) {
        // The cache preserves recent packets so that when recording starts, the packets during a time interval prior to
        // start of recording are preserved. This insures that moments leading up to the alarm are recorded as well. For
        // continuously recording streams, this guarantees that there is some overlap during the transition between files.
        if (pkt.stream_index() == reader->video_stream_index) {
            // first pkt of video cache must always be keyframe, so only trim when a new keyframe enters the cache
            if (pkt.is_key_frame()) {
                int64_t stream_time = reader->real_time(reader->video_stream_index, pkt.pts());
                size_t search_index = video_cache.size() - 1;
                size_t key_frame_index = video_cache.find_last_key_frame(search_index);
                if (key_frame_index != SIZE_MAX) {
                    // search the cache for keyframes starting from the back until the trimmed cache duration is approx target
                    int64_t key_frame_time = reader->real_time(reader->video_stream_index, video_cache.at(key_frame_index)->pts());
                    int64_t cache_duration = stream_time - key_frame_time;
                    while ((cache_duration < reader->cache_size_in_seconds * 1000) && (key_frame_index > 0)) {
                        key_frame_index = video_cache.find_last_key_frame(search_index);
                        if (key_frame_index == SIZE_MAX)
                            break;
                        key_frame_time = reader->real_time(reader->video_stream_index, video_cache.at(key_frame_index)->pts());
                        cache_duration = stream_time - key_frame_time;
                        if (key_frame_index > 0)
                            search_index = key_frame_index - 1;
                    }
                    // match audio cache duration to video cache duration
                    if (reader->has_audio()) {
                        int64_t audio_pts = reader->pts_from_real_time(reader->audio_stream_index, key_frame_time);
                        size_t audio_index = audio_cache.find_pts(audio_pts);
                        if (audio_index != SIZE_MAX) {
                            audio_cache.erase_front(audio_index);
                        }
                    }
                    // trim video cache
                    if (key_frame_index != SIZE_MAX) {
                        video_cache.erase_front(key_frame_index);
                    }
                    // guarantee that the first video pkt in cache is a key frame
                    key_frame_index = video_cache.find_first_key_frame(0);
                    if (key_frame_index > 0 && key_frame_index != SIZE_MAX) {
                        if (reader->has_audio()) {
                            key_frame_time = reader->real_time(reader->video_stream_index, video_cache.at(key_frame_index)->pts());
                            int64_t audio_pts = reader->pts_from_real_time(reader->audio_stream_index, key_frame_time);
                            size_t audio_index = audio_cache.find_pts(audio_pts);
                            if (audio_index != SIZE_MAX)
                                audio_cache.erase_front(audio_index);
                        }
                        video_cache.erase_front(key_frame_index);
                    }
                }
            }
            video_cache.push(std::move(pkt));
        }
        else if (pkt.stream_index() == reader->audio_stream_index) {
            // only trim the audio cache here if there is no video cache
            if (!reader->has_video() && audio_cache.size()) {
                int64_t stream_time = reader->real_time(reader->audio_stream_index, pkt.pts());
                int64_t cache_start_time = reader->real_time(reader->audio_stream_index, audio_cache.at(0)->pts());
                while ((stream_time - cache_start_time) > reader->cache_size_in_seconds * 1000) {
                    audio_cache.erase_front(1);
                    cache_start_time = reader->real_time(reader->audio_stream_index, audio_cache.at(0)->pts());
                } 
            }
            audio_cache.push(std::move(pkt));
        }
    }

    void close() {
        if (video_ctx) {
            avcodec_free_context(&video_ctx);
            video_ctx = nullptr;
        }
        if (audio_ctx) {
            avcodec_free_context(&audio_ctx);
            audio_ctx = nullptr;
        }
        if (fmt_ctx) {
            try {
                avio_flush(fmt_ctx->pb);
                ex.ck(av_write_trailer(fmt_ctx), AWT);
                ex.ck(avio_closep(&fmt_ctx->pb), ACP);
                avformat_free_context(fmt_ctx);
                fmt_ctx = nullptr;
            }
            catch (const std::exception& e) {
                std::cout << "writer close exception: " << e.what() << std::endl;
            }
        }
    }
};

}

#endif // PIPE_HPP