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
    PKT_Q_MAP::iterator pkt_q;
    for (pkt_q = pkt_queues.begin(); pkt_q != pkt_queues.end(); ++pkt_q) {
        pkt_q->second->clear();
    }
    FRAME_Q_MAP::iterator frame_q;
    for (frame_q = frame_queues.begin(); frame_q != frame_queues.end(); ++frame_q) {
        frame_q->second->clear();
    }
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

void Player::add_reader(Reader& reader_in)
{
    reader = &reader_in;
    
    if (!reader_in.vpq_name.empty()) pkt_q_names.push_back(reader_in.vpq_name);
    if (!reader_in.apq_name.empty()) pkt_q_names.push_back(reader_in.apq_name);
}

void Player::add_decoder(Decoder& decoder_in)
{
    if (decoder_in.mediaType == AVMEDIA_TYPE_VIDEO)
        videoDecoder = &decoder_in;
    if (decoder_in.mediaType == AVMEDIA_TYPE_AUDIO)
        audioDecoder = &decoder_in;

    pkt_q_names.push_back(decoder_in.pkt_q_name);
    frame_q_names.push_back(decoder_in.frame_q_name);
}

void Player::add_filter(Filter& filter_in)
{
    if (filter_in.mediaType() == AVMEDIA_TYPE_VIDEO)
        videoFilter = &filter_in;
    if (filter_in.mediaType() == AVMEDIA_TYPE_AUDIO)
        audioFilter = &filter_in;

    frame_q_names.push_back(filter_in.q_in_name);
    frame_q_names.push_back(filter_in.q_out_name);
}

void Player::add_display(Display& display_in)
{
    display_in.player = (void*)this;
    display = &display_in;

    if (!display->vfq_out_name.empty())
        frame_q_names.push_back(display->vfq_out_name);
    if (!display->afq_out_name.empty())
        frame_q_names.push_back(display->afq_out_name);
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
    
    for (const std::string& name : pkt_q_names) {
        if (!name.empty()) {
            if (pkt_queues.find(name) == pkt_queues.end())
                pkt_queues.insert({ name, new Queue<Packet>() });
        }
    }

    for (const std::string& name : frame_q_names) {
        if (!name.empty()) {
            if (frame_queues.find(name) == frame_queues.end())
                frame_queues.insert({ name, new Queue<Frame>() });
        }
    }

    if (reader) {
        ops.push_back(new std::thread(read, this));
    }

    if (videoDecoder) {
        ops.push_back(new std::thread(decode, this, AVMEDIA_TYPE_VIDEO));
    }

    if (videoFilter) {
        ops.push_back(new std::thread(filter, this, AVMEDIA_TYPE_VIDEO));
    }

    if (audioDecoder) {
        ops.push_back(new std::thread(decode, this, AVMEDIA_TYPE_AUDIO));
    }

    if (audioFilter) {
        ops.push_back(new std::thread(filter, this, AVMEDIA_TYPE_AUDIO));
    }

    if (display) {

        if (!display->vfq_in_name.empty()) display->vfq_in = frame_queues[display->vfq_in_name];
        if (!display->afq_in_name.empty()) display->afq_in = frame_queues[display->afq_in_name];

        if (!display->vfq_out_name.empty()) display->vfq_out = frame_queues[display->vfq_out_name];
        if (!display->afq_out_name.empty()) display->afq_out = frame_queues[display->afq_out_name];

        display->init();

        while (display->display()) {}
        running = false;

        std::cout << "display done" << std::endl;

    }

    cleanup();
}



}