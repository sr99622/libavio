#ifndef FFMPEG_COMPAT_HPP
#define FFMPEG_COMPAT_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

namespace avio {

#if LIBAVUTIL_VERSION_MAJOR >= 57
#define AVIO_HAS_CH_LAYOUT 1
#else
#define AVIO_HAS_CH_LAYOUT 0
#endif

#if LIBSWRESAMPLE_VERSION_MAJOR >= 4
#define AVIO_HAS_SWR_ALLOC_SET_OPTS2 1
#else
#define AVIO_HAS_SWR_ALLOC_SET_OPTS2 0
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 59
#define AVIO_HAS_FRAME_TIME_BASE 1
#else
#define AVIO_HAS_FRAME_TIME_BASE 0
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 59
#define AVIO_HAS_PACKET_TIME_BASE 1
#else
#define AVIO_HAS_PACKET_TIME_BASE 0
#endif

inline int channel_count_from_frame(const AVFrame* frame) {
    if (!frame) return 0;
#if AVIO_HAS_CH_LAYOUT
    return frame->ch_layout.nb_channels;
#else
    return frame->channels;
#endif
}

inline int channel_count_from_codecpar(const AVCodecParameters* codecpar) {
    if (!codecpar) return 0;
#if AVIO_HAS_CH_LAYOUT
    return codecpar->ch_layout.nb_channels;
#else
    return codecpar->channels;
#endif
}

inline int channel_count_from_codecctx(const AVCodecContext* codec_ctx) {
    if (!codec_ctx) return 0;
#if AVIO_HAS_CH_LAYOUT
    return codec_ctx->ch_layout.nb_channels;
#else
    return codec_ctx->channels;
#endif
}

inline uint64_t channel_mask_from_codecpar(const AVCodecParameters* codecpar) {
    if (!codecpar) return 0;
#if AVIO_HAS_CH_LAYOUT
    if (codecpar->ch_layout.order == AV_CHANNEL_ORDER_NATIVE)
        return codecpar->ch_layout.u.mask;
    return av_get_default_channel_layout(codecpar->ch_layout.nb_channels);
#else
    if (codecpar->channel_layout)
        return codecpar->channel_layout;
    return av_get_default_channel_layout(codecpar->channels);
#endif
}

inline uint64_t channel_mask_from_codecctx(const AVCodecContext* codec_ctx) {
    if (!codec_ctx) return 0;
#if AVIO_HAS_CH_LAYOUT
    if (codec_ctx->ch_layout.order == AV_CHANNEL_ORDER_NATIVE)
        return codec_ctx->ch_layout.u.mask;
    return av_get_default_channel_layout(codec_ctx->ch_layout.nb_channels);
#else
    if (codec_ctx->channel_layout)
        return codec_ctx->channel_layout;
    return av_get_default_channel_layout(codec_ctx->channels);
#endif
}

inline const char* sample_fmt_name(AVSampleFormat fmt) {
    const char* name = av_get_sample_fmt_name(fmt);
    return name ? name : "unknown";
}

inline AVRational frame_time_base(const AVFrame* frame) {
#if AVIO_HAS_FRAME_TIME_BASE
    return frame ? frame->time_base : av_make_q(0, 0);
#else
    (void)frame;
    return av_make_q(0, 0);
#endif
}

inline AVRational packet_time_base(const AVPacket* pkt) {
#if AVIO_HAS_PACKET_TIME_BASE
    return pkt ? pkt->time_base : av_make_q(0, 0);
#else
    (void)pkt;
    return av_make_q(0, 0);
#endif
}

inline void describe_codecpar_channel_layout(const AVCodecParameters* codecpar, char* dst, size_t dst_size) {
    if (!dst || !dst_size) return;
    dst[0] = '\0';
    if (!codecpar) return;

#if AVIO_HAS_CH_LAYOUT
    av_channel_layout_describe(&codecpar->ch_layout, dst, dst_size);
#else
    av_get_channel_layout_string(
        dst,
        static_cast<int>(dst_size),
        codecpar->channels,
        codecpar->channel_layout
    );
#endif
}

inline void describe_codecctx_channel_layout(const AVCodecContext* codec_ctx, char* dst, size_t dst_size) {
    if (!dst || !dst_size) return;
    dst[0] = '\0';
    if (!codec_ctx) return;

#if AVIO_HAS_CH_LAYOUT
    AVChannelLayout tmp = codec_ctx->ch_layout;
    if (tmp.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&tmp, codec_ctx->ch_layout.nb_channels);
    }
    av_channel_layout_describe(&tmp, dst, dst_size);
#else
    uint64_t mask = codec_ctx->channel_layout;
    if (!mask)
        mask = av_get_default_channel_layout(codec_ctx->channels);
    av_get_channel_layout_string(
        dst,
        static_cast<int>(dst_size),
        codec_ctx->channels,
        mask
    );
#endif
}

inline int swr_alloc_set_opts_compat(
    SwrContext** swr_ctx,
    const AVCodecParameters* codecpar,
    AVSampleFormat out_fmt,
    int out_rate
) {
#if AVIO_HAS_SWR_ALLOC_SET_OPTS2
    return swr_alloc_set_opts2(
        swr_ctx,
        &codecpar->ch_layout, out_fmt, out_rate,
        &codecpar->ch_layout, static_cast<AVSampleFormat>(codecpar->format), codecpar->sample_rate,
        0, nullptr
    );
#else
    uint64_t mask = channel_mask_from_codecpar(codecpar);
    *swr_ctx = swr_alloc_set_opts(
        *swr_ctx,
        static_cast<int64_t>(mask), out_fmt, out_rate,
        static_cast<int64_t>(mask), static_cast<AVSampleFormat>(codecpar->format), codecpar->sample_rate,
        0, nullptr
    );
    return (*swr_ctx) ? 0 : AVERROR(ENOMEM);
#endif
}

inline int find_best_stream_compat(
    AVFormatContext* fmt_ctx,
    AVMediaType media_type,
    int wanted_stream_nb,
    int related_stream,
    const AVCodec** decoder_ret,
    int flags
) {
#if LIBAVFORMAT_VERSION_MAJOR >= 59
    return av_find_best_stream(fmt_ctx, media_type, wanted_stream_nb, related_stream, decoder_ret, flags);
#else
    return av_find_best_stream(
        fmt_ctx,
        media_type,
        wanted_stream_nb,
        related_stream,
        const_cast<AVCodec**>(decoder_ret),
        flags
    );
#endif
}

} // namespace avio

#endif // FFMPEG_COMPAT_HPP