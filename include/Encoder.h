#ifndef ENCODER_H
#define ENCODER_H

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

#include "Exception.h"
#include "Queue.h"
#include "Frame.h"
#include "Packet.h"
#include "Writer.h"

namespace avio
{

enum class EncoderOutput
{
    FILE,
    PACKET
};

class Encoder
{
public:
    Encoder(Writer* writer, AVMediaType mediaType);
    ~Encoder();
    void openVideoStream();
    void openAudioStream();
    bool init();
    int encode(Frame& f);
    void close();
    bool cmpFrame(AVFrame* frame);
    bool opened = false;

    Writer* writer;
    AVMediaType mediaType;
    std::string strMediaType;

    EncoderOutput output = EncoderOutput::FILE;

    AVStream* stream = NULL;
    AVCodecContext* enc_ctx = NULL;
    AVPacket* pkt = NULL;
    SwsContext* sws_ctx = NULL;
    SwrContext* swr_ctx = NULL;
    AVFrame* cvt_frame = NULL;

    AVBufferRef* hw_frames_ref = NULL;
    AVBufferRef* hw_device_ctx = NULL;
    AVFrame* hw_frame = NULL;

    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
    std::string hw_video_codec_name;
    std::string profile;
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    AVPixelFormat sw_pix_fmt = AV_PIX_FMT_NONE;

    AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    int width = 0;
    int height = 0;
    int video_bit_rate = 0;
    int gop_size = 0;
    AVRational frame_rate = av_make_q(0, 0);
    AVRational video_time_base = av_make_q(0, 0);
    AVDictionary* opts = NULL;

    AVSampleFormat sample_fmt = AV_SAMPLE_FMT_NONE;
    uint64_t channel_layout = 0;
    int audio_bit_rate = 0;
    int sample_rate = 0;
    int nb_samples = 0;
    int channels = 0;
    AVRational audio_time_base = av_make_q(0, 0);
    void set_channel_layout_mono() { channel_layout = AV_CH_LAYOUT_MONO; }
    void set_channel_layout_stereo() { channel_layout = AV_CH_LAYOUT_STEREO; }
    int64_t total_samples = 0;

    bool show_frames = false;

    int64_t pts_offset = 0;
    bool first_pass = true;

    Queue<Frame>* frame_q = nullptr;
    Queue<Packet>* pkt_q = nullptr;

    int frame_q_max_size = 0;

	std::function<void(const std::string&, const std::string&)> infoCallback = nullptr;
    ExceptionHandler ex;
};

}

#endif // ENCODER_H