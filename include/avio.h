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

static void read(Reader* reader) 
{
    Player* player = (Player*)reader->player;
    if (reader->has_video() && !(player->disable_video)) {
        if (reader->vpq_max_size > 0)
            reader->vpq->set_max_size(reader->vpq_max_size);
    }
    if (reader->has_audio() && !(player->disable_audio)) {
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

            if (!player->running) {
                av_packet_free(&pkt);
                break;
            }

            if (reader->vpq) {
                if (player->isCameraStream() && (reader->vpq->size() == reader->vpq->max_size()) && !pkt->flags) {
                    av_packet_free(&pkt);
                    player->infoCallback("dropping frames due to buffer overflow", player->uri);
                    if (player->packetDrop) player->packetDrop(player->uri);
                    continue;
                }
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
                    continue;
                }
            }

            if (player->isCameraStream()) {
                if (reader->request_pipe_write) {
                    reader->pipe_bytes_written += pkt->size;
                    reader->pipe_write(pkt);
                }
                else {
                    reader->close_pipe();
                }
                reader->fill_pkts_cache(pkt);
            }

            Packet p(pkt);
            if (!player->hidden) {
                if (pkt->stream_index == reader->video_stream_index) {
                    if (reader->show_video_pkts)
                        std::cout << p.description() << std::endl;
                    if (reader->vpq && !player->disable_video) {
                        reader->vpq->push_move(p);
                    }
                }
                else if (pkt->stream_index == reader->audio_stream_index) {
                    if (reader->show_audio_pkts)
                        std::cout << p.description() << std::endl;
                    if (reader->apq && !player->disable_audio) {
                        if (!reader->apq->full() || !reader->has_video())
                            reader->apq->push_move(p);
                    }
                }
            }
        }
    }
    catch (const QueueClosedException& e) { }
    catch (const Exception& e) {
        std::string msg(e.what());
        std::string mark("-138");
        if (msg.find(mark) != std::string::npos)
            msg = "No connection to server";
        std::stringstream str;
        str << "read error: " << msg;
        if (player->errorCallback) player->errorCallback(str.str(), player->uri, true);
        else std::cout << str.str() << std::endl;
    }

    try {
        reader->close_pipe();
        reader->clear_pkts_cache(0);
        player->clear_queues();
        if (reader->vpq) reader->vpq->push_move(Packet(nullptr));
        if (reader->apq) reader->apq->push_move(Packet(nullptr));
    }
    catch (const QueueClosedException& e) { }
}

static void decode(Decoder* decoder) 
{
    try {
        while (true)
        {
            bool running = true;
            if (decoder->reader) {
                Reader* reader = (Reader*)decoder->reader;
                if (reader->player) {
                    Player* player = (Player*)reader->player;
                    running = player->running;
                }
            }
            Packet p;
            decoder->pkt_q->pop_move(p);
            if (!p.isValid() || !running)
                break;

            decoder->decode(p.m_pkt);
        }
    }
    catch (const QueueClosedException& e) { }
    catch (const Exception& e) { 
        std::stringstream str;
        str << decoder->strMediaType << " decoder failed: " << e.what();
        if (decoder->errorCallback) decoder->errorCallback(str.str(), "", false);
    }

    try {
        decoder->frame_q->push_move(Frame(nullptr));
    }
    catch (const QueueClosedException& e) { }
}

static void filter(Filter* filter)
{
    try {
        while (true)
        {
            bool running = true;
            if (filter->decoder) {
                Decoder* decoder = (Decoder*)filter->decoder;
                if (decoder->reader) {
                    Reader* reader = (Reader*)decoder->reader;
                    if (reader->player) {
                        Player* player = (Player*)reader->player;
                        running = player->running;
                    }
                }
            }
            Frame f;
            filter->frame_in_q->pop_move(f);
            if (!f.isValid() || !running)
                break;

            filter->filter(f);
        }
    }
    catch (const QueueClosedException& e) { }
    catch (const Exception& e) {
        std::stringstream str;
        str << "filter loop exception: " << e.what();
        if (filter->errorCallback) filter->errorCallback(str.str(), "", false);
    }
    try {
        filter->frame_out_q->push_move(Frame(nullptr));
    }
    catch (const QueueClosedException& e) { }
}

/*
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
*/

}

#endif // AVIO_H
