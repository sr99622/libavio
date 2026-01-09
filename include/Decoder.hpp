/********************************************************************
* libavio/include/Decoder.hpp
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

#ifndef DECODER_HPP
#define DECODER_HPP

#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "Reader.hpp"
#include "Writer.hpp"
#include "Queue.hpp"
#include "Packet.hpp"
#include "Frame.hpp"

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
    const AVPixelFormat* p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == hw_pix_fmt) {
            return *p;
        }
    }

    throw std::runtime_error("Failed to get HW surface format");
}

namespace avio {

class Decoder {
public:
    AVCodecContext* codec_ctx = nullptr;
    int stream_index = -1;
    const AVCodec* decoder = nullptr;
    Queue<Packet>* pkts = nullptr;
    Queue<Frame>* frames = nullptr;
    Queue<Packet>* writer_pkts = nullptr;
    Reader* reader = nullptr;
    AVFrame* av_frame = nullptr;
    AVFrame* sw_frame = nullptr;
    ExceptionChecker ex;
    AVMediaType media_type;
    std::string str_media_type;
    AVHWDeviceType hw_type;
    AVBufferRef* hw_device_ctx = nullptr;

    Decoder(Reader* reader, AVMediaType media_type, Queue<Packet>* pkts, Queue<Frame>* frames, AVHWDeviceType hw_type=AV_HWDEVICE_TYPE_NONE) 
            : reader(reader), media_type(media_type), pkts(pkts), frames(frames), hw_type(hw_type) {

        const char* str = av_get_media_type_string(media_type);
        str_media_type = (str ? str : "unknown media type");
        ex.ck((stream_index = av_find_best_stream(reader->fmt_ctx, media_type, -1, -1, &decoder, 0)), AFBS);
        AVStream* stream = reader->fmt_ctx->streams[stream_index];

        if (hw_type != AV_HWDEVICE_TYPE_NONE) {
            for (int i=0;; i++) {
                const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
                if (!config) {
                    std::stringstream str;
                    str << str_media_type << " decoder " << decoder->name << " does not support device type " << av_hwdevice_get_type_name(hw_type);
                    throw std::runtime_error(str.str());
                }
                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == hw_type) {
                    hw_pix_fmt = config->pix_fmt;
                    break;
                }
            }
        }
        
        ex.ck((codec_ctx = avcodec_alloc_context3(decoder)), AAC3);
        ex.ck(avcodec_parameters_to_context(codec_ctx, stream->codecpar), APTC);

        if (hw_type != AV_HWDEVICE_TYPE_NONE) {
            codec_ctx->get_format = get_hw_format;
            ex.ck(av_hwdevice_ctx_create(&hw_device_ctx, hw_type, nullptr, nullptr, 0), "hardware decoder initialization error");
            codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
            ex.ck(sw_frame = av_frame_alloc(), AFA);
        }

        ex.ck(avcodec_open2(codec_ctx, decoder, nullptr), AO2);
        ex.ck(av_frame = av_frame_alloc(), AFA);
    }

    ~Decoder() {
        if (av_frame) av_frame_free(&av_frame);
        if (sw_frame) av_frame_free(&sw_frame);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
    }

    int decode() {
        Packet pkt = pkts->pop();

        if (reader->terminated) {
            frames->clear();
            frames->push(Frame(nullptr));
            if (writer_pkts) writer_pkts->push(Packet(nullptr));
            return 0;
        }

        if (!pkt.is_null() && pkt.pts() == AV_NOPTS_VALUE) {
            if (pkt.pkt->data) {
                if (!strcmp((const char*)pkt.pkt->data, "FLUSH"))
                    avcodec_flush_buffers(codec_ctx);
            }
            return 1;
        }

        if (reader->seek_pts != AV_NOPTS_VALUE) 
            return 1;

        try {
            int ret = -1;
            ex.ck((ret = avcodec_send_packet(codec_ctx, pkt.pkt)), ASP);
            while ((ret = avcodec_receive_frame(codec_ctx, av_frame)) >= 0) {
                if (av_frame->format == hw_pix_fmt) {
                    ex.ck(av_hwframe_transfer_data(sw_frame, av_frame, 0), AHTD);
                	ex.ck(av_frame_copy_props(sw_frame, av_frame), AFCP);
                    frames->push(Frame(sw_frame));
                    Frame term(av_frame);
                }
                else {
                    frames->push(Frame(av_frame));
                }
            }
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                ex.ck(ret, "error during decoding");
        }
        catch (const std::exception& e) {
            std::stringstream str;
            str << str_media_type << " decode exception: " << e.what() << std::endl;
            std::cout << str.str() << std::endl;
        }

        if (pkt.is_null()) {
            frames->push(Frame(nullptr));
            if (writer_pkts) writer_pkts->push(std::move(pkt));
            return 0;
        }

        if (writer_pkts) {
            if (writer_pkts) writer_pkts->push(std::move(pkt));
        }

        return 1;
    }
};

}

#endif // DECODER_HPP