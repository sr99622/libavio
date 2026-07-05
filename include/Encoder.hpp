#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <iostream>
#include <exception>

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "Reader.hpp"
#include "Writer.hpp"
#include "Queue.hpp"
#include "Packet.hpp"
#include "Frame.hpp"

namespace avio {

class Encoder {
public:
    AVMediaType media_type;
    Queue<Packet>* pkts = nullptr;
    Queue<Frame>* frames = nullptr;
    Writer* writer = nullptr;
    ExceptionChecker ex;
    std::string codec_name = "h264";
    std::string format = "mp4";
    AVFormatContext* fmt_ctx = nullptr;
    std::string str_media_type = "unknown media type";
    std::string output_filename;

    AVStream* stream = nullptr;
    const AVCodec* codec = nullptr;
    AVCodecContext* enc_ctx = nullptr;
    AVPacket* pkt = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* cvt_frame = nullptr;

    AVRational video_time_base = av_make_q(0, 0);
    AVDictionary* opts = nullptr;
    AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    int width = 0;
    int height = 0;
    int video_bit_rate = 0;
    AVRational frame_rate = av_make_q(0, 0);
    int gop_size = 0;

    Encoder() {}

    Encoder(AVMediaType media_type, Writer* writer, Queue<Frame>* frames, Queue<Packet>* pkts) : 
            media_type(media_type), writer(writer), frames(frames), pkts(pkts)
    {
        const char* str = av_get_media_type_string(media_type);
        str_media_type = (str ? str : "unknown media type");
        std::cout << str_media_type << " encoder constructor" << std::endl;
    }

    ~Encoder() {
        if (fmt_ctx) {
            avformat_free_context(fmt_ctx);
            fmt_ctx = nullptr;
            std::cout << "active fmt_ctx freed" << std::endl;
        }
        if (enc_ctx)       avcodec_free_context(&enc_ctx);
        if (pkt)           av_packet_free(&pkt);
        if (cvt_frame)     av_frame_free(&cvt_frame);
        std::cout  << str_media_type << " encoder destructor" << std::endl;
    }

    void open_video_stream() {
        std::cout << "open_video_stream" << std::endl;
        ex.ck(avformat_alloc_output_context2(&fmt_ctx, nullptr, format.c_str(), output_filename.c_str()), AAOC2);
        codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec);
        if (!codec) throw std::runtime_error("avcodec_find_encoder");

        ex.ck(stream = avformat_new_stream(fmt_ctx, nullptr), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;

        stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->codec_id = AV_CODEC_ID_H264;
        stream->codecpar->width = width;
        stream->codecpar->height = height;
        stream->time_base = video_time_base; // 30 FPS timebase

        AVCodecID codec_id = fmt_ctx->oformat->video_codec;
        ex.ck(enc_ctx = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc_ctx->codec_id = codec_id;
        enc_ctx->bit_rate = video_bit_rate;
        enc_ctx->width = width;
        enc_ctx->height = height;
        enc_ctx->time_base = stream->time_base;
        enc_ctx->gop_size = gop_size;

        cvt_frame = av_frame_alloc();
        cvt_frame->width = enc_ctx->width;
        cvt_frame->height = enc_ctx->height;
        cvt_frame->format = AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(cvt_frame, 0);
        av_frame_make_writable(cvt_frame);
    }

    void open_file() {
        std::cout << str_media_type << "encoder open file: " << output_filename << std::endl;
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            std::cout << "AVFMT_NOFILE" << std::endl;
            ex.ck(avio_open(&fmt_ctx->pb, output_filename.c_str(), AVIO_FLAG_WRITE), AO);
        }
        ex.ck(avformat_write_header(fmt_ctx, NULL), AWH);
        std::cout << "header written successfully" << std::endl;
    }

    void close_file() {
        std::cout << str_media_type << " encoder close file: " << output_filename << std::endl;
        ex.ck(av_write_trailer(fmt_ctx), AWT);

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            ex.ck(avio_closep(&fmt_ctx->pb), AC);
    }

    int encode() {
        Frame f = frames->pop();
        std::cout << str_media_type << " encode: " << f.pts() << std::endl;

        if (f.is_null()) {
            std::cout << str_media_type << " encoder recvd null frame" << std::endl;
            //writer->input->push(Packet(nullptr));
            return 0;
        }
        return 1;
    }

};

}

#endif // ENCODER_HPP
