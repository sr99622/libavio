/********************************************************************
* libavio/include/Filter.hpp
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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <iostream>
#include <sstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
}

#include "Decoder.hpp"
#include "Frame.hpp"
#include "Queue.hpp"
#include "Exception.hpp"

namespace avio {

class Filter {
public:
	Decoder* decoder = nullptr;
    Queue<Frame>* input = nullptr;
    Queue<Frame>* output = nullptr;
	AVFilterContext* sink_ctx = nullptr;
	AVFilterContext* src_ctx = nullptr;
	AVFilterGraph* graph = nullptr;
	AVFrame* av_frame = nullptr;
	std::string description;
    ExceptionChecker ex;

    Filter(Decoder* decoder, const std::string& description, Queue<Frame>* input, Queue<Frame>* output) 
            : decoder(decoder), description(description), input(input), output(output) {

        const AVFilter* buf_src = avfilter_get_by_name(source_name(decoder->media_type).c_str());
        const AVFilter* buf_sink = avfilter_get_by_name(sink_name(decoder->media_type).c_str());
        AVFilterInOut* outputs = avfilter_inout_alloc();
        AVFilterInOut* inputs = avfilter_inout_alloc();
        try {
            if (!buf_src || !buf_sink || !outputs || !inputs) throw std::runtime_error("buffer allocation failure");

            ex.ck(av_frame = av_frame_alloc(), AFA);
            ex.ck(graph = avfilter_graph_alloc(), AGA);
            ex.ck(avfilter_graph_create_filter(&src_ctx, buf_src, "in", get_input_config(decoder).c_str(), nullptr, graph), AGCF);
            ex.ck(avfilter_graph_create_filter(&sink_ctx, buf_sink, "out", nullptr, nullptr, graph), AGCF);
            
            if (description.length()) {
                outputs->name = av_strdup("in");
                outputs->filter_ctx = src_ctx;
                outputs->pad_idx = 0;
                outputs->next = nullptr;

                inputs->name = av_strdup("out");
                inputs->filter_ctx = sink_ctx;
                inputs->pad_idx = 0;
                inputs->next = nullptr;

                ex.ck(avfilter_graph_parse_ptr(graph, description.c_str(), &inputs, &outputs, nullptr), AGPP);
            }
            else {
                ex.ck(avfilter_link(src_ctx, 0, sink_ctx, 0), AL);
            }
            ex.ck(avfilter_graph_config(graph, nullptr), AGC);
        }
        catch (const std::exception& e) {
            std::stringstream str;
            str << decoder->str_media_type << " filter constructor exception: " << e.what();
            throw std::runtime_error(str.str());
        }
        if (outputs) avfilter_inout_free(&outputs);
        if (inputs) avfilter_inout_free(&inputs);
    }

    ~Filter() {
        if (av_frame) av_frame_free(&av_frame);
        if (sink_ctx) avfilter_free(sink_ctx);
        if (src_ctx)  avfilter_free(src_ctx);
        if (graph)    avfilter_graph_free(&graph);
    }

    int filter() {
        Frame f = input->pop();

        if (decoder->reader->terminated) {
            output->clear();
            output->push(Frame(nullptr));
            return 0;
        }

        if (f.is_null()) {
            output->push(Frame(nullptr));
            return 0; 
        }

        if (decoder->reader->seek_pts != AV_NOPTS_VALUE)
            return 1;

        try {
            ex.ck(av_buffersrc_add_frame_flags(src_ctx, f.frame, AV_BUFFERSRC_FLAG_KEEP_REF), ABAFF);

            int ret = -1;
            while ((ret = av_buffersink_get_frame(sink_ctx, av_frame)) >= 0) {
                output->push(Frame(av_frame));
            }
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                ex.ck(ret, "error during filtering");
        }
        catch (const std::exception& e) {
            std::stringstream str;
            str << decoder->str_media_type << " filter exception: " << e.what();
            std::cout << str.str() << std::endl;
        }

        return 1;
    }

    std::string get_input_config(Decoder* decoder) const {
        char args[512] = {0};
        AVRational time_base = decoder->reader->fmt_ctx->streams[decoder->stream_index]->time_base;
        
        if (decoder->media_type == AVMEDIA_TYPE_VIDEO) {
            snprintf(args, sizeof(args),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                decoder->codec_ctx->width, decoder->codec_ctx->height, decoder->codec_ctx->pix_fmt,
                time_base.num, time_base.den,
                decoder->codec_ctx->sample_aspect_ratio.num, decoder->codec_ctx->sample_aspect_ratio.den);
        }
        else if (decoder->media_type == AVMEDIA_TYPE_AUDIO) {
            if (decoder->codec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
                av_channel_layout_default(&decoder->codec_ctx->ch_layout, decoder->codec_ctx->ch_layout.nb_channels);
            int ret = 0;
            ret = snprintf(args, sizeof(args),
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",
                time_base.num, time_base.den, decoder->codec_ctx->sample_rate,
                av_get_sample_fmt_name(decoder->codec_ctx->sample_fmt));
            av_channel_layout_describe(&decoder->codec_ctx->ch_layout, args + ret, sizeof(args) - ret);
        }
        else {
            throw std::runtime_error("get_input_config error: unknown media type");
        }
        return std::string(args);
    }

    std::string source_name(AVMediaType media_type) const {
        if (media_type == AVMEDIA_TYPE_VIDEO)
            return "buffer";
        else if (media_type == AVMEDIA_TYPE_AUDIO)
            return "abuffer";
        else
            throw std::runtime_error("source_name error: unknown media type");
    }

    std::string sink_name(AVMediaType media_type) const {
        if (media_type == AVMEDIA_TYPE_VIDEO)
            return "buffersink";
        else if (media_type == AVMEDIA_TYPE_AUDIO)
            return "abuffersink";
        else
            throw std::runtime_error("sink_name error: unknown media type");
    }
};

}

#endif // FILTER_HPP