/********************************************************************
* libavio//src/Player.cpp
*
* Copyright (c) 2023  Stephen Rhodes
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

#include <algorithm>
#include <cmath>
#include "Player.h"
#include "avio.h"

namespace avio
{

Player::~Player()
{
    if (reader) delete reader;
}

bool Player::operator==(const Player& other) const
{
    bool result = false;

    if (!uri.empty() && !other.uri.empty()) {
        if (uri == other.uri)
            result = true;
    }

    return result;
}

bool Player::isPaused()
{
    bool result = false;
    if (display) result = display->paused;
    return result;
}

void Player::setMute(bool arg)
{
    mute = arg;
    if (display) display->mute = arg;
}

bool Player::isMuted()
{
    bool result = mute;
    if (display) result = display->mute;
    return result;
}

void Player::setVolume(int arg)
{
    volume = arg;
    if (display) display->volume = (float)arg / 100.0f;
}

int Player::getVolume()
{
    int result = volume;
    if (display) result = display->volume * 100;
    return result;
}

void Player::togglePaused()
{
    //std::cout << "---------------------toggle pause---------------";
    //if (isPaused()) std::cout << "PAUSED" << std::endl; else std::cout << " not paused " << std::endl;
    if (display) display->togglePause();
}

void Player::seek(float arg)
{
    if (reader) reader->request_seek(arg);
}

int64_t Player::pipeBytesWritten()
{
    int64_t result = 0;
    if (reader) result = reader->pipe_bytes_written;
    return result;
}

void Player::startFileBreakPipe(const std::string& filename)
{
    if (reader) reader->pipe_out_filename = filename;
    std::thread thread([&]() { fileBreakPipe(); });
    thread.detach();
}

void Player::fileBreakPipe()
{
    if (reader) {
        if (reader->request_pipe_write) {
            reader->request_pipe_write = false;
            while (reader->pipe) 
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            reader->request_pipe_write = true;
        }
    }
}

void Player::togglePiping(const std::string& filename)
{
    if (reader) {
        reader->pipe_out_filename = filename;
        reader->request_pipe_write = !reader->request_pipe_write;
    }
}

/*
void Player::toggleEncoding(const std::string& filename)
{
    if (writer) {
        writer->filename = filename;
        writer->enabled = !writer->enabled;
    }
}
*/

void Player::toggleRecording(const std::string& filename)
{
    togglePiping(filename);
    /*
    if (post_encode) {
        toggleEncoding(filename);
    }
    else {
        togglePiping(filename);
    }
    */
}

bool Player::isPiping()
{
    bool result = false;
    if (reader) result = reader->request_pipe_write;
    return result;
}

/*
bool Player::isEncoding()
{
    bool result = false;
    if (writer) result = writer->enabled;
    return result;
}
*/

bool Player::isRecording()
{
    //if (post_encode)
    //    return isEncoding();
    //else
        return isPiping();
}

void Player::closeReader()
{
    if (reader) {
        if (reader->fmt_ctx)
            avformat_close_input(&reader->fmt_ctx);
    }
}

void Player::clear_queues()
{
    if (reader->vpq)  reader->vpq->clear();
    if (reader->apq)  reader->apq->clear();
    if (videoDecoder) videoDecoder->frame_q->clear();
    if (videoFilter)  videoFilter->frame_out_q->clear();
    if (audioDecoder) audioDecoder->frame_q->clear();
    if (audioFilter)  audioFilter->frame_out_q->clear();
}

void Player::clear_decoders()
{
    if (videoDecoder) videoDecoder->flush();
    if (audioDecoder) audioDecoder->flush();
}

void Player::key_event(int keyCode)
{
    SDL_Event event;
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = keyCode;
    SDL_PushEvent(&event);
}

bool Player::isCameraStream()
{
    std::string tmp = uri;

    if (tmp.rfind("rtsp://", 0) == 0)
        return true;
    if (tmp.rfind("http://", 0) == 0)
        return true;
    if (tmp.rfind("https://", 0) == 0)
        return true;
    if (tmp.rfind("RTSP://", 0) == 0)
        return true;
    if (tmp.rfind("HTTP://", 0) == 0)
        return true;
    if (tmp.rfind("HTTPS://", 0) == 0)
        return true;

    return false;
}

void Player::setMetaData(const std::string& key, const std::string& value)
{
    metadata[key] = value;
}

