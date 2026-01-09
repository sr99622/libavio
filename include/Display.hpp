/********************************************************************
* libavio/include/Display.hpp
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

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <SDL.h>

#include "Frame.hpp"
#include "Queue.hpp"
#include "Reader.hpp"
#include "Filter.hpp"
#include "Exception.hpp"

namespace avio {

class Display {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    Uint32 sdl_pixel_format = SDL_PIXELFORMAT_UNKNOWN;

    Reader* reader = nullptr;
    Queue<Frame>* frames = nullptr;
    Frame last_frame;
    bool one_shot = false;
    ExceptionChecker ex;
    
    std::function<void(const Frame& f, const std::string& uri)> renderCallback = nullptr;
    std::function<void(float progress, const std::string& uri)> progressCallback = nullptr;
    bool headless = false;

    Display(Reader* reader, Queue<Frame>* frames, bool headless) : reader(reader), frames(frames), headless(headless) {

        if (headless) return;
        
        if (SDL_Init(SDL_INIT_VIDEO)) error("SDL_Init");

        if (!reader->has_video()) {
            ex.ck((window = SDL_CreateWindow("Sample Video", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN)), "SDL_CreateWindow", SDL_GetError());
            return;
        }

        // the window is created without size information first so that it can grab the focus
        ex.ck((window = SDL_CreateWindow("Sample Video", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_SHOWN)), "SDL_CreateWindow", SDL_GetError());
        ex.ck((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)), "SDL_CreateRenderer", SDL_GetError());
    }

    ~Display() {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }

    void show_frame(const Frame& f) {
        try {
            if (renderCallback) renderCallback(f, reader->uri);
            if (progressCallback) progressCallback(progress(f.pts()), reader->uri);

            if (headless) return;

            // window size information is found from the first frame rather than the reader so that filters can be used to adjust size
            // the texture is likewise created using size and pixel format information from the first frame so filters can change format
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            if (!(width == f.width() && height == f.height())) {
                adjust_window(f.width(), f.height());
                if (texture) SDL_DestroyTexture(texture);
                sdl_pixel_format = get_sdl_pix_fmt((AVPixelFormat)f.format());
                ex.ck((texture = SDL_CreateTexture(renderer, sdl_pixel_format, SDL_TEXTUREACCESS_STREAMING, f.width(), f.height())), "SDL_CreateTexture", SDL_GetError());               
            }

            update_texture(f);

            if (SDL_RenderClear(renderer)) error("SDL_RenderClear");
            if (SDL_RenderCopy(renderer, texture, nullptr, nullptr)) error("SDL_RenderCopy");
            SDL_RenderPresent(renderer);
        }
        catch (const std::exception& e) {
            std::cout << "display error: " << e.what() << std::endl;
        }
    }

    int render() {
        if (!headless) poll();

        if (reader->terminated) {
            frames->clear();
            return 0;
        }

        if (!reader->has_video()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 1;
        }

        if (reader->paused && !one_shot) {
            show_frame(last_frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else {
            Frame f = frames->pop();

            if (f.is_null())
                return 0;

            if (reader->seek_pts != AV_NOPTS_VALUE)
                return 1;

            if (!reader->live_stream)
                wait(f.pts());

            show_frame(f);
            
            last_frame = std::move(f);
            one_shot = false;
        }
        return 1;
    }

    void wait(int64_t pts) {
        if (reader->has_audio()) {
            int64_t rts = reader->real_time(reader->video_stream_index, pts);
            int64_t diff = rts - reader->last_audio_rts;
            if (diff > 0 && diff < 1000)
                SDL_Delay(diff);
        }
        else {
            int64_t pts_diff = pts - last_frame.pts();
            int64_t diff = reader->real_time(reader->video_stream_index, pts_diff);
            if (diff > 0 && diff < 1000)
                SDL_Delay(diff);
        }
    }

    void poll() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                reader->terminate();
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        reader->terminate();
                        break;
                    case SDLK_r:
                        if (reader->live_stream)
                            reader->recording = !reader->recording;
                        break;
                    case SDLK_SPACE:
                        if (!reader->live_stream)
                            reader->paused = !reader->paused;
                        break;
                    case SDLK_LEFT:
                        if (!reader->closed && !reader->live_stream) {
                            reader->seek_pts = last_frame.pts() - 10 * av_q2d(av_inv_q(reader->video_time_base()));
                            if (reader->paused) {
                                reader->clear_callback(reader->player);
                                one_shot = true;
                            }
                        }
                        break;
                    case SDLK_RIGHT:
                        if (!reader->closed && !reader->live_stream) {
                            reader->seek_pts = last_frame.pts() + 10 * av_q2d(av_inv_q(reader->video_time_base()));
                            if (reader->paused) {
                                reader->clear_callback(reader->player);
                                one_shot = true;
                            }
                        }
                        break;
                }
            }
        }
    }

    void adjust_window(int width, int height) {
        SDL_SetWindowSize(window, width, height);
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        int x = (DM.w - width) / 2;
        int y = (DM.h - height) / 2;
        SDL_SetWindowPosition(window, x, y);                
    }

    Uint32 get_sdl_pix_fmt(AVPixelFormat fmt) {
        switch (fmt) {
            case AV_PIX_FMT_RGB24:
                return SDL_PIXELFORMAT_RGB24;
            break;
            case AV_PIX_FMT_YUV420P:
                return SDL_PIXELFORMAT_IYUV;
            break;
            case AV_PIX_FMT_NV12:
                return SDL_PIXELFORMAT_NV12;
            break;
            default:
                return SDL_PIXELFORMAT_UNKNOWN;
        }
    }

    void update_texture(const Frame& f) {
        switch (sdl_pixel_format) {
            case SDL_PIXELFORMAT_RGB24:
                if (SDL_UpdateTexture(texture, nullptr, 
                                    f.frame->data[0], f.frame->linesize[0])) error("SDL_UpdateTexture");
            break;
            case SDL_PIXELFORMAT_IYUV:
                if (SDL_UpdateYUVTexture(texture, nullptr,
                                    f.frame->data[0], f.frame->linesize[0], 
                                    f.frame->data[1], f.frame->linesize[1], 
                                    f.frame->data[2], f.frame->linesize[2])) error("SDL_UpdateYUVTexture");
            break;
            case SDL_PIXELFORMAT_NV12:
                if (SDL_UpdateNVTexture(texture, nullptr, 
                                    f.frame->data[0], f.frame->linesize[0],
                                    f.frame->data[1], f.frame->linesize[1])) error("SDL_UpdateNVTexture");
            break;
            default:
                throw std::runtime_error("texture update error: unknown pixel format");
        }
    }

    float progress(int64_t pts) {
        float pct = 0.0;
        int64_t duration = reader->duration();
        if (duration) pct = (float)reader->real_time(reader->video_stream_index, pts) / (float)duration;
        return pct;
    }

    void error(const std::string& msg) {
        std::stringstream str;
        str << msg << " : " << SDL_GetError();
        throw std::runtime_error(str.str());
    }
};

}

#endif // DISPLAY_HPP