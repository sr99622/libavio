/********************************************************************
* libavio/include/Pipe.h
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

#ifndef PIPE_H
#define PIPE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "Exception.h"
#include "Packet.h"
#include "Frame.h"
#include "Queue.h"
#include <map>

namespace avio
{

class Pipe
{
public:
    Pipe(AVFormatContext* input_fmt_ctx, int video_stream_index, int audio_stream_index, void* decoder = nullptr, void* encoder = nullptr);
    ~Pipe();

	std::function<void(const std::string&, const std::string&)> infoCallback = nullptr;
	std::function<void(const std::string&, const std::string&, bool)> errorCallback = nullptr;

    AVCodecContext* getContext(AVMediaType mediaType);
    void open(const std::string& filename, std::map<std::string, std::string>& metadata);
    void close();
    void adjust_pts(AVPacket* pkt);
    void write(AVPacket* pkt);
    void transcode(AVPacket* pkt);
    int encode(Frame& f);

    std::string m_filename;

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_ctx = nullptr;
    AVCodecContext* audio_ctx = nullptr;
    AVStream* video_stream = nullptr;
    AVStream* audio_stream = nullptr;

    AVFormatContext* input_fmt_ctx = nullptr;
    int video_stream_index = -1;
    int audio_stream_index = -1;

    int64_t video_next_pts = 0;
    int64_t audio_next_pts = 0;
    int last_video_pkt_duration = 0;

    bool opened = false;

    AVRational onvif_frame_rate;
    AVRational encoder_frame_rate;

    void* decoder = nullptr;
    void* encoder = nullptr;

    ExceptionHandler ex;

};

}

#endif // PIPE_H