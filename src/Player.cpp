#include "Player.h"
#include "avio.h"

namespace avio
{

bool Player::isPaused()
{
    bool result = false;
    if (display) result = display->paused;
    return result;
}

bool Player::isPiping()
{
    bool result = false;
    if (reader) result = reader->request_pipe_write;
    return result;
}

void Player::setMute(bool arg)
{
    if (display) display->mute = arg;
}

void Player::setVolume(int arg)
{
    if (display) display->volume = (float)arg / 100.0f;
}

void Player::togglePaused()
{
    if (display) display->togglePause();
}

void Player::seek(float arg)
{
    if (reader) reader->request_seek(arg);
}

void Player::toggle_pipe_out(const std::string& filename)
{
    if (reader) {
        reader->pipe_out_filename = filename;
        reader->request_pipe_write = !reader->request_pipe_write;
    }
}

void Player::clear_queues()
{
    if (reader->vpq) reader->vpq->clear();
    if (reader->apq) reader->apq->clear();
    if (videoDecoder) videoDecoder->frame_q->clear();
    if (videoFilter) videoFilter->frame_out_q->clear();
    if (audioDecoder) audioDecoder->frame_q->clear();
    if (audioFilter) audioFilter->frame_out_q->clear();
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

void Player::hook(void* caller)
{
    Player* player = (Player*)caller;
    player->run();
}

void Player::start()
{
    std::thread process_thread(hook, this);
    process_thread.detach();
}

void Player::run()
{
    try {
        running = true;

        reader = new Reader(uri.c_str());
        reader->cbInfo = cbInfo;
        reader->cbError = cbError;

        if (reader->has_video()) {
            Queue<Packet> vpq_reader;
            reader->vpq = &vpq_reader;

            videoDecoder = new Decoder(*reader, AVMEDIA_TYPE_VIDEO, hw_device_type);
            videoDecoder->cbInfo = cbInfo;
            videoDecoder->cbError = cbError;
            videoDecoder->pkt_q = &vpq_reader;
            Queue<Frame> vfq_decoder;
            videoDecoder->frame_q = &vfq_decoder;

            videoFilter = new Filter(*videoDecoder, video_filter.c_str());
            videoFilter->cbInfo = cbInfo;
            videoFilter->cbError = cbError;
            videoFilter->frame_in_q = &vfq_decoder;
            Queue<Frame> vfq_filter;
            videoFilter->frame_out_q = &vfq_filter;
        }

        if (reader->has_audio()) {
            Queue<Packet> apq_reader;
            reader->apq = &apq_reader;

            audioDecoder = new Decoder(*reader, AVMEDIA_TYPE_AUDIO);
            audioDecoder->cbInfo = cbInfo;
            audioDecoder->cbError = cbError;
            audioDecoder->pkt_q = &apq_reader;
            Queue<Frame> afq_decoder;
            audioDecoder->frame_q = &afq_decoder;

            audioFilter = new Filter(*audioDecoder, audio_filter.c_str());
            audioFilter->cbInfo = cbInfo;
            audioFilter->cbError = cbError;
            audioFilter->frame_in_q = &afq_decoder;
            Queue<Frame> afq_filter;
            audioFilter->frame_out_q = &afq_filter;
        }

        ops.push_back(new std::thread(read, reader, this));
        if (videoDecoder) ops.push_back(new std::thread(decode, videoDecoder));
        if (videoFilter) ops.push_back(new std::thread(filter, videoFilter));
        if (audioDecoder) ops.push_back(new std::thread(decode, this->audioDecoder));
        if (audioFilter) ops.push_back(new std::thread(filter, this->audioFilter));

        display = new Display(*reader);
        display->cbInfo = cbInfo;
        display->cbError = cbError;
        if (videoFilter) display->vfq_in = videoFilter->frame_out_q;
        if (audioFilter) display->afq_in = audioFilter->frame_out_q;
        if (audioFilter) display->initAudio(audioFilter);
        display->player = this;

        if (cbMediaPlayingStarted) cbMediaPlayingStarted(reader->duration());
        while (display->display()) {}
        std::cout << "display done" << std::endl;
    }
    catch (const Exception& e) {
        std::stringstream str;
        str << "avio Player error: " << e.what();
        cbError(str.str());
    }

    running = false;

    if (reader) {
        if (reader->vpq) reader->vpq->close();
        if (reader->apq) reader->apq->close();
    }

    for (int i = 0; i < ops.size(); i++) {
        ops[i]->join();
        delete ops[i];
    }

    if (reader) delete reader;
    if (videoFilter) delete videoFilter;
    if (videoDecoder) delete videoDecoder;
    if (audioFilter) delete audioFilter;
    if (audioDecoder) delete audioDecoder;
    if (display) delete display;

    if (cbMediaPlayingStopped) cbMediaPlayingStopped();
}

}