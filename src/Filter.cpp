/********************************************************************
* libavio/src/Filter.cpp
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

#include <SDL.h>
#include "Filter.h"
#include "Player.h"

namespace avio
{

Filter::Filter(Decoder& decoder, const char* description) : decoder(&decoder), desc(description)
{
    switch (decoder.mediaType) {
    case AVMEDIA_TYPE_VIDEO:
        initVideo();
        break;
    case AVMEDIA_TYPE_AUDIO:
        initAudio();
        break;
    default:
        std::cout << "Filter constructor error, unknown media type" << std::endl;
        break;
    }
}

void Filter::initVideo()
{
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();

    char args[512];
    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        decoder->dec_ctx->width, decoder->dec_ctx->height, decoder->dec_ctx->pix_fmt,
        decoder->stream->time_base.num, decoder->stream->time_base.den,
        decoder->dec_ctx->sample_aspect_ratio.num, decoder->dec_ctx->sample_aspect_ratio.den);

    try {
        ex.ck(frame = av_frame_alloc(), AFA);
        ex.ck(graph = avfilter_graph_alloc(), AGA);
        ex.ck(avfilter_graph_create_filter(&src_ctx, buffersrc, "in", args, nullptr, graph), AGCF);
        ex.ck(avfilter_graph_create_filter(&sink_ctx, buffersink, "out", nullptr, nullptr, graph), AGCF);

        outputs->name = av_strdup("in");
        outputs->filter_ctx = src_ctx;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        ex.ck(avfilter_graph_parse_ptr(graph, desc.c_str(), &inputs, &outputs, nullptr), AGPP);
        ex.ck(avfilter_graph_config(graph, nullptr), AGC);

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    }
    catch (const Exception& e) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        std::stringstream str;
        str << "Video filter constructor exception: " << e.what();
        throw Exception(str.str());
    }
}

void Filter::initAudio()
{
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    const AVFilter* buf_src = avfilter_get_by_name("abuffer");
    const AVFilter* buf_sink = avfilter_get_by_name("abuffersink");

    //static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_U8, AV_SAMPLE_FMT_NONE };
    //static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_NONE };

    try {
        if (decoder->dec_ctx->channel_layout && av_get_channel_layout_nb_channels(decoder->dec_ctx->channel_layout) == decoder->dec_ctx->channels)
            m_channel_layout = decoder->dec_ctx->channel_layout;

        std::stringstream str;
        str << "sample_rate=" << decoder->dec_ctx->sample_rate << ":"
            << "sample_fmt=" << av_get_sample_fmt_name(decoder->dec_ctx->sample_fmt) << ":"
            << "channels=" << decoder->dec_ctx->channels << ":"
            << "time_base=" << decoder->stream->time_base.num << "/" << decoder->stream->time_base.den;

        if (m_channel_layout)
            str << ":channel_layout=0x" << std::hex << m_channel_layout;

        ex.ck(frame = av_frame_alloc(), AFA);
        ex.ck(graph = avfilter_graph_alloc(), AGA);
        ex.ck(avfilter_graph_create_filter(&src_ctx, buf_src, "buf_src", str.str().c_str(), nullptr, graph), AGCF);
        ex.ck(avfilter_graph_create_filter(&sink_ctx, buf_sink, "buf_sink", nullptr, nullptr, graph), AGCF);
        ex.ck(av_opt_set_int_list(sink_ctx, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN), AOSIL);
        ex.ck(av_opt_set_int(sink_ctx, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN), AOSI);

        if (desc.c_str()) {
            if (!outputs || !inputs) throw Exception("avfilter_inout_alloc");

            outputs->name = av_strdup("in");
            outputs->filter_ctx = src_ctx;
            outputs->pad_idx = 0;
            outputs->next = nullptr;

            inputs->name = av_strdup("out");
            inputs->filter_ctx = sink_ctx;
            inputs->pad_idx = 0;
            inputs->next = nullptr;

            ex.ck(avfilter_graph_parse_ptr(graph, desc.c_str(), &inputs, &outputs, nullptr), AGPP);
        }
        else {
            ex.ck(avfilter_link(src_ctx, 0, sink_ctx, 0), AL);
        }

        ex.ck(avfilter_graph_config(graph, nullptr), AGC);

        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
    }
    catch (const Exception& e) {
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        std::stringstream str;
        str << "Audio filter constructor exception: " << e.what();
        throw Exception(str.str());
    }

}

Filter::~Filter()
{
    if (sink_ctx) avfilter_free(sink_ctx);
    if (src_ctx) avfilter_free(src_ctx);
    if (graph) avfilter_graph_free(&graph);
    if (frame) av_frame_free(&frame);
}

void Filter::filter(Frame& f)
{
    int ret = 0;

    try {
        ex.ck(av_buffersrc_add_frame_flags(src_ctx, f.m_frame, AV_BUFFERSRC_FLAG_KEEP_REF), ABAFF);
        while (true) {
            ret = av_buffersink_get_frame(sink_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                throw Exception("av_buffersink_get_frame");

            f.invalidate();
            f = Frame(frame);
            f.m_rts = f.pts() * 1000 * av_q2d(av_buffersink_get_time_base(sink_ctx));
            if (show_frames) std::cout << "filter " << f.description() << std::endl;
            frame_out_q->push_move(f);
        }
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Filter exception: " << e.what();
        if (infoCallback) infoCallback(str.str());
        else std::cout << str.str() << std::endl;
    }
}

}
