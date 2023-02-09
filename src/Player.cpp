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
    //FRAME_Q_MAP::iterator frame_q;
    //for (frame_q = frame_queues.begin(); frame_q != frame_queues.end(); ++frame_q) {
    //    frame_q->second->clear();
    //}
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

void Player::cleanup()
{
    if (reader) {
        if (reader->vpq) reader->vpq->close();
        if (reader->apq) reader->apq->close();
    }

    for (int i = 0; i < ops.size(); i++) {
        ops[i]->join();
        delete ops[i];
    }
}

void Player::twink(void* caller)
{
    Player* player = (Player*)caller;
    player->run();
}

void Player::start()
{
    std::thread process_thread(twink, this);
    process_thread.detach();
}

void Player::run()
{
    running = true;

    Reader reader(uri.c_str());
    this->reader = &reader;

    //if (reader.has_video()) {
        Queue<Packet> vpq_reader;
        reader.vpq = &vpq_reader;

        Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, hw_device_type);
        videoDecoder.pkt_q = &vpq_reader;
        Queue<Frame> vfq_decoder;
        videoDecoder.frame_q = &vfq_decoder;
        this->videoDecoder = &videoDecoder;

        Filter videoFilter(videoDecoder, video_filter.c_str());
        videoFilter.frame_in_q = &vfq_decoder;
        Queue<Frame> vfq_filter;
        videoFilter.frame_out_q = &vfq_filter;
        this->videoFilter = &videoFilter;
    //}

    //if (reader.has_audio()) {
        Queue<Packet> apq_reader;
        reader.apq = &apq_reader;

        Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
        audioDecoder.pkt_q = &apq_reader;
        Queue<Frame> afq_decoder;
        audioDecoder.frame_q = &afq_decoder;
        this->audioDecoder = &audioDecoder;

        Filter audioFilter(audioDecoder, audio_filter.c_str());
        audioFilter.frame_in_q = &afq_decoder;
        Queue<Frame> afq_filter;
        audioFilter.frame_out_q = &afq_filter;
        this->audioFilter = &audioFilter;
    //}

    ops.push_back(new std::thread(read, this->reader, this));
    ops.push_back(new std::thread(decode, this->videoDecoder));
    ops.push_back(new std::thread(filter, this->videoFilter));
    ops.push_back(new std::thread(decode, this->audioDecoder));
    ops.push_back(new std::thread(filter, this->audioFilter));

    Display display(reader);
    display.vfq_in = this->videoFilter->frame_out_q;
    display.afq_in = this->audioFilter->frame_out_q;
    display.player = this;
    this->display = &display;
    display.initAudio(this->audioFilter);

    while (display.display()) {}
    running = false;

    std::cout << "display done" << std::endl;

    cleanup();

}

}