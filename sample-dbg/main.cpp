#include <sstream>
#include <thread>
#include <chrono>
#include <deque>
#include "avio.h"

avio::Reader reader("/home/stephen/Videos/short.mp4");
avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_NONE);
avio::Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
avio::Display display(reader);
avio::Queue<AVPacket*> video_pkt_q(1);
avio::Queue<AVPacket*> audio_pkt_q(1);
avio::Queue<avio::Frame> video_frame_q(1);
avio::Queue<avio::Frame> audio_frame_q(1);

static void read_pkts()
{
    while (true)
    {
        AVPacket* pkt = reader.read();
        if (!pkt)
            break;

        if (pkt->stream_index == reader.video_stream_index)
            video_pkt_q.push(pkt);
        if (pkt->stream_index == reader.audio_stream_index)
            audio_pkt_q.push(pkt);

    }
    std::cout << "done reading packets" << std::endl;
    video_pkt_q.push(nullptr);
    audio_pkt_q.push(nullptr);
    std::cout << "pushed null pkts" << std::endl;
}

static void video_decode()
{
    videoDecoder.frame_q = &video_frame_q;
    //videoDecoder.show_frames = true;
    avio::Frame f;
    while (true)
    {
        AVPacket* pkt;
        video_pkt_q.pop(pkt);
        if (!pkt)
            break;

        videoDecoder.decode(pkt);
        av_packet_free(&pkt);
    }
    std::cout << "video decode thread done" << std::endl;
    video_frame_q.push(avio::Frame(nullptr));
    std::cout << "video decode null frame push" << std::endl;
}

static void audio_decode()
{
    audioDecoder.frame_q = &audio_frame_q;
    //audioDecoder.show_frames = true;
    avio::Frame f;

    while (true)
    {
        AVPacket* pkt;
        audio_pkt_q.pop(pkt);
        if (!pkt)
            break;

        audioDecoder.decode(pkt);
        av_packet_free(&pkt);
    }
    std::cout << "audio decode thread done" << std::endl;
    audio_frame_q.push(avio::Frame(nullptr));
    std::cout << "audio decode null frame push" << std::endl;
}

static void drain_pkts(avio::Queue<AVPacket*>* pkt_q)
{
    while (true) {
        AVPacket* pkt = pkt_q->pop();
        if (!pkt)
            break;

        std::cout << "pkt_q pop: " << pkt->pts << std::endl;
        av_packet_free(&pkt);
    }
}

static void drain_frames(avio::Queue<avio::Frame>* frame_q)
{
    avio::Frame f;
    while (true) {
        frame_q->pop(f);
        if (!f.isValid())
            break;

        std::cout << "frame_q pop: " << f.m_frame->pts << std::endl;
        f.invalidate();
    }
    std::cout << "frame drain done" << std::endl;

}

int main(int argc, char** argv)
{
    std::cout << "hello world" << std::endl;
    //display.vfq_in = &video_frame_q;
    //display.afq_in = &audio_frame_q;
    //display.initAudio(audioDecoder.sample_rate(), audioDecoder.sample_format(), audioDecoder.channels(), audioDecoder.channel_layout(), audioDecoder.frame_size());

    std::thread reader_thread(read_pkts);
    //std::thread video_pkt_thread(drain_pkts, &video_pkt_q);
    //std::thread audio_pkt_thread(drain_pkts, &audio_pkt_q);
    std::thread video_decode_thread(video_decode);
    std::thread audio_decode_thread(audio_decode);
    std::thread video_frame_thread(drain_frames, &video_frame_q);
    std::thread audio_frame_thread(drain_frames, &audio_frame_q);

    //while (display.display()) {}

    reader_thread.join();
    //video_pkt_thread.join();
    //audio_pkt_thread.join();
    video_decode_thread.join();
    audio_decode_thread.join();
    video_frame_thread.join();
    audio_frame_thread.join();

}