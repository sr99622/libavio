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
    //PKT_Q_MAP::iterator pkt_q;
    //for (pkt_q = pkt_queues.begin(); pkt_q != pkt_queues.end(); ++pkt_q) {
    //    pkt_q->second->clear();
    //}
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


/*
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
*/

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
    Queue<Packet> vpq_reader;
    Queue<Packet> apq_reader;
    reader.vpq = &vpq_reader;
    reader.apq = &apq_reader;

    //reader.set_video_out("vpq_reader");
    //reader.set_audio_out("apq_reader");
    //pkt_q_names.push_back(reader.vpq_name);
    //pkt_q_names.push_back(reader.apq_name);

    Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, hw_device_type);
    //videoDecoder.set_video_in(reader.video_out());
    videoDecoder.pkt_q = &vpq_reader;
    videoDecoder.set_video_out("vfq_decoder");
    //pkt_q_names.push_back(videoDecoder.pkt_q_name);
    frame_q_names.push_back(videoDecoder.frame_q_name);

    Filter videoFilter(videoDecoder, video_filter.c_str());
    videoFilter.set_video_in(videoDecoder.video_out());
    videoFilter.set_video_out("vfq_filter");
    frame_q_names.push_back(videoFilter.q_in_name);
    frame_q_names.push_back(videoFilter.q_out_name);

    Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
    //audioDecoder.set_audio_in(reader.audio_out());
    audioDecoder.pkt_q = &apq_reader;
    audioDecoder.set_audio_out("afq_decoder");
    //pkt_q_names.push_back(audioDecoder.pkt_q_name);
    frame_q_names.push_back(audioDecoder.frame_q_name);

    Filter audioFilter(audioDecoder, audio_filter.c_str());
    audioFilter.set_audio_in(audioDecoder.audio_out());
    audioFilter.set_audio_out("afq_filter");
    frame_q_names.push_back(audioFilter.q_in_name);
    frame_q_names.push_back(audioFilter.q_out_name);

    Display display(reader, audioFilter);
    display.set_video_in(videoFilter.video_out());
    display.set_audio_in(audioFilter.audio_out());
    display.player = this;
    
    //for (const std::string& name : pkt_q_names) {
    //    if (!name.empty()) {
    //        if (pkt_queues.find(name) == pkt_queues.end())
    //            pkt_queues.insert({ name, new Queue<Packet>() });
    //    }
    //}

    for (const std::string& name : frame_q_names) {
        if (!name.empty()) {
            if (frame_queues.find(name) == frame_queues.end())
                frame_queues.insert({ name, new Queue<Frame>() });
        }
    }

    ops.push_back(new std::thread(read, &reader, this));
    ops.push_back(new std::thread(decode, &videoDecoder, frame_queues[videoDecoder.frame_q_name]));
    ops.push_back(new std::thread(filter, &videoFilter, frame_queues[videoFilter.q_in_name], frame_queues[videoFilter.q_out_name]));
    ops.push_back(new std::thread(decode, &audioDecoder, frame_queues[audioDecoder.frame_q_name]));
    ops.push_back(new std::thread(filter, &audioFilter, frame_queues[audioFilter.q_in_name], frame_queues[audioFilter.q_out_name]));

    if (!display.vfq_in_name.empty()) display.vfq_in = frame_queues[display.vfq_in_name];
    if (!display.afq_in_name.empty()) display.afq_in = frame_queues[display.afq_in_name];

    if (!display.vfq_out_name.empty()) display.vfq_out = frame_queues[display.vfq_out_name];
    if (!display.afq_out_name.empty()) display.afq_out = frame_queues[display.afq_out_name];

    display.init();

    while (display.display()) {}
    running = false;

    std::cout << "display done" << std::endl;

    //}

    cleanup();

    //if (display) delete display;
}

}