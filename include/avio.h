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

#include <iostream>
#include <iomanip>
#include "Exception.h"
#include "Queue.h"
#include "Reader.h"
#include "Decoder.h"
#include "Pipe.h"
#include "Filter.h"
#include "Player.h"
#include "Packet.h"

namespace avio
{

static void read(Reader* reader, Player* player) 
{
    if (reader->has_video() && !player->disable_video) {
        if (reader->vpq_max_size > 0)
            reader->vpq->set_max_size(reader->vpq_max_size);
    }
    if (reader->has_audio() && !player->disable_audio) {
        if (reader->apq_max_size > 0)
            reader->apq->set_max_size(reader->apq_max_size);
    }
    
    try {
        while (true)
        {
            AVPacket* pkt = reader->read();
            if (!pkt) {
                break;
            }

            if (reader->disable_video && pkt->stream_index == reader->video_stream_index) {
                av_packet_free(&pkt);
                continue;
            }

            if (reader->disable_audio && pkt->stream_index == reader->audio_stream_index) {
                av_packet_free(&pkt);
                continue;
            } 

            if (!player->running) {
                if (reader->pipe) reader->close_pipe();
                reader->clear_pkts_cache(0);
                player->clear_queues();
                av_packet_free(&pkt);
                break;
            }

            if (reader->seek_target_pts != AV_NOPTS_VALUE) {
                AVPacket* tmp = reader->seek();
                reader->clear_pkts_cache(0);
                player->clear_queues();
                if (tmp) {
                    av_packet_free(&pkt);
                    pkt = tmp;
                }
                else {
                    reader->seek_target_pts = AV_NOPTS_VALUE;
                    player->running = false;
                }
            }

            if (reader->request_pipe_write) {
                reader->pipe_write(pkt);
            }
            else {
                reader->close_pipe();
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
    catch (const QueueClosedException& e) {}
    catch (const Exception& e) { 
        reader->close_pipe();
        reader->clear_pkts_cache(0);
        std::string msg(e.what());
        std::string mark("-138");
        if (msg.find(mark) != std::string::npos)
            msg = "No connection to server";
        std::stringstream str;
        str << "Reader thread loop error: " << msg;
        std::cout << str.str() << std::endl;
        if (player->errorCallback) reader->errorCallback(str.str());
        else std::cout << str.str() << std::endl;
    }

    try {
        if (reader->vpq) reader->vpq->push_move(Packet(nullptr));
        if (reader->apq) reader->apq->push_move(Packet(nullptr));
    }
    catch (const QueueClosedException& e) {}
}

static void decode(Decoder* decoder) 
{
    try {
        while (true)
        {
            Packet p;
            decoder->pkt_q->pop_move(p);
            if (!p.isValid())
                break;

            decoder->decode(p.m_pkt);
        }
    }
    catch (const QueueClosedException& e) {}
    catch (const Exception& e) { 
        std::stringstream str;
        str << decoder->strMediaType << " decoder failed: " << e.what();
        if (decoder->errorCallback) decoder->errorCallback(str.str());
    }

    decoder->frame_q->push_move(Frame(nullptr));
}

static void filter(Filter* filter)
{
    try {
        while (true)
        {
            Frame f;
            filter->frame_in_q->pop_move(f);
            if (!f.isValid())
                break;

            filter->filter(f);
        }
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "filter loop exception: " << e.what();
        if (filter->errorCallback) filter->errorCallback(str.str());
    }

    filter->frame_out_q->push_move(Frame(nullptr));
}

static void write(Writer* writer, Encoder* encoder)
{
    Frame f;
    while (true) 
    {
        encoder->frame_q->pop_move(f);
        if (!f.isValid()) {
            break;
        }

        if (writer->enabled) {

            if (!encoder->opened) {
                encoder->init();
            }

            if (!writer->opened) {
                writer->open();
            }

            if (writer->opened && encoder->opened) {
                encoder->encode(f);
            }
        }
        else {
            if (writer->opened) {
                if (encoder->opened) {
                    Frame tmp(nullptr);
                    encoder->encode(tmp);
                    encoder->close();
                }
                writer->close();
            }
        }
    }

    if (writer->opened) {
        if (encoder->opened) {
            Frame tmp(nullptr);
            encoder->encode(tmp);
            encoder->close();
        }
        writer->close();
    }
    
}

}

#endif // AVIO_H
