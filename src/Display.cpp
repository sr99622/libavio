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

#include <filesystem>
#include "Display.h"
#include "Player.h"

namespace avio 
{

void Display::init()
{
    if (P->audioFilter)
        initAudio(P->audioFilter->sample_rate(), P->audioFilter->sample_format(), P->audioFilter->channels(), P->audioFilter->channel_layout(), P->audioFilter->frame_size());
    else if (P->audioDecoder)
        initAudio(P->audioDecoder->sample_rate(), P->audioDecoder->sample_format(), P->audioDecoder->channels(), P->audioDecoder->channel_layout(), P->audioDecoder->frame_size());
}

Display::~Display()
{
    std::cout << "~Display" << std::endl;
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_LockAudioDevice(audioDeviceID);
        SDL_CloseAudioDevice(audioDeviceID);
    }
    if (swr_ctx) swr_free(&swr_ctx);
    if (swr_buffer) delete[] swr_buffer;
    if (texture)  SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    if (screen)   SDL_FreeSurface(screen);
    SDL_Quit();
}

int Display::initVideo(int width, int height, AVPixelFormat pix_fmt)
{
    int ret = 0;

    try {
        Uint32 sdl_format = 0;
        int w = width;
        int h = height;

        switch (pix_fmt) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            sdl_format = SDL_PIXELFORMAT_IYUV;
            break;
        case AV_PIX_FMT_RGB24:
            sdl_format = SDL_PIXELFORMAT_RGB24;
            break;
        case AV_PIX_FMT_RGBA:
            sdl_format = SDL_PIXELFORMAT_RGBA32;
            break;
        case AV_PIX_FMT_BGR24:
            sdl_format = SDL_PIXELFORMAT_BGR24;
            break;
        case AV_PIX_FMT_BGRA:
            sdl_format = SDL_PIXELFORMAT_BGRA32;
            break;
        default:
            const char* pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
            std::stringstream str;
            str << "unsupported pix fmt: " << (pix_fmt_name ? pix_fmt_name : std::to_string(pix_fmt));
            throw Exception(str.str());
        }

        if (!SDL_WasInit(SDL_INIT_VIDEO))
            if (SDL_Init(SDL_INIT_VIDEO)) throw Exception(std::string("SDL video init error: ") + SDL_GetError());

        if (!window) {
            const char* pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
            std::stringstream str;
            str << "initializing video display | width: " << width << " height: " << height
                << " pixel format: " << pix_fmt_name ? pix_fmt_name : "unknown pixel format";
            ex.msg(str.str());

            window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
            if (!window) throw Exception(std::string("SDL_CreateWindow") + SDL_GetError());

            if (fullscreen) SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

            renderer = SDL_CreateRenderer(window, -1, 0);
            if (!renderer) throw Exception(std::string("SDL_CreateRenderer") + SDL_GetError());

            SDL_RenderSetLogicalSize(renderer, w, h);

            texture = SDL_CreateTexture(renderer, sdl_format, SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!texture) throw Exception(std::string("SDL_CreateTexture") + SDL_GetError());

        }
        else {
            if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)) {
                int window_width;
                int window_height;
                SDL_GetWindowSize(window, &window_width, &window_height);
                if (!(window_width == w && window_height == h)) {
                    SDL_SetWindowSize(window, w, h);
                    SDL_DisplayMode DM;
                    SDL_GetCurrentDisplayMode(0, &DM);
                    auto Width = DM.w;
                    auto Height = DM.h;
                    int x = (Width - w) / 2;
                    int y = (Height - h) / 2;
                    SDL_SetWindowPosition(window, x, y);
                    if (texture)
                        SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, sdl_format, SDL_TEXTUREACCESS_STREAMING, w, h);
                }
            }
        }
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Display init video exception: " << e.what();
        if (errorCallback) errorCallback(str.str());
        ret = -1;
    }
    
    return ret;
}

void Display::videoPresentation()
{
    if (!f.isValid())
        return;

    if (f.m_frame->format == AV_PIX_FMT_YUV420P) {
        ex.ck(SDL_UpdateYUVTexture(texture, NULL,
            f.m_frame->data[0], f.m_frame->linesize[0],
            f.m_frame->data[1], f.m_frame->linesize[1],
            f.m_frame->data[2], f.m_frame->linesize[2]), 
            SDL_GetError());
    }
    else {
        ex.ck(SDL_UpdateTexture(texture, NULL, f.m_frame->data[0], f.m_frame->linesize[0]), SDL_GetError());
    }

    SDL_RenderClear(renderer);
    ex.ck(SDL_RenderCopy(renderer, texture, NULL, NULL), SDL_GetError());
    SDL_RenderPresent(renderer);
}

