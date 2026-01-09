/********************************************************************
* libavio/include/Frame.hpp
*
* Copyright (c) 2025  Stephen Rhodes
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

#ifndef FRAME_HPP
#define FRAME_HPP

extern "C" {
#include <libavutil/frame.h>
}

#include "Exception.hpp"

namespace avio {

class Frame {
public:
    AVFrame* frame = nullptr;
    ExceptionChecker ex;

    Frame() {
        ex.ck((frame = av_frame_alloc()), AFA);
    }

    Frame(AVFrame* raw_frame) {
        if (raw_frame) {
            ex.ck((frame = av_frame_alloc()), AFA);
            av_frame_move_ref(frame, raw_frame);
        } 
    }

    Frame(const Frame& other) {
        //std::cout << "frame copy constructor" << std::endl;
        ex.ck((frame = av_frame_clone(other.frame)), AFC);
    }

    Frame(Frame&& other) noexcept {
        frame = other.frame;
        other.frame = nullptr;
    }

    Frame& operator=(const Frame& other) {
        //std::cout << "frame copy assignment" << std::endl;
        if (this != &other) {
            if (frame) av_frame_free(&frame);
            ex.ck((frame = av_frame_clone(other.frame)), AFC);
        }
        return *this;
    }

    Frame& operator=(Frame&& other) noexcept {
        if (this != &other) {
            if (frame) av_frame_free(&frame);
            frame = other.frame;
            other.frame = nullptr;
        }
        return *this;
    }

    ~Frame() {
        if (frame) av_frame_free(&frame);
    }

    bool       is_null()     const { return frame == nullptr; }
    int64_t    pts()         const { return frame ? frame->pts : AV_NOPTS_VALUE; }
    uint64_t   channels()    const { return frame ? frame->ch_layout.nb_channels : 0; }
    int        samples()     const { return frame ? frame->nb_samples : 0; }
    int        width()       const { return frame ? frame->width : 0; }
    int        height()      const { return frame ? frame->height : 0; }
    int        stride()      const { return frame ? frame->linesize[0] : 0; }
    uint8_t*   data()        const { return frame ? frame->data[0] : nullptr; }
    int        nb_samples()  const { return frame ? frame->nb_samples : 0; }
    int        sample_rate() const { return frame ? frame->sample_rate : 0; }
    int        format()      const { return frame ? frame->format : -1; }
    AVRational time_base()   const { return frame ? frame->time_base : av_make_q(0, 0); }
    
};

}

#endif // FRAME_HPP