std::string Player::getVideoCodec() const {
    std::string result = "Unknown codec";
    if (reader)
        result = reader->str_video_codec();
    return result;
}

std::string Player::pipeOutFilename() const {
    std::string result = "";
    if (reader) {
        result = reader->pipe_out_filename;
    }
    return result;
}

int Player::getVideoWidth() 
{
    int result = 0;
    if (reader)
        result = reader->width();
    return result;
}

int Player::getVideoHeight() 
{
    int result = 0;
    if (reader)
        result = reader->height();
    return result;
}

int Player::getVideoFrameRate() 
{
    int result = 0;
    if (reader)
        result = (int)av_q2d(reader->frame_rate());
    return result;
}

int Player::getVideoBitrate() 
{
    int result = 0;
    if (reader)
        result = reader->video_bit_rate();
    return result;
}

bool Player::hasAudio() 
{
    bool result = false;
    if (reader)
        result = reader->has_audio();
    return result;
}

bool Player::hasVideo()
{
    bool result = false;
    if (reader)
        result = reader->has_video();
    return result;
}

std::string Player::getAudioCodec() const {
    std::string result = "unknown";
    if (reader)
        result = reader->str_audio_codec();
    return result;
}

AudioEncoding Player::getAudioEncoding() const {
    AudioEncoding audio_encoding = AudioEncoding::NONE;
    if (reader) {
        if (!strcmp(reader->str_audio_codec(), "aac")) audio_encoding = AudioEncoding::AAC;
        if (!strcmp(reader->str_audio_codec(), "pcm_mulaw") || !strcmp(reader->str_audio_codec(), "pcm_alaw")) audio_encoding = AudioEncoding::G711;
        if (!strcmp(reader->str_audio_codec(), "adpcm_g726le")) audio_encoding = AudioEncoding::G726;
    }
    return audio_encoding;
}

std::string Player::getAudioChannelLayout() const {
    std::string result = "unknown";
    if (reader)
        result = reader->str_channel_layout();
    return result;
}

int Player::getAudioBitrate() {
    int result = 0;
    if (reader)
        result = reader->audio_bit_rate();
    return result;
}

int Player::getAudioSampleRate() {
    int result = 0;
    if (reader)
        result = reader->sample_rate();
    return result;
}

int Player::getAudioFrameSize() {
    int result = 0;
    if (reader)
        result = reader->frame_size();
    return result;
}

int Player::getCacheSize() 
{
    int result = -1;
    if (reader)
        result = reader->vpq->size();
    return result;
}

void Player::clearCache()
{
    if (reader) {
        if (reader->vpq)
            reader->vpq->clear();
        if (reader->apq)
            reader->apq->clear();
    }
}

std::string Player::getStreamInfo() const
{
    std::string result("");
    if (reader) 
        result = reader->getStreamInfo();
    return result;
}

std::string Player::getFFMPEGVersions() const
{
    std::stringstream str;
    str << LIBAVCODEC_IDENT << " " 
        << LIBAVFILTER_IDENT << " "
        << LIBAVFORMAT_IDENT << " "
        << LIBAVUTIL_IDENT << " "
        << LIBSWSCALE_IDENT;
    return str.str();
}

std::vector<std::string> Player::getAudioDrivers() const
{
    std::vector<std::string> result;
    int numDrivers = SDL_GetNumAudioDrivers();
    for (int i = 0; i < numDrivers; i++) {
        result.push_back(SDL_GetAudioDriver(i));
    }
    return result;
}

std::vector<std::string> Player::getVideoDrivers() const
{
    std::vector<std::string> result;
    int numDrivers = SDL_GetNumVideoDrivers();
    for (int i = 0; i < numDrivers; i++) {
        result.push_back(SDL_GetVideoDriver(i));
    }
    return result;
}

void Player::doit()
{
    std::cout << "doit" << std::endl;
    try {
        reader = new Reader(uri.c_str(), this);
        reader->show_video_pkts = true;
        read(reader);
        delete reader;
        reader = nullptr;
    }
    catch (const std::exception& ex) {
        std::cout << "exception: " << ex.what() << std::endl;
    }
}

void Player::start()
{
    std::thread process_thread([&]() { run(); });
    process_thread.detach();
}