PlayState Display::getEvents(std::vector<SDL_Event>* events)
{
    PlayState state = PlayState::PLAY;
    SDL_Event event;
    while (SDL_PollEvent(&event))
        events->push_back(event);

    if (events->empty()) {
        SDL_Event user_event = { 0 };
        user_event.type = SDL_USEREVENT;
        events->push_back(user_event);
    }

    for (int i = 0; i < events->size(); i++) {
        SDL_Event event = events->at(i);
        if (event.type == SDL_QUIT)
            state = PlayState::QUIT;
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                state = PlayState::QUIT;
            }
            else if (event.key.keysym.sym == SDLK_SPACE) {
                state = PlayState::PAUSE;
            }
        }
    }
    return state;
}

bool Display::display()
{
    bool playing = true;

    while (true)
    {
        try {

            std::vector<SDL_Event> events;
            PlayState state = getEvents(&events);

            if (state == PlayState::QUIT) {
                P->running = false;
            }
            else if (state == PlayState::PAUSE) {
                togglePause();
            }

            if (paused) 
            {
                if (!P->running) {
                    playing = false;
                    break;
                }

                f = paused_frame;
                bool flag_out = true;
                
                while (reader->seeking() || state == PlayState::QUIT) {
                    if (afq_in) afq_in->clear();
                    if (vfq_in->size() > 0) vfq_in->pop_move(f);
                    if (f.isValid()) {
                        if (f.m_frame->pts == reader->seek_found_pts) {
                            reader->seek_found_pts = AV_NOPTS_VALUE;
                            flag_out = false;
                        }
                    }
                    else {
                        std::cout << "Display received eof" << std::endl;
                        playing = false;
                        break;
                    }
                }

                if (!renderCallback) {
                    videoPresentation();
                }

                //SDL_Delay(SDL_EVENT_LOOP_WAIT);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));

                if (flag_out)
                    break;

            }

            if (vfq_in) {
                vfq_in->pop_move(f);
                if (!f.isValid()) {
                    playing = false;
                    break;
                }
            }
            else {
                SDL_Delay(SDL_EVENT_LOOP_WAIT);
                f = Frame(640, 480, AV_PIX_FMT_YUV420P);
                f.m_rts = rtClock.stream_time();
            }

            paused_frame = f;

            SDL_Delay(rtClock.update(f.m_rts - reader->start_time()));

            if (renderCallback) {
                renderCallback(f);
            }
            else {
                ex.ck(initVideo(f.m_frame->width, f.m_frame->height, (AVPixelFormat)f.m_frame->format), "initVideo");
                videoPresentation();
            }
            reader->last_video_pts = f.m_frame->pts;

            if (progressCallback) {
                if (reader->duration()) {
                    float pct = (float)f.m_rts / (float)reader->duration();
                    progressCallback(pct);
                }
            }

            if (vfq_out) {
                vfq_out->push_move(f);
            }
            else {
                f.invalidate();
            }
        }
        catch (const Exception& e) {
            std::stringstream str;
            str << "Display exception: " << e.what();
            if (infoCallback) infoCallback(str.str());
        }
    }

    return playing;
}

