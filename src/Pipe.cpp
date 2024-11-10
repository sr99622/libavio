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
#include "Decoder.h"
#include "Reader.h"

namespace avio
{

Pipe::Pipe(AVFormatContext* input_fmt_ctx, int video_stream_index, int audio_stream_index, void* decoder, void* encoder) : 
    input_fmt_ctx(input_fmt_ctx),
    video_stream_index(video_stream_index), 
    audio_stream_index(audio_stream_index),
    decoder(decoder),
    encoder(encoder)
{

}

Pipe::~Pipe()
{
    close();
    std::cout << "pipe destructor called" << std::endl;
}

AVCodecContext* Pipe::getContext(AVMediaType mediaType)
{
    AVCodecContext* enc_ctx = nullptr;

    if (mediaType == AVMEDIA_TYPE_VIDEO && video_stream_index > -1) {
        AVStream* stream = nullptr;
        stream = input_fmt_ctx->streams[video_stream_index];
        if (stream->avg_frame_rate.den)
            encoder_frame_rate = stream->avg_frame_rate;
        else
            encoder_frame_rate = stream->r_frame_rate;
        const AVCodec* enc = avcodec_find_encoder(stream->codecpar->codec_id);
        if (!enc) throw Exception("could not find encoder");
        ex.ck(enc_ctx = avcodec_alloc_context3(enc), AAC3);
        ex.ck(avcodec_parameters_to_context(enc_ctx, stream->codecpar), APTC);
    }

    if (mediaType == AVMEDIA_TYPE_AUDIO && audio_stream_index > -1) {
        AVStream* stream = nullptr;
        if (encoder)
            stream = ((Encoder*)encoder)->writer->fmt_ctx->streams[0];
        else
            stream = input_fmt_ctx->streams[audio_stream_index];

        const AVCodec* enc = avcodec_find_encoder(stream->codecpar->codec_id);
        if (!enc) throw Exception("could not find encoder");
        ex.ck(enc_ctx = avcodec_alloc_context3(enc), AAC3);
        ex.ck(avcodec_parameters_to_context(enc_ctx, stream->codecpar), APTC);
    }

    return enc_ctx;
}

void Pipe::open(const std::string& filename, std::map<std::string, std::string>& metadata)
{
    opened = false;
    ex.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename.c_str()), AAOC2);

    video_ctx = getContext(AVMEDIA_TYPE_VIDEO);
    if (video_ctx) {
        ex.ck(video_stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        if (video_ctx == NULL) throw Exception("no video reference context");
        ex.ck(avcodec_parameters_from_context(video_stream->codecpar, video_ctx), APFC);
        video_stream->time_base = input_fmt_ctx->streams[video_stream_index]->time_base;
    }

    audio_ctx = getContext(AVMEDIA_TYPE_AUDIO);
    if (audio_ctx) {
        ex.ck(audio_stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        if (audio_ctx == NULL) throw Exception("no audio reference context");
        ex.ck(avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx), APFC);
        audio_stream->time_base = input_fmt_ctx->streams[audio_stream_index]->time_base;
    }

    ex.ck(avio_open(&fmt_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE), AO);
    opened = true;
    m_filename = filename;

    std::map<std::string, std::string>::iterator it;
    for(it = metadata.begin(); it != metadata.end(); ++it)
        av_dict_set(&fmt_ctx->metadata, (const char*)it->first.c_str(), (const char*)it->second.c_str(), 0);

    AVDictionary* options = nullptr;
    av_dict_set(&options, "movflags", "use_metadata_tags", 0);
    ex.ck(avformat_write_header(fmt_ctx, &options), AWH);

    video_next_pts = 0;
    audio_next_pts = 0;

    std::stringstream str;
    str << "Pipe opened write file: " << filename.c_str();
    if (infoCallback) infoCallback(str.str(), filename);
    else std::cout << str.str() << std::endl;
}

void Pipe::adjust_pts(AVPacket* pkt)
{
    if (pkt->stream_index == video_stream_index) {
        pkt->stream_index = video_stream->index;
        double factor = 1;
        if (encoder_frame_rate.den && onvif_frame_rate.den && onvif_frame_rate.num)
            factor = av_q2d(av_div_q(encoder_frame_rate, onvif_frame_rate));
        pkt->pts = (int64_t)(video_next_pts * factor);
        pkt->dts = pkt->pts;
        video_next_pts += pkt->duration;
    }
    else if (pkt->stream_index == audio_stream_index) {
        pkt->stream_index = audio_stream->index;
        pkt->dts = pkt->pts = audio_next_pts;
        audio_next_pts += pkt->duration;
    }
}

void Pipe::transcode(AVPacket* pkt)
{
    try {
        ((Decoder*)decoder)->decode(pkt);
        while (((Decoder*)decoder)->frame_q->size() > 0) {
            Frame f;
            ((Decoder*)decoder)->frame_q->pop_move(f);
            encode(f);
        }
    }
    catch (const QueueClosedException& e) {  std::cout << "Pipe transcode closed queue exception 1" << std::endl; }
}

int Pipe::encode(Frame& f) {
    int result = ((Encoder*)encoder)->encode(f);
    while (((Encoder*)encoder)->pkt_q->size() > 0) {
        Packet p;
        try {
            ((Encoder*)encoder)->pkt_q->pop_move(p);
        }
        catch (const QueueClosedException& e) { std::cout << "Pipe encode closed queue exception 1" << std::endl; }
        p.m_pkt->stream_index = audio_stream_index;
        if (p.isValid()) {
            adjust_pts(p.m_pkt);
            try {
                ex.ck(av_interleaved_write_frame(fmt_ctx, p.m_pkt), AIWF);
            }
            catch (const Exception& e) {
                std::stringstream str;
                str << "Pipe write audio transcode packet exception: " << e.what();
                if (infoCallback) infoCallback(str.str(), m_filename);
                else std::cout << str.str() << std::endl;
            }
        }
    }
    return result;
}

void Pipe::write(AVPacket* pkt)
{
    std::string pkt_type = "Unkown Stream Type";
    if (pkt->stream_index == video_stream_index) pkt_type = "Video";
    if (pkt->stream_index == audio_stream_index) pkt_type = "Audio";
    
    if (pkt->stream_index == audio_stream_index && decoder && encoder) {
        transcode(pkt);
    }
    else {
        adjust_pts(pkt);

        try {
            if (pkt->stream_index == video_stream_index || pkt->stream_index == audio_stream_index) {
                ex.ck(av_interleaved_write_frame(fmt_ctx, pkt), AIWF);
            }
        }
        catch (const Exception& e) {
            std::stringstream str;
            str << "Pipe write " << pkt_type << " packet exception: " << e.what();
            if (infoCallback) infoCallback(str.str(), m_filename);
            else std::cout << str.str() << std::endl;
        }
    }
}

void Pipe::close()
{
    try {
        if (decoder && encoder) {
            transcode(NULL);
            Frame f(nullptr);
            encode(f);
        }

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

        if (decoder) delete (Decoder*)decoder;
        if (encoder) {
            delete ((Encoder*)encoder)->writer;
            delete (Encoder*)encoder;
        }

        std::stringstream str;
        str << "Pipe closed file: " << url;
        if (infoCallback) infoCallback(str.str(), m_filename);
        else std::cout << str.str() << std::endl;
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Record to file close error: " << e.what();
        if (infoCallback) infoCallback(str.str(), m_filename);
        else std::cout << str.str() << std::endl;
    }
}

}
