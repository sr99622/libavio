/********************************************************************
* libavio/include/avio.h
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

#ifndef AVIO_H
#define AVIO_H

#include "Exception.h"
#include "Queue.h"
#include "Reader.h"
#include "Decoder.h"
#include "Pipe.h"
#include "Filter.h"
#include "Display.h"
#include "Process.h"
#include <iomanip>
#include <functional>
#include <map>



#define P ((Process*)process)

namespace avio
{

static void show_pkt(AVPacket* pkt)
{
    std::stringstream str;
    str 
        << " index: " << pkt->stream_index
        << " flags: " << pkt->flags
        << " pts: " << pkt->pts
        << " dts: " << pkt->dts
        << " size: " << pkt->size
        << " duration: " << pkt->duration;

    std::cout << str.str() << std::endl;
}

static void read(Process* process) 
{
    Reader* reader = process->reader;
    if (reader->has_video() && !reader->vpq_name.empty()) {
        reader->vpq = process->pkt_queues[reader->vpq_name];
        if (reader->vpq_max_size > 0)
            reader->vpq->set_max_size(reader->vpq_max_size);
    }
    if (reader->has_audio() && !reader->apq_name.empty()) {
        reader->apq = process->pkt_queues[reader->apq_name];
        if (reader->apq_max_size > 0)
            reader->apq->set_max_size(reader->apq_max_size);
    }

    Pipe* pipe = nullptr;
    std::deque<AVPacket*> pkts;
    int keyframe_count = 0;
    int keyframe_marker = 0;

    try {
        while (true)
        {
            AVPacket* pkt = reader->read();
            if (!pkt)
                break;

            if (!process->running) {
                process->clear_queues();
                while (pkts.size() > 0) {
                    AVPacket* jnk = pkts.front();
                    pkts.pop_front();
                    av_packet_free(&jnk);
                }
                av_packet_free(&pkt);
                if (pipe) {
                    pipe->close();
                    delete pipe;
                }
                break;
            }

            if (reader->seek_target_pts != AV_NOPTS_VALUE) {

                AVPacket* tmp = reader->seek();

                while (pkts.size() > 0) {
                    AVPacket* jnk = pkts.front();
                    pkts.pop_front();
                    av_packet_free(&jnk);
                }

                if (tmp) {
                    av_packet_free(&pkt);
                    pkt = tmp;
                    process->clear_queues();
                }
                else {
                    break;
                }
            }

            if (reader->request_pipe_write) {
                // pkts queue caches recent packets based off key frame packet
                if (!pipe) {
                    pipe = new Pipe(*reader);
                    pipe->process = reader->process;
                    if (pipe->open(reader->pipe_out_filename)) {
                        while (pkts.size() > 0) {
                            AVPacket* tmp = pkts.front();
                            pkts.pop_front();
                            pipe->write(tmp);
                            av_packet_free(&tmp);
                        }
                    }
                    else {
                        delete pipe;
                        pipe = nullptr;
                    }
                }
                // verify pipe was opened successfully before continuing
                if (pipe) {
                    AVPacket* tmp = av_packet_clone(pkt);
                    pipe->write(tmp);
                    av_packet_free(&tmp);
                }
            }
            else {
                if (pipe) {
                    pipe->close();
                    delete pipe;
                    pipe = nullptr;
                }
                if (pkt->stream_index == reader->video_stream_index) {
                    if (pkt->flags) {
                        // key frame packet found in stream
                        if (++keyframe_count >= reader->keyframe_cache_size()) {
                            while (pkts.size() > keyframe_marker) {
                                AVPacket* tmp = pkts.front();
                                pkts.pop_front();
                                av_packet_free(&tmp);
                            }
                            keyframe_count--;
                        }
                        keyframe_marker = pkts.size();
                    }
                }
                AVPacket* tmp = av_packet_clone(pkt);
                pkts.push_back(tmp);
            }

            if (reader->stop_play_at_pts != AV_NOPTS_VALUE && pkt->stream_index == reader->seek_stream_index()) {
                if (pkt->pts > reader->stop_play_at_pts) {
                    av_packet_free(&pkt);
                    break;
                }
            }

            if (pkt->stream_index == reader->video_stream_index) {
                if (reader->show_video_pkts) show_pkt(pkt);
                if (reader->vpq)
                    reader->vpq->push(pkt);
                else
                    av_packet_free(&pkt);
            }

            else if (pkt->stream_index == reader->audio_stream_index) {
                if (reader->show_audio_pkts) show_pkt(pkt);
                if (reader->apq)
                    reader->apq->push(pkt);
                else
                    av_packet_free(&pkt);
            }
        }
        if (reader->vpq) reader->vpq->push(nullptr);
        if (reader->apq) reader->apq->push(nullptr);
    }
    //catch (const QueueClosedException& e) { }
    catch (const Exception& e) { 
        std::cout << " reader failed: " << e.what() << std::endl; 
    }
}

static void decode(Decoder* decoder, Queue<AVPacket*>* pkt_q, Queue<Frame>* frame_q) 
{
    decoder->frame_q = frame_q;
    decoder->pkt_q = pkt_q;

    try {
        while (AVPacket* pkt = pkt_q->pop())
        {
            decoder->decode(pkt);
            av_packet_free(&pkt);
        }
        decoder->decode(nullptr);
        decoder->flush();
        decoder->frame_q->push(Frame(nullptr));
    }
    //catch (const QueueClosedException& e) { }
    catch (const Exception& e) { 
        std::stringstream str;
        str << decoder->strMediaType << " decoder failed: " << e.what();
        std::cout << str.str() << std::endl;
        //decoder->reader->exit_error_msg = str.str();
        decoder->decode(nullptr);
        decoder->frame_q->push(Frame(nullptr));
    }
}

static void filter(Filter* filter, Queue<Frame>* q_in, Queue<Frame>* q_out)
{
    try {
        filter->frame_out_q = q_out;
        while (true)
        {
            Frame f;
            q_in->pop_move(f);
            filter->filter(f);
            if (!f.isValid())
                break;
        }
        filter->frame_out_q->push_move(Frame(nullptr));
    }
    catch (const Exception& e) {
        std::cout << "filter loop exception: " << e.what() << std::endl;
    }
}


}

#endif // AVIO_H