int Display::initAudio(int stream_sample_rate, AVSampleFormat stream_sample_format, int stream_channels, uint64_t stream_channel_layout, int stream_nb_samples)
{
    int ret = 0;
    try {

        if (stream_nb_samples == 0) {
            int audio_frame_size = av_samples_get_buffer_size(NULL, stream_channels, 1, sdl_sample_format, 1);
            int bytes_per_second = av_samples_get_buffer_size(NULL, stream_channels, stream_sample_rate, sdl_sample_format, 1);
            stream_nb_samples = bytes_per_second * 0.02f / audio_frame_size;
        }

        if (!stream_channel_layout || stream_channels != av_get_channel_layout_nb_channels(stream_channel_layout)) {
            stream_channel_layout = av_get_default_channel_layout(stream_channels);
            stream_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
        }
        stream_channels = av_get_channel_layout_nb_channels(stream_channel_layout);

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
            std::cout << "ERROR: incompatible sample format: " << result << std::endl;
            std::cout << "supported formats: AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_U8" << std::endl;
        }

        audio_buffer_len = av_samples_get_buffer_size(NULL, sdl.channels, sdl.samples, sdl_sample_format, 1);

        ex.ck(swr_ctx = swr_alloc_set_opts(NULL, stream_channel_layout, sdl_sample_format, stream_sample_rate,
            stream_channel_layout, stream_sample_format, stream_sample_rate, 0, NULL), SASO);
        ex.ck(swr_init(swr_ctx), SI);

        if (!SDL_WasInit(SDL_INIT_AUDIO)) {
            if (SDL_Init(SDL_INIT_AUDIO))
                throw Exception(std::string("SDL audio init error: ") + SDL_GetError());
        }

        audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &sdl, &have, 0);
        if (audioDeviceID == 0) {
            throw Exception(std::string("SDL_OpenAudioDevice exception: ") + SDL_GetError());
        }

        SDL_PauseAudioDevice(audioDeviceID, 0);
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "Display init audio exception: " << e.what();
        if (errorCallback) errorCallback(str.str());
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
    int ptr = 0;

    if (d->paused)
        return;

    try {
        while (len > 0) {
            if (ptr < d->audio_buffer_len) {
                d->afq_in->pop_move(f);

                if (f.isValid()) {
                    uint64_t channels = f.m_frame->channels;
                    int nb_samples = f.m_frame->nb_samples;
                    const uint8_t** data = (const uint8_t**)&f.m_frame->data[0];
                    int frame_buffer_size = av_samples_get_buffer_size(NULL, channels, nb_samples, d->sdl_sample_format, 0);
                        
                    if (frame_buffer_size != d->swr_buffer_size) {
                        if (d->swr_buffer) delete[] d->swr_buffer;
                        d->swr_buffer = new uint8_t[frame_buffer_size];
                        d->swr_buffer_size = frame_buffer_size;
                    }

                    swr_convert(d->swr_ctx, &d->swr_buffer, nb_samples, data, nb_samples);
                    int mark = std::min(len, d->swr_buffer_size);
                    memcpy(temp + ptr, d->swr_buffer, mark);
                    ptr += mark;
                    len -= mark;
                }
                else {
                    std::cout << "audio callback revcd null eof" << std::endl;
                    SDL_PauseAudioDevice(d->audioDeviceID, true);
                    if (!d->vfq_in) {
                        len = -1;
                        d->audio_eof = true;
                        SDL_Event event;
                        event.type = SDL_QUIT;
                        SDL_PushEvent(&event);
                    }
                    return;
                }
            }

            if (!d->mute)
                SDL_MixAudioFormat(audio_buffer, temp, d->sdl.format, d->audio_buffer_len, SDL_MIX_MAXVOLUME * d->volume);

            d->rtClock.sync(f.m_rts); 
            d->reader->seek_found_pts = AV_NOPTS_VALUE;

            if (d->afq_out)
                d->afq_out->push_move(f);
            else
                f.invalidate();
        }
    }
    catch (const Exception& e) { 
        std::stringstream str;
        str << "Audio callback exception: " << e.what();
        if (d->infoCallback) d->infoCallback(str.str());
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
    /*
     Note: SDL does not support planar format audio

     AV_SAMPLE_FMT_NONE = -1,
     AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
     AV_SAMPLE_FMT_S16,         ///< signed 16 bits
     AV_SAMPLE_FMT_S32,         ///< signed 32 bits
     AV_SAMPLE_FMT_FLT,         ///< float
     AV_SAMPLE_FMT_DBL,         ///< double
    */

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

void Display::snapshot()
{
    std::cout << "Display::snapshot" << std::endl;

    AVCodecContext* codec_ctx = NULL;
    AVFormatContext* fmt_ctx = NULL;
    AVStream* stream = NULL;
    AVCodec* codec = NULL;

    try {
        const char* out_name = "filename.jpg";
        int width = f.m_frame->width;
        int height = f.m_frame->height;

        ex.ck(fmt_ctx = avformat_alloc_context(), AAC);
        fmt_ctx->oformat = av_guess_format("mjpeg", NULL, NULL);

        ex.ck(avio_open(&fmt_ctx->pb, out_name, AVIO_FLAG_READ_WRITE), AO);
        ex.ck(stream = avformat_new_stream(fmt_ctx, 0), ANS);

        AVCodecParameters *parameters = stream->codecpar;
        parameters->codec_id = fmt_ctx->oformat->video_codec;
        parameters->codec_type = AVMEDIA_TYPE_VIDEO;
        parameters->format = AV_PIX_FMT_YUVJ420P;
        parameters->width = width;
        parameters->height = height;

        ex.ck(codec = (AVCodec *)avcodec_find_encoder(stream->codecpar->codec_id), AFE);
        ex.ck(codec_ctx = avcodec_alloc_context3(codec), AAC3);
        ex.ck(avcodec_parameters_to_context(codec_ctx, stream->codecpar), APTC);
        codec_ctx->time_base = av_make_q(1, 25);
        ex.ck(avcodec_open2(codec_ctx, codec, NULL), AO2);
        ex.ck(avformat_write_header(fmt_ctx, NULL), AWH);

        AVPacket pkt;
        av_new_packet(&pkt, width * height * 3);

        ex.ck(avcodec_send_frame(codec_ctx, f.m_frame), ASF);
        ex.ck(avcodec_receive_packet(codec_ctx, &pkt), ARP);
        ex.ck(av_write_frame(fmt_ctx, &pkt), AWF);
        av_packet_unref(&pkt);
        ex.ck(av_write_trailer(fmt_ctx), AWT);
    }    
    catch (Exception& e) {
        std::cout << "Display::snapshot exception: " << e.what() << std::endl;
    }    

    if (codec_ctx) avcodec_close(codec_ctx);
    if (fmt_ctx) {
        avio_close(fmt_ctx->pb);
        avformat_free_context(fmt_ctx);
    }
}

}
