/********************************************************************
* libavio/include/Audio.hpp
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

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <SDL.h>
#include <vector>

extern "C" {
#include <libswresample/swresample.h>
}    

#include "Frame.hpp"
#include "Queue.hpp"
#include "Reader.hpp"
#include "Exception.hpp"

namespace avio {

class Audio {
public:
    SDL_AudioSpec sdl = { 0 };
    SDL_AudioSpec have = { 0 };
    SDL_AudioDeviceID device_id = -1;
    Reader* reader = nullptr;
    Queue<Frame>* frames = nullptr;
    ExceptionChecker ex;
    SwrContext* swr_ctx = nullptr;
    AVSampleFormat output_format = AV_SAMPLE_FMT_S16;

    uint8_t* buffer = nullptr;
    int size = 0;
    uint8_t* temp = nullptr;
    int temp_size = 0;
    int residual = 0;

    float volume = 1.0f;
    bool mute = false;
    bool closed = false;
    int audio_driver_index = 0; 

    
    std::function<void(const Frame&, const std::string& uri)> pyAudioCallback = nullptr;
    std::function<void(float progress, const std::string& uri)> progressCallback = nullptr;
    int last_progress = 0;

    Audio(Reader* reader, Queue<Frame>* frames, int audio_driver_index);
    ~Audio();
    int get_number_of_samples(AVCodecParameters* codecpar);
    void update_progress(int64_t pts);
    void error(const std::string& msg);
};

void callback(void* user_data, uint8_t* output_buffer, int output_length) {
    Audio* audio = (Audio*)user_data;
    memset(output_buffer, 0, output_length);
    int avail = output_length;

    if (audio->reader->terminated) {
        audio->frames->clear();
        audio->closed = true;
        SDL_PauseAudioDevice(audio->device_id, 1);
        return;
    }

    if (audio->reader->paused) 
        return;

    try {
        if (audio->temp_size != output_length) {
            if (audio->temp) free(audio->temp);
            audio->ex.ck(audio->temp = (uint8_t*)malloc(output_length));
            audio->temp_size = output_length;
        }
        memset(audio->temp, 0, output_length);

        while (avail > 0 && !audio->closed) {
            if (!audio->residual) {

                if (audio->reader->live_stream)
                    audio->reader->audio_pkts->remove_latency();

                Frame f = audio->frames->pop();

                if (f.is_null() || audio->reader->terminated) {
                    audio->closed = true;
                    return;
                }
                else {
                    if (audio->reader->seek_pts != AV_NOPTS_VALUE) {
                        return;
                    }
                    
                    int64_t rts = audio->reader->real_time(audio->reader->audio_stream_index, f.pts());
                    audio->reader->update_rt(audio->reader->audio_stream_index, rts);
                    int input_size = av_samples_get_buffer_size(NULL, f.channels(), f.samples(), audio->output_format, 0);
                    if (audio->size != input_size) {
                        if (audio->buffer) free(audio->buffer);
                        audio->ex.ck(audio->buffer = (uint8_t*)malloc(input_size));
                        audio->size = input_size;
                    }
                    const uint8_t** data = (const uint8_t**)&f.frame->data[0];
                    audio->ex.ck(swr_convert(audio->swr_ctx, &audio->buffer, audio->have.samples, data, f.samples()), SC);

                    int to_write = 0;
                    if (audio->size > avail) {
                        to_write = avail;
                        audio->residual = avail;
                    }
                    else {
                        to_write = audio->size;
                    }

                    int accum = output_length - avail;
                    memcpy(audio->temp + accum, audio->buffer, to_write);
                    avail -= to_write;
                }

                if (audio->pyAudioCallback) {
                    audio->pyAudioCallback(f, audio->reader->uri);
                }
                if (audio->progressCallback) {
                    audio->update_progress(f.pts());
                }

            }
            else {
                int data_size = audio->size - audio->residual; 
                int length = (data_size > output_length) ? output_length : data_size;
                memcpy(audio->temp, audio->buffer + audio->residual, length);
                avail -= length;
                audio->residual -= (audio->size - length);
            }
        }
        if (!audio->mute) {
            SDL_MixAudioFormat(output_buffer, audio->temp, audio->sdl.format, output_length, SDL_MIX_MAXVOLUME * audio->volume);
        }
    }
    catch (const std::exception& e) {
        std::cout << "audio callback error: " << e.what() << std::endl;
    }
}

Audio::Audio(Reader* reader, Queue<Frame>* frames, int audio_driver_index) : reader(reader), frames(frames), audio_driver_index(audio_driver_index) {
    AVCodecParameters* codecpar = reader->fmt_ctx->streams[reader->audio_stream_index]->codecpar;
    ex.ck(swr_ctx = swr_alloc());
    ex.ck(swr_alloc_set_opts2(&swr_ctx, &codecpar->ch_layout, output_format, codecpar->sample_rate,
        &codecpar->ch_layout, (AVSampleFormat)codecpar->format, codecpar->sample_rate, 0, NULL), SASO);
    ex.ck(swr_init(swr_ctx), SI);

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_SetHint("SDL_AUDIODRIVER", SDL_GetAudioDriver(audio_driver_index));
        if (SDL_Init(SDL_INIT_AUDIO)) {
            error("SDL audio init error");
        }
        else {
            std::stringstream str;
            str << "Using SDL audio driver " << SDL_GetCurrentAudioDriver();
            std::cout << str.str() << std::endl;
        }
    }

    sdl.channels = codecpar->ch_layout.nb_channels;
    sdl.freq = codecpar->sample_rate;
    sdl.silence = 0;
    sdl.samples = get_number_of_samples(codecpar);
    sdl.userdata = this;
    sdl.callback = callback;
    sdl.format = AUDIO_S16SYS;

    if (!(device_id = SDL_OpenAudioDevice(NULL, 0, &sdl, &have, 0)))
        error("SDL_OpenAudioDevice error");

    SDL_PauseAudioDevice(device_id, 0);
}

Audio::~Audio() {
    if (SDL_WasInit(SDL_INIT_AUDIO) && device_id > 0) {
        //SDL_PauseAudioDevice(device_id, 1);
        //SDL_LockAudioDevice(device_id);
        SDL_CloseAudioDevice(device_id);
    }
    if (swr_ctx) swr_free(&swr_ctx);
    if (buffer) free(buffer);
    if (temp) free(temp);
}

int Audio::get_number_of_samples(AVCodecParameters* codecpar) {
    int samples = codecpar->frame_size;
    if ( !samples && 
            codecpar->codec_id != AV_CODEC_ID_VORBIS && 
            codecpar->codec_id != AV_CODEC_ID_OPUS ) {
        int count = 0;
        while (!frames->size()) {
            SDL_Delay(10);
            count++;
            if (count > 100)
                break;
        }
        if (frames->size())
            samples = frames->peek()->nb_samples();
    }
    return samples;
}

void Audio::update_progress(int64_t pts) {
    if (progressCallback) {
        int64_t duration = reader->duration();
        if (duration) {
            float pct = (float)reader->real_time(reader->audio_stream_index, pts) / (float)duration;
            int progress = (int)(1000*pct);
            if (progress != last_progress) {
                progressCallback(pct, reader->uri);
                last_progress = progress;
            }
        }
    }
}

void Audio::error(const std::string& msg) {
    std::stringstream str;
    str << msg << " : " << SDL_GetError();
    throw std::runtime_error(str.str());
}

}

#endif // AUDIO_HPP