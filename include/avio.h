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
#include "Process.h"
#include "Packet.h"

namespace avio
{

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


    try {
        while (true)
        {
            AVPacket* pkt = reader->read();
            if (!pkt) {
                std::string msg = "reader recieved null packet eof";
                if (process->infoCallback) process->infoCallback(process->infoCaller, msg);
                break;
            }

            if (!process->running) {
                if (reader->pipe) reader->close_pipe();
                reader->clear_pkts_cache(0);
                process->clear_queues();
                av_packet_free(&pkt);
                break;
            }

            if (reader->seek_target_pts != AV_NOPTS_VALUE) {
                AVPacket* tmp = reader->seek();
                reader->clear_pkts_cache(0);
                if (tmp) {
                    av_packet_free(&pkt);
                    pkt = tmp;
                    process->clear_queues();
                    //process->clear_decoders();
                }
                else {
                    break;
                }
            }

            if (reader->request_pipe_write) {
                reader->pipe_write(pkt);
            }
            else {
                if (reader->pipe) reader->close_pipe();
                reader->fill_pkts_cache(pkt);
            }

            if (reader->stop_play_at_pts != AV_NOPTS_VALUE && pkt->stream_index == reader->seek_stream_index()) {
                if (pkt->pts > reader->stop_play_at_pts) {
                    av_packet_free(&pkt);
                    break;
                }
            }

            Packet p(pkt);
            if (pkt->stream_index == reader->video_stream_index) {
                if (reader->show_video_pkts)
                    std::cout << p.description() << std::endl;
                if (reader->vpq)
                    reader->vpq->push_move(p);
            }

            else if (pkt->stream_index == reader->audio_stream_index) {
                if (reader->show_audio_pkts)
                    std::cout << p.description() << std::endl;
                if (reader->apq)
                    reader->apq->push_move(p);
            }
        }
    }
    catch (const Exception& e) { 
        std::cout << " reader failed: " << e.what() << std::endl;
        if (process->errorCallback)
            process->errorCallback(process->errorCaller, e.what());
    }
    if (reader->vpq) reader->vpq->push_move(Packet(nullptr));
    if (reader->apq) reader->apq->push_move(Packet(nullptr));
}

static void decode(Process* process, AVMediaType mediaType) 
{

    Decoder* decoder = nullptr;
    switch (mediaType) {
        case AVMEDIA_TYPE_VIDEO:
            decoder = process->videoDecoder;
            break;
        case AVMEDIA_TYPE_AUDIO:
            decoder = process->audioDecoder;
            break;
    }

    if (!decoder) {
        std::cout << "no decoder in decode loop" << std::endl; 
        return;
    }

    decoder->frame_q = process->frame_queues[decoder->frame_q_name];
    decoder->pkt_q = process->pkt_queues[decoder->pkt_q_name];

    try {
        while (true)
        {
            Packet p;
            decoder->pkt_q->pop_move(p);
            if (!p.isValid())
                break;

            decoder->decode(p.m_pkt);
        }
        decoder->decode(nullptr);
        decoder->flush();
        decoder->frame_q->push(Frame(nullptr));
    }
    catch (const Exception& e) { 
        std::stringstream str;
        str << decoder->strMediaType << " decoder failed: " << e.what();
        std::cout << str.str() << std::endl;
        decoder->decode(nullptr);
        decoder->frame_q->push(Frame(nullptr));
    }
}

static void filter(Process* process, AVMediaType mediaType)
{
    Filter* filter = nullptr;
    switch (mediaType) {
        case AVMEDIA_TYPE_VIDEO:
            filter = process->videoFilter;
            break;
        case AVMEDIA_TYPE_AUDIO:
            filter = process->audioFilter;
            break;
    }

    if (!filter) {
        std::cout << "no filter in filter loop" << std::endl; 
        return;
    }

    filter->frame_in_q = process->frame_queues[filter->q_in_name];
    filter->frame_out_q = process->frame_queues[filter->q_out_name];

    try {
        while (true)
        {
            Frame f;
            filter->frame_in_q->pop_move(f);
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