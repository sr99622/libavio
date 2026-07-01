#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <iostream>

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

    Encoder(AVMediaType media_type, Writer* writer, Queue<Frame>* frames, Queue<Packet>* pkts) : 
            media_type(media_type), writer(writer), frames(frames), pkts(pkts)
    {
        std::cout << "Encoder constructor" << std::endl;

        AVStream* stream = NULL;
        AVCodecContext* enc_ctx = NULL;
        AVPacket* pkt = NULL;
        SwsContext* sws_ctx = NULL;

        AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
        int width = 0;
        int height = 0;
        int video_bit_rate = 0;
        int frame_rate = 0;
        int gop_size = 0;
        AVRational video_time_base = av_make_q(0, 0);
        AVDictionary* opts = NULL;
    }

    int encode() {
        Frame f = frames->pop();
        // std::cout << "encode: " << f.pts() << std::endl;

        if (f.is_null()) {
            std::cout << "encoder recvd null frame" << std::endl;
            writer->input->push(Packet(nullptr));
            return 0;
        }
        return 1;
    }

};

}

#endif // ENCODER_HPP
