/********************************************************************
* libavio/src/Encoder.cpp
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

#include "Encoder.h"

namespace avio
{

Encoder::Encoder(Writer* writer, AVMediaType mediaType) : mediaType(mediaType), writer(writer)
{
    if (mediaType == AVMEDIA_TYPE_VIDEO) this->writer->videoEncoder = this;
    if (mediaType == AVMEDIA_TYPE_AUDIO) this->writer->audioEncoder = this;
}

bool Encoder::init()
{
    const char* str = av_get_media_type_string(mediaType);
    strMediaType = (str ? str : "UNKOWN MEDIA TYPE");

    switch (mediaType) {
    case AVMEDIA_TYPE_VIDEO:
        openVideoStream();
        break;
    case AVMEDIA_TYPE_AUDIO:
        openAudioStream();
        break;
    default:
        std::string msg = "Encoder constructor failed: unknown media type";
        if (infoCallback) infoCallback(msg, writer->filename);
        else std::cout << msg << std::endl;
    }
    return opened;
}

Encoder::~Encoder()
{
    close();
}

void Encoder::close()
{
    if (enc_ctx)       avcodec_free_context(&enc_ctx);
    if (pkt)           av_packet_free(&pkt);
    if (hw_frame)      av_frame_free(&hw_frame);
    if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
    if (cvt_frame)     av_frame_free(&cvt_frame);
    if (swr_ctx)       swr_free(&swr_ctx);
    if (sws_ctx)       sws_freeContext(sws_ctx);
    opened = false;
}

void Encoder::openVideoStream()
{
    if (!writer->fmt_ctx) 
        writer->init();

    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = writer->fmt_ctx->oformat->video_codec;

    first_pass = true;
    pts_offset = 0;

    try {
        const AVCodec* codec;
        ex.ck(pkt = av_packet_alloc(), APA);

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            codec = avcodec_find_encoder_by_name(hw_video_codec_name.c_str());
            std::stringstream str;
            str << "avcodec_find_encoder_by_name: " << hw_video_codec_name;
            if (!codec) throw Exception(str.str());
        }
        else {
            codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec);
            if (!codec) throw Exception("Encoder avcodec_find_encoder");
        }

        ex.ck(stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->video_stream_id = stream->id;
        ex.ck(enc_ctx = avcodec_alloc_context3(codec), AAC3);

        enc_ctx->codec_id = codec_id;
        enc_ctx->bit_rate = video_bit_rate;
        enc_ctx->width = width;
        enc_ctx->height = height;
        enc_ctx->framerate = frame_rate;
        stream->time_base = video_time_base;
        enc_ctx->time_base = video_time_base;
        enc_ctx->gop_size = gop_size;

        cvt_frame = av_frame_alloc();
        cvt_frame->width = enc_ctx->width;
        cvt_frame->height = enc_ctx->height;
        cvt_frame->format = AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(cvt_frame, 0);
        av_frame_make_writable(cvt_frame);

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            enc_ctx->pix_fmt = hw_pix_fmt;
            if (!profile.empty())
                av_opt_set(enc_ctx->priv_data, "profile", profile.c_str(), 0);

            ex.ck(av_hwdevice_ctx_create(&hw_device_ctx, hw_device_type, NULL, NULL, 0), AHCC);
            ex.ck(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx), AHCA);

            AVHWFramesContext* frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
            frames_ctx->format = hw_pix_fmt;
            frames_ctx->sw_format = sw_pix_fmt;
            frames_ctx->width = width;
            frames_ctx->height = height;
            frames_ctx->initial_pool_size = 20;

            ex.ck(av_hwframe_ctx_init(hw_frames_ref), AHCI);
            ex.ck(enc_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref), ABR);
            av_buffer_unref(&hw_frames_ref);

            ex.ck(hw_frame = av_frame_alloc(), AFA);
            ex.ck(av_hwframe_get_buffer(enc_ctx->hw_frames_ctx, hw_frame, 0), AHGB);
        }
        else {
            enc_ctx->pix_fmt = pix_fmt;
        }

        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) enc_ctx->max_b_frames = 2;
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) enc_ctx->mb_decision = 2;

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ex.ck(avcodec_open2(enc_ctx, codec, &opts), AO2);
        ex.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), APFC);

        opened = true;
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Encoder video stream constructor exception: " << e.what();
        if (infoCallback) infoCallback(str.str(), writer->filename);
        else std::cout << str.str() << std::endl;
        close();
    }
}

void Encoder::openAudioStream()
{
    if (!writer->fmt_ctx) 
        writer->init();

    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = writer->fmt_ctx->oformat->audio_codec;

    first_pass = true;
    pts_offset = 0;

    try {
        const AVCodec* codec;
        codec = avcodec_find_encoder(codec_id);

        if (!codec)
            throw Exception(std::string("avcodec_find_decoder could not find ") + avcodec_get_name(codec_id));

        ex.ck(pkt = av_packet_alloc(), APA);
        ex.ck(stream = avformat_new_stream(fmt_ctx, NULL), ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->audio_stream_id = stream->id;
        ex.ck(enc_ctx = avcodec_alloc_context3(codec), AAC3);

        enc_ctx->sample_fmt = sample_fmt;
        enc_ctx->bit_rate = audio_bit_rate;
        enc_ctx->sample_rate = sample_rate;
        enc_ctx->frame_size = nb_samples;
        stream->time_base = audio_time_base;

#if LIBAVCODEC_VERSION_MAJOR < 61
        enc_ctx->channel_layout = channel_layout;
        enc_ctx->channels = channels;
#else
        av_channel_layout_copy(&enc_ctx->ch_layout, &ch_layout);
#endif

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ex.ck(avcodec_open2(enc_ctx, codec, NULL), AO2);
        ex.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), APFC);

        opened = true;
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Encoder audio stream constructor exception: " << e.what();
        if (infoCallback) infoCallback(str.str(), writer->filename);
        else std::cout << str.str() << std::endl;
        close();
    }
}

bool Encoder::cmpFrame(AVFrame* frame)
{
    if (mediaType == AVMEDIA_TYPE_VIDEO)
        return (frame->width == width && frame->height == height && frame->format == pix_fmt);

#if LIBAVCODEC_VERSION_MAJOR < 61
    if (mediaType == AVMEDIA_TYPE_AUDIO)
        return (frame->channels == channels && frame->channel_layout == channel_layout &&
            frame->nb_samples == nb_samples && frame->format == sample_fmt);
#else
        return (frame->ch_layout.nb_channels == ch_layout.nb_channels &&
            frame->ch_layout.u.mask == ch_layout.u.mask &&
            frame->nb_samples == nb_samples && frame->format == sample_fmt);
#endif

    return false;
}

int Encoder::encode(Frame& f)
{
    int ret = 0;

    try {
        if (f.isValid()) {
            f.set_pts(stream);
            AVFrame* frame = f.m_frame;

            if (first_pass) {
                pts_offset = frame->pts;
                first_pass = false;
            }
            frame->pts -= pts_offset;

            if (mediaType == AVMEDIA_TYPE_VIDEO && !cmpFrame(frame)) {
                if (!sws_ctx) {
                    ex.ck(sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                        enc_ctx->width, enc_ctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL), SGC);
                }

                ex.ck(sws_scale(sws_ctx, frame->data, frame->linesize, 0, enc_ctx->height,
                    cvt_frame->data, cvt_frame->linesize), SS);

                cvt_frame->pts = av_rescale_q(frame->pts, video_time_base, enc_ctx->time_base);
                frame = cvt_frame;
            }

            if (mediaType == AVMEDIA_TYPE_AUDIO && !cmpFrame(frame)) {
                if (!swr_ctx) {
                    ex.ck(swr_ctx = swr_alloc(), SA);

#if LIBAVCODEC_VERSION_MAJOR < 61
                    av_opt_set_channel_layout(swr_ctx, "in_channel_layout", frame->channel_layout, 0);
                    av_opt_set_channel_layout(swr_ctx, "out_channel_layout", channel_layout, 0);
#else
                    av_opt_set_chlayout(swr_ctx, "in_chlayout", &frame->ch_layout, 0);
                    av_opt_set_chlayout(swr_ctx, "out_chlayout", &ch_layout, 0);
#endif
                    
                    av_opt_set_int(swr_ctx, "in_sample_rate", frame->sample_rate, 0);
                    av_opt_set_int(swr_ctx, "out_sample_rate", sample_rate, 0);
                    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (AVSampleFormat)frame->format, 0);
                    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", sample_fmt, 0);
                    ex.ck(swr_init(swr_ctx), SI);
                }

                if (!cvt_frame) {
                    cvt_frame = av_frame_alloc();
                    cvt_frame->sample_rate = sample_rate;
                    cvt_frame->format = sample_fmt;
                    cvt_frame->nb_samples = nb_samples;

#if LIBAVCODEC_VERSION_MAJOR < 61
                    cvt_frame->channels = channels;
                    cvt_frame->channel_layout = channel_layout;
#else
                    av_channel_layout_copy(&cvt_frame->ch_layout, &ch_layout);
#endif

                    av_frame_get_buffer(cvt_frame, 0);
                    av_frame_make_writable(cvt_frame);
                }

                ex.ck(swr_convert(swr_ctx, cvt_frame->data, nb_samples, (const uint8_t**)frame->data, frame->nb_samples), SC);
                cvt_frame->pts = av_rescale_q(total_samples, av_make_q(1, enc_ctx->sample_rate), enc_ctx->time_base);
                total_samples += nb_samples;
                frame = cvt_frame;
            }

            if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
                av_frame_copy_props(hw_frame, frame);
                ex.ck(av_hwframe_transfer_data(hw_frame, frame, 0), AHTD);
                ex.ck(avcodec_send_frame(enc_ctx, hw_frame), ASF);
            }
            else {
                ex.ck(avcodec_send_frame(enc_ctx, frame), "avcodec_send_frame");
            }
        }
        else {
            ex.ck(ret = avcodec_send_frame(enc_ctx, NULL), "avcodec_send_frame null frame encountered");
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) 
                ex.ck(ret, CmdTag::ARP);

            pkt->stream_index = stream->index;

            AVPacket* tmp = av_packet_alloc();

            // needed to prevent corruption in dts
            if (pkt->dts > pkt->pts) 
                pkt->dts = pkt->pts - 2;

            tmp = av_packet_clone(pkt);

            if (output == EncoderOutput::FILE) {
                writer->write(tmp);
                av_packet_free(&tmp);
            }
            else {
                Packet p(tmp);
                pkt_q->push_move(p);
            }


        }
    }
    catch (const QueueClosedException& e) {}
    catch (const Exception& e) {
        if (strcmp(e.what(), "End of file")) {
            std::stringstream str;
            str << strMediaType << " encode exception -- " << e.what(); 
            if (infoCallback) infoCallback(str.str(), writer->filename);
            else std::cout << str.str() << std::endl;
        }
    }

    return ret;
}


}
