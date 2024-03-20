/********************************************************************
* libavio/include/Display.h
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

#ifndef DISPLAY_H
#define DISPLAY_H

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <chrono>
#include <deque>
#include <functional>
#include "Exception.h"
#include "Queue.h"
#include "Frame.h"
#include "Clock.h"
#include "Decoder.h"
#include "Filter.h"
#include "Reader.h"

#define SDL_EVENT_LOOP_WAIT 30

namespace avio
{

enum AudioStatus {
    UNINITIALIZED,
    INIT_STARTED,
    INITIALIZED
};

class Display
{

public:
    Display(Reader* reader, void* player) : reader(reader), player(player) { }
    ~Display();

    Reader* reader;
    void* player = nullptr;
    uint64_t hWnd = 0;

    int initAudio(Filter* audioFilter);
    static void AudioCallback(void* userdata, uint8_t* stream, int len);
    static void display(void* player);
    
    bool paused = false;
    Frame paused_frame;
    bool isPaused();
    void togglePause();

    bool recording = false;
    void toggleRecord();

    Frame f;

    std::string audioDeviceStatus() const;
    const char* sdlAudioFormatName(SDL_AudioFormat format) const;

    SDL_AudioSpec sdl = { 0 };
    SDL_AudioSpec have = { 0 };

    Queue<Frame>* vfq_in = nullptr;
    Queue<Frame>* afq_in = nullptr;
    Queue<Frame>* vfq_out = nullptr;
    Queue<Frame>* afq_out = nullptr;

    SDL_AudioDeviceID audioDeviceID;
    Clock rtClock;

    SwrContext* swr_ctx = nullptr;
    AVSampleFormat sdl_sample_format = AV_SAMPLE_FMT_S16;

    uint8_t* swr_buffer = nullptr;
    int swr_buffer_size = 0;
    int audio_buffer_len = 0;
    float volume = 1.0f;
    bool mute = false;
    int swr_ptr = 0;

    uint64_t start_time;
    uint64_t duration;

    ExceptionHandler ex;

};

}

#endif // DISPLAY_H