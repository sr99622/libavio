/********************************************************************
* libavio/src/Pipe.cpp
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

#include "Pipe.h"
#include "Player.h"

namespace avio
{

Pipe::Pipe(AVFormatContext* reader_fmt_ctx, int video_stream_index, int audio_stream_index) : 
    reader_fmt_ctx(reader_fmt_ctx),
    video_stream_index(video_stream_index), 
    audio_stream_index(audio_stream_index)
{

}

Pipe::~Pipe()
{
    close();
}

AVCodecContext* Pipe::getContext(AVMediaType mediaType)
{
    AVCodecContext* enc_ctx = nullptr;
    std::string strMediaType = "unknown";

    int stream_index = -1;

    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        strMediaType = "video";
        stream_index = video_stream_index;
    }
    else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        strMediaType = "audio";
        stream_index = audio_stream_index;
    }

    if (stream_index < 0) return nullptr;
    AVStream* stream = reader_fmt_ctx->streams[stream_index];
    const AVCodec* enc = avcodec_find_encoder(stream->codecpar->codec_id);
    if (!enc) throw Exception("could not find encoder");
    ex.ck(enc_ctx = avcodec_alloc_context3(enc), AAC3);
    ex.ck(avcodec_parameters_to_context(enc_ctx, stream->codecpar), APTC);

    return enc_ctx;
}

void Pipe::open(const std::string& filename)
{
    opened = false;
    ex.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename.c_str()), AAOC2);

    video_ctx = getContext(AVMEDIA_TYPE_VIDEO);
    if (video_ctx) {
        ex.ck(video_stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        if (video_ctx == NULL) throw Exception("no video reference context");
        ex.ck(avcodec_parameters_from_context(video_stream->codecpar, video_ctx), APFC);
        video_stream->time_base = reader_fmt_ctx->streams[video_stream_index]->time_base;
    }

    audio_ctx = getContext(AVMEDIA_TYPE_AUDIO);
    if (audio_ctx) {
        ex.ck(audio_stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        if (audio_ctx == NULL) throw Exception("no audio reference context");
        ex.ck(avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx), APFC);
        audio_stream->time_base = reader_fmt_ctx->streams[audio_stream_index]->time_base;
    }

    ex.ck(avio_open(&fmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE), AO);
    opened = true;
    ex.ck(avformat_write_header(fmt_ctx, NULL), AWH);

    video_next_pts = 0;
    audio_next_pts = 0;

    std::stringstream str;
    str << "Pipe opened write file: " << filename.c_str();
    if (infoCallback) infoCallback(str.str());
    else std::cout << str.str() << std::endl;
}

void Pipe::adjust_pts(AVPacket* pkt)
{
    if (pkt->stream_index == video_stream_index) {
        pkt->stream_index = video_stream->index;
        pkt->dts = pkt->pts = video_next_pts;
        video_next_pts += pkt->duration;
    }
    else if (pkt->stream_index == audio_stream_index) {
        pkt->stream_index = audio_stream->index;
        pkt->dts = pkt->pts = audio_next_pts;
        audio_next_pts += pkt->duration;
    }
}

void Pipe::write(AVPacket* pkt)
{
    adjust_pts(pkt);

    try {
        ex.ck(av_interleaved_write_frame(fmt_ctx, pkt), AIWF);
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Pipe write packet exception: " << e.what();
        if (infoCallback) infoCallback(str.str());
        else std::cout << str.str() << std::endl;
    }
}

void Pipe::close()
{
    try {

        std::string url(fmt_ctx->url);

        if (video_ctx) {
            avcodec_free_context(&video_ctx);
            video_ctx = nullptr;
        }
    
        if (audio_ctx) {
            avcodec_free_context(&audio_ctx);
            audio_ctx = nullptr;
        }

        if (fmt_ctx) {
            ex.ck(av_write_trailer(fmt_ctx), AWT);
            ex.ck(avio_closep(&fmt_ctx->pb), ACP);
            avformat_free_context(fmt_ctx);
            fmt_ctx = nullptr;
        }

        std::stringstream str;
        str << "Pipe closed file: " << url;
        if (infoCallback) infoCallback(str.str());
        else std::cout << str.str() << std::endl;
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Record to file close error: " << e.what();
        if (errorCallback) errorCallback(str.str());
        else std::cout << str.str() << std::endl;
    }
}

}
