/********************************************************************
* libavio/src/Display.cpp
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

#include "Display.h"
#include "Player.h"

namespace avio 
{

Display::~Display()
{
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_LockAudioDevice(audioDeviceID);
        SDL_CloseAudioDevice(audioDeviceID);
    }
    if (swr_ctx) swr_free(&swr_ctx);
    if (swr_buffer) delete[] swr_buffer;
}

void Display::display(void* player)
{
    while (true)
    {
        if (!P->hasVideo()){
            if (!P->running)
                return;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        try {
            if (P->display->paused && !P->reader->seeking()) 
            {
                if (!P->running) {
                    P->display->togglePause();
                    continue;
                }

                P->display->f = P->display->paused_frame;
                
                if (P->renderCallback)
                    P->renderCallback(P->display->f, *P);

                if (!P->process_pause)
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));

                continue;
            }

            if (P->display->vfq_in) {
                P->display->vfq_in->pop_move(P->display->f);
                if (P->display->f.pts() == P->reader->seek_found_pts) {
                    P->reader->seek_found_pts = AV_NOPTS_VALUE;
                }
                if (!P->display->f.isValid())
                    break;
            }

            int64_t delay = P->display->rtClock.update(P->display->f.m_rts - P->reader->start_time());
            if (!P->isCameraStream() || P->sync_audio) {
                P->display->paused_frame = P->display->f;
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }

            if (P->renderCallback) P->renderCallback(P->display->f, *P);
            P->reader->last_video_pts = P->display->f.m_frame->pts;

            if (P->progressCallback) {
                if (P->reader->duration()) {
                    float pct = (float)P->display->f.m_rts / (float)(P->reader->duration());
                    int progress = (int)(1000 * pct);
                    if (progress != P->last_progress) {
                        P->progressCallback(pct, P->uri);
                        P->last_progress = progress;
                    }
                }
            }

            if (P->display->vfq_out)
                P->display->vfq_out->push_move(P->display->f);
            else
                P->display->f.invalidate();
        }
        catch (const QueueClosedException& e) { std::cout << "Display closed queue exception 1" << std::endl; }
        catch (const Exception& e) {
            std::stringstream str;
            str << "Display exception: " << e.what();
            if (P->infoCallback) P->infoCallback(str.str(), P->uri);
            else std::cout << str.str() << std::endl;
        }
    }

    try {
        P->display->f.invalidate();
        if (P->display->vfq_out)
            P->display->vfq_out->push_move(P->display->f);
    }
    catch (const QueueClosedException& e) { std::cout << "Display closed queue exception 2" << std::endl; }
}

int Display::initAudio(Filter* audioFilter)
{
    int stream_sample_rate = audioFilter->sample_rate();
    AVSampleFormat stream_sample_format = audioFilter->sample_format();
    int stream_channels = audioFilter->channels();
    int stream_nb_samples = audioFilter->frame_size();
    int ret = 0;

#if LIBAVCODEC_VERSION_MAJOR < 61
    uint64_t stream_channel_layout = audioFilter->channel_layout();
#endif

    try {

        if (stream_nb_samples == 0) {
            int audio_frame_size = av_samples_get_buffer_size(NULL, stream_channels, 1, sdl_sample_format, 1);
            int bytes_per_second = av_samples_get_buffer_size(NULL, stream_channels, stream_sample_rate, sdl_sample_format, 1);
            stream_nb_samples = bytes_per_second * 0.02f / audio_frame_size;
        }

#if LIBAVCODEC_VERSION_MAJOR < 61
        if (!stream_channel_layout || stream_channels != av_get_channel_layout_nb_channels(stream_channel_layout)) {
            stream_channel_layout = av_get_default_channel_layout(stream_channels);
            stream_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
        }
        stream_channels = av_get_channel_layout_nb_channels(stream_channel_layout);
#endif

        sdl.channels = stream_channels;
        sdl.freq = stream_sample_rate;
        sdl.silence = 0;
        sdl.samples = stream_nb_samples;
        sdl.userdata = this;
        sdl.callback = AudioCallback;

        switch (sdl_sample_format) {
        case AV_SAMPLE_FMT_FLT:
            sdl.format = AUDIO_F32;
            break;
        case AV_SAMPLE_FMT_S16:
            sdl.format = AUDIO_S16;
            break;
        case AV_SAMPLE_FMT_U8:
            sdl.format = AUDIO_U8;
            break;
        default:
            const char* result = "unknown sample format";
            const char* name = av_get_sample_fmt_name(sdl_sample_format);
            if (name)
                result = name;
            std::stringstream str;
            str << "ERROR: incompatible sample format: " << result << "supported formats: AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_U8";
            throw Exception(str.str());
        }

        audio_buffer_len = av_samples_get_buffer_size(NULL, sdl.channels, sdl.samples, sdl_sample_format, 1);

#if LIBAVCODEC_VERSION_MAJOR < 61
        ex.ck(swr_ctx = swr_alloc_set_opts(NULL, stream_channel_layout, sdl_sample_format, stream_sample_rate,
            stream_channel_layout, stream_sample_format, stream_sample_rate, 0, NULL), SASO);
#else
        AVChannelLayout stream_channel_layout;
        av_buffersink_get_ch_layout(audioFilter->sink_ctx, &stream_channel_layout);
        ex.ck(swr_alloc_set_opts2(&swr_ctx, &stream_channel_layout, sdl_sample_format, stream_sample_rate,
            &stream_channel_layout, stream_sample_format, stream_sample_rate, 0, NULL), SASO);
        av_channel_layout_uninit(&stream_channel_layout);
#endif

        ex.ck(swr_init(swr_ctx), SI);

        if (P->getAudioStatus && P->setAudioStatus) {
            // multi stream threading issues during SLD_Init for audio, simple mutex held by main window
            AudioStatus status = P->getAudioStatus();
            switch (status) {
                case AudioStatus::UNINITIALIZED:
                    P->setAudioStatus(AudioStatus::INIT_STARTED);
                    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
                        SDL_SetHint("SDL_AUDIODRIVER", SDL_GetAudioDriver(P->audio_driver_index));
                        if (SDL_Init(SDL_INIT_AUDIO)) {
                            std::stringstream str;
                            str << "SDL audio init error: " << SDL_GetError();
                            throw Exception(str.str());
                        }
                        else {
                            P->setAudioStatus(AudioStatus::INITIALIZED);
                            std::stringstream str;
                            str << "Using SDL audio driver " << SDL_GetCurrentAudioDriver();
                            if (P->infoCallback) P->infoCallback(str.str(), P->uri);
                        }
                    }
                break;
                case AudioStatus::INIT_STARTED:
                    while (P->getAudioStatus() == AudioStatus::INIT_STARTED) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                break;
                case AudioStatus::INITIALIZED:
                break;
            }
        }
        else {
            // fall back to basic operation
            if (P->infoCallback) P->infoCallback("getAudioStatus callback function not set", P->uri);
            else std::cout << "getAudioStatus callback function not set" << std::endl;
            if (!SDL_WasInit(SDL_INIT_AUDIO)) {
                SDL_SetHint("SDL_AUDIODRIVER", SDL_GetAudioDriver(P->audio_driver_index));
                if (SDL_Init(SDL_INIT_AUDIO)) {
                    std::stringstream str;
                    str << "SDL audio init error: " << SDL_GetError();
                    throw Exception(str.str());
                }
                else {
                    std::stringstream str;
                    str << "Using SDL audio driver " << SDL_GetCurrentAudioDriver();
                    if (P->infoCallback) P->infoCallback(str.str(), P->uri);
                }
            }
        }

        audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &sdl, &have, 0);
        if (audioDeviceID == 0) {
            std::stringstream str;
            str << "SDL_OpenAudioDevice exception: " << SDL_GetError() << " audio will be disabled";
            if (P->infoCallback) P->infoCallback(str.str(), P->uri);
            else std::cout << str.str() << std::endl;
            return 1;
        }

        SDL_PauseAudioDevice(audioDeviceID, 0);
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Display init audio exception: " << e.what();
        if (P->errorCallback) P->errorCallback(str.str(), P->uri, false);
        else std::cout << str.str() << std::endl;
        ret = -1;
    }

    return ret;
}

void Display::AudioCallback(void* userdata, uint8_t* audio_buffer, int len)
{
    Display* d = (Display*)userdata;
    memset(audio_buffer, 0, len);
    uint8_t* temp = (uint8_t*)malloc(len);
    memset(temp, 0, len);
    Frame f;
    Player* player = (Player*)d->player;

    if (d->paused)
        return;

    try {
        while (len > 0) {
            if (!d->swr_ptr) {

                d->afq_in->pop_move(f);

                if (player->pyAudioCallback) f = player->pyAudioCallback(f, *player);

                if (f.isValid()) {

#if LIBAVCODEC_VERSION_MAJOR < 61
                    uint64_t channels = f.m_frame->channels;
#else
                    uint64_t channels = f.m_frame->ch_layout.nb_channels;
#endif

                    int nb_samples = f.m_frame->nb_samples;
                    int format = f.m_frame->format;
                    const uint8_t** data = (const uint8_t**)&f.m_frame->data[0];
                    int frame_buffer_size = av_samples_get_buffer_size(NULL, channels, nb_samples, d->sdl_sample_format, 0);

                    if (frame_buffer_size != d->swr_buffer_size) {
                        if (d->swr_buffer) delete[] d->swr_buffer;
                        d->swr_buffer = new uint8_t[frame_buffer_size];
                        d->swr_buffer_size = frame_buffer_size;
                    }

                    swr_convert(d->swr_ctx, &d->swr_buffer, nb_samples, data, nb_samples);

                    if (player->isCameraStream()) {
                        int64_t delay = f.m_rts - d->rtClock.stream_time();
                        if (delay > 0 && player->sync_audio) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        }
                    }
                    else {
                        d->rtClock.update(f.m_rts - player->reader->start_time());
                    }

                    if (player->progressCallback) {
                        if (!d->reader->has_video()) {
                            if (d->reader->duration()) {
                                float pct = (float)f.m_rts / (float)d->reader->duration();
                                int progress = (int)(1000 * pct);
                                if (progress != player->last_progress) {
                                    player->progressCallback(pct, player->uri);
                                    player->last_progress = progress;
                                }
                            }
                        }
                    }

                    if (d->afq_out)
                        d->afq_out->push_move(f);
                    else
                        f.invalidate();
                }
                else {
                    if (d->afq_out) d->afq_out->push_move(f);
                    SDL_PauseAudioDevice(d->audioDeviceID, true);
                    if (!d->vfq_in) {
                        player->running = false;
                    }
                    return;
                }
            }

            int swr_residual = d->swr_buffer_size - d->swr_ptr;
            int tmp_residual = d->audio_buffer_len - len;
            int mark = std::min(len, swr_residual);

            if (player->running) memcpy(temp + tmp_residual, d->swr_buffer + d->swr_ptr, mark);
            len -= mark;
            d->swr_ptr += mark;
            if (d->swr_ptr >= d->swr_buffer_size)
                d->swr_ptr = 0;
        }

        if (!d->mute)
            SDL_MixAudioFormat(audio_buffer, temp, d->sdl.format, d->audio_buffer_len, SDL_MIX_MAXVOLUME * d->volume);
    }
    catch (const QueueClosedException& e) { std::cout << "Display audio callback closed queue exception 1" << std::endl; }
    catch (const Exception& e) { 
        std::stringstream str;
        str << "Audio callback exception: " << e.what();
        if (player->infoCallback) player->infoCallback(str.str(), P->uri);
        else std::cout << str.str() << std::endl;
    }

    free(temp);
}

bool Display::isPaused()
{
    return paused;
}

void Display::togglePause()
{
    paused = !paused;
    rtClock.pause(paused);
}

void Display::toggleRecord()
{
    recording = !recording;
    reader->request_pipe_write = recording;
}

std::string Display::audioDeviceStatus() const
{
    std::stringstream str;
    bool output = false;
    int count = SDL_GetNumAudioDevices(output);
    for (int i = 0; i < count; i++)
        str << "audio device: " << i << " name: " << SDL_GetAudioDeviceName(i, output) << "\n";

    str << "selected audio device ID (+2): " << audioDeviceID << "\n";
    str << "CHANNELS  want: " << (int)sdl.channels << "\n          have: " << (int)have.channels << "\n";
    str << "FREQUENCY want: " << sdl.freq << "\n          have: " << have.freq << "\n";
    str << "SAMPLES   want: " << sdl.samples << "\n          have: " << have.samples << "\n";
    str << "FORMAT    want: " << sdlAudioFormatName(sdl.format) << "\n          have: " << sdlAudioFormatName(have.format) << "\n";
    str << "SIZE      want: " << sdl.size << "\n          have: " << have.size << "\n";
    return str.str();
}

const char* Display::sdlAudioFormatName(SDL_AudioFormat format) const 
{
    switch (format) {
    case AUDIO_S8:
        return "AUDIO_S8";
        break;
    case AUDIO_U8:
        return "AUDIO_U8";
        break;
    case AUDIO_S16:
        return "AUDIO_S16";
        break;
    case AUDIO_U16:
        return "AUDIO_U16";
        break;
    case AUDIO_S32:
        return "AUDIO_S32";
        break;
    case AUDIO_F32:
        return "AUDIO_F32";
        break;
    default:
        return "UNKNOWN";
    }
}

}