void Player::run()
{
    try {
        running = true;
        stopped = false;

        vpq_reader  = new Queue<Packet>;
        vfq_decoder = new Queue<Frame>;
        vfq_filter  = new Queue<Frame>;
        vfq_display = new Queue<Frame>;
        apq_reader  = new Queue<Packet>;
        afq_decoder = new Queue<Frame>;
        afq_filter  = new Queue<Frame>;
        afq_display = new Queue<Frame>;


        reader = new Reader(uri.c_str(), this);
        duration = reader->duration();

        if (file_start_from_seek > 0.0) 
            reader->request_seek(file_start_from_seek);

        if (disable_video) {
            if(infoCallback) infoCallback("player video disabled", uri);
        }

        if (disable_audio) {
            if (infoCallback) infoCallback("player audio disabled", uri);
        }

        //writer = new Writer("mp4");
        //writer->infoCallback = infoCallback;

        if (reader->has_video() && !disable_video) {
            const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(reader->pix_fmt());
            if (!desc) throw avio::Exception("No pixel format in video stream");

            reader->vpq = vpq_reader;

            videoDecoder = new Decoder(reader, AVMEDIA_TYPE_VIDEO, hw_device_type);
            videoDecoder->pkt_q = vpq_reader;
            videoDecoder->frame_q = vfq_decoder;
            videoDecoder->errorCallback = errorCallback;
            videoDecoder->infoCallback = infoCallback;

            videoFilter = new Filter(*videoDecoder, video_filter.c_str());
            videoFilter->frame_in_q = vfq_decoder;
            videoFilter->frame_out_q = vfq_filter;
            videoFilter->errorCallback = errorCallback;
            videoFilter->infoCallback = infoCallback;

            /*
            if (post_encode) {
                
                videoEncoder = new Encoder(writer, AVMEDIA_TYPE_VIDEO);
                videoEncoder->frame_q = vfq_display;
                videoEncoder->width = videoFilter->width();
                videoEncoder->height = videoFilter->height();
                videoEncoder->frame_rate = reader->frame_rate();
                if (adjust_time_base)
                    videoEncoder->video_time_base = av_make_q(round(av_q2d(videoEncoder->frame_rate)), 1);
                else
                    videoEncoder->video_time_base = reader->video_time_base();
                videoEncoder->video_bit_rate = reader->video_bit_rate();

                videoEncoder->gop_size = 30;
                videoEncoder->profile = "high";
                videoEncoder->pix_fmt = AV_PIX_FMT_YUV420P;

                if (hw_encoding) {
                    videoEncoder->hw_video_codec_name = "h264_nvenc";
                    videoEncoder->hw_device_type = AV_HWDEVICE_TYPE_CUDA;
                    videoEncoder->hw_pix_fmt = AV_PIX_FMT_CUDA;
                    videoEncoder->sw_pix_fmt = AV_PIX_FMT_YUV420P;
                }

                videoEncoder->infoCallback = infoCallback;
            }
            */
        }

        if (reader->has_audio() && !disable_audio) {
            reader->apq = apq_reader;

            audioDecoder = new Decoder(reader, AVMEDIA_TYPE_AUDIO);
            audioDecoder->pkt_q = apq_reader;
            audioDecoder->frame_q = afq_decoder;
            audioDecoder->errorCallback = errorCallback;
            audioDecoder->infoCallback = infoCallback;

            audioFilter = new Filter(*audioDecoder, audio_filter.c_str());
            audioFilter->frame_in_q = afq_decoder;
            audioFilter->frame_out_q = afq_filter;
            audioFilter->errorCallback = errorCallback;
            audioFilter->infoCallback = infoCallback;

            /*
            if (post_encode) {
                audioEncoder = new Encoder(writer, AVMEDIA_TYPE_AUDIO);
                audioEncoder->frame_q = afq_display;
                audioEncoder->sample_fmt = AV_SAMPLE_FMT_FLTP;
                audioEncoder->audio_bit_rate = reader->audio_bit_rate();
                audioEncoder->sample_rate = reader->sample_rate();
                audioEncoder->audio_time_base = reader->audio_time_base();
                audioEncoder->nb_samples = reader->frame_size();

#if LIBAVCODEC_VERSION_MAJOR < 61
                audioEncoder->channels = reader->channels();
                if (audioEncoder->channels = 1)
                    audioEncoder->set_channel_layout_mono();
                else 
                    audioEncoder->set_channel_layout_stereo();
#else
                AVChannelLayout cl = reader->fmt_ctx->streams[reader->audio_stream_index]->codecpar->ch_layout;
                av_channel_layout_copy(&audioEncoder->ch_layout, &cl);
#endif
                
                audioEncoder->infoCallback = infoCallback;
            }
           */
        }

        if (isCameraStream()) {
            if (vpq_size) reader->apq_max_size = vpq_size;
            if (apq_size) reader->vpq_max_size = vpq_size;
        }

        display = new Display(reader, this);
        if (videoFilter) {
            display->vfq_in = videoFilter->frame_out_q;
            if (videoEncoder)
                display->vfq_out = vfq_display;
        }
        if (audioFilter) {
            display->afq_in = audioFilter->frame_out_q;
            if (!hidden) {
                if (display->initAudio(audioFilter)) {
                    disable_audio = true;
                    delete audioDecoder;
                    delete audioFilter;
                    audioDecoder = nullptr;
                    audioFilter = nullptr;
                }
            }
            if (audioEncoder)
                display->afq_out = afq_display;
        }
        display->mute = mute;
        display->volume = (float)volume / 100.0f;
        display->hWnd = hWnd;

        reader_thread = new std::thread(read, reader);
        if (videoDecoder) video_decoder_thread = new std::thread(decode, videoDecoder);
        if (videoFilter)  video_filter_thread  = new std::thread(filter, videoFilter);
        //if (videoEncoder) video_encoder_thread = new std::thread(write, writer, videoEncoder);
        if (audioDecoder) audio_decoder_thread = new std::thread(decode, audioDecoder);
        if (audioFilter)  audio_filter_thread  = new std::thread(filter, audioFilter);
        //if (audioEncoder) audio_encoder_thread = new std::thread(write, writer, audioEncoder);

        if (mediaPlayingStarted) mediaPlayingStarted(uri);

        display_thread = new std::thread(Display::display, this);

    }
    catch (const std::exception& e) {
        if (errorCallback)
            errorCallback(e.what(), uri, request_reconnect);
        else 
            std::cout << e.what() << std::endl;
    }

    if (display_thread) display_thread->join();

    if (isRecording()) 
        toggleRecording("");

    running = false;

    if (reader) {
        if (reader->vpq) reader->vpq->close();
        if (reader->apq) reader->apq->close();
    }

    if (reader_thread)        reader_thread->join();
    if (video_decoder_thread) video_decoder_thread->join();
    if (video_filter_thread)  video_filter_thread->join();
    //if (video_encoder_thread) video_encoder_thread->join();
    if (audio_decoder_thread) audio_decoder_thread->join();
    if (audio_filter_thread)  audio_filter_thread->join();
    //if (audio_encoder_thread) audio_encoder_thread->join();

    //if (writer)       delete writer;
    if (videoFilter)  delete videoFilter;
    if (videoDecoder) delete videoDecoder;
    //if (videoEncoder) delete videoEncoder;
    if (audioFilter)  delete audioFilter;
    if (audioDecoder) delete audioDecoder;
    //if (audioEncoder) delete audioEncoder;
    if (display)      delete display;

    if (reader_thread)        delete reader_thread;
    if (video_decoder_thread) delete video_decoder_thread;
    if (video_filter_thread)  delete video_filter_thread;
    //if (video_encoder_thread) delete video_encoder_thread;
    if (audio_decoder_thread) delete audio_decoder_thread;
    if (audio_filter_thread)  delete audio_filter_thread;
    //if (audio_encoder_thread) delete audio_encoder_thread;
    if (display_thread)       delete display_thread;

    if (vpq_reader)  delete  vpq_reader;  
    if (vfq_decoder) delete  vfq_decoder;
    if (vfq_filter)  delete  vfq_filter;
    if (vfq_display) delete  vfq_display;
    if (apq_reader)  delete  apq_reader;
    if (afq_decoder) delete  afq_decoder;
    if (afq_filter)  delete  afq_filter;
    if (afq_display) delete  afq_display;
    
    std::thread thread([&]() { signalStopped(); });
    thread.detach();
    stopped = true;
}

void Player::signalStopped()
{
    if (mediaPlayingStopped) mediaPlayingStopped(uri);
}

}