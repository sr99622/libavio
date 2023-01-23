#include "Process.h"
#include "avio.h"


namespace avio
{


bool Process::isPaused()
{
    bool result = false;
    if (display) result = display->paused;
    return result;
}

void Process::setMute(bool arg)
{
    if (display) display->mute = arg;
}

void Process::setVolume(int arg)
{
    if (display) display->volume = (float)arg / 100.0f;
}

void Process::togglePaused()
{
    if (display) display->togglePause();
}

void Process::seek(float arg)
{
    if (reader) reader->request_seek(arg);
}

void Process::toggle_pipe_out(const std::string& filename)
{
    if (reader) {
        reader->pipe_out_filename = filename;
        reader->request_pipe_write = !reader->request_pipe_write;
    }
}

void Process::clear_queues()
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

void Process::clear_decoders()
{
    if (videoDecoder) videoDecoder->flush();
    if (audioDecoder) audioDecoder->flush();
}

void Process::key_event(int keyCode)
{
    SDL_Event event;
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = keyCode;
    SDL_PushEvent(&event);
}

void Process::add_reader(Reader& reader_in)
{
    reader_in.process = (void*)this;
    reader = &reader_in;
    
    if (!reader_in.vpq_name.empty()) pkt_q_names.push_back(reader_in.vpq_name);
    if (!reader_in.apq_name.empty()) pkt_q_names.push_back(reader_in.apq_name);
}

void Process::add_decoder(Decoder& decoder_in)
{
    if (decoder_in.mediaType == AVMEDIA_TYPE_VIDEO)
        videoDecoder = &decoder_in;
    if (decoder_in.mediaType == AVMEDIA_TYPE_AUDIO)
        audioDecoder = &decoder_in;

    pkt_q_names.push_back(decoder_in.pkt_q_name);
    frame_q_names.push_back(decoder_in.frame_q_name);
}

void Process::add_filter(Filter& filter_in)
{
    if (filter_in.mediaType() == AVMEDIA_TYPE_VIDEO)
        videoFilter = &filter_in;
    if (filter_in.mediaType() == AVMEDIA_TYPE_AUDIO)
        audioFilter = &filter_in;

    frame_q_names.push_back(filter_in.q_in_name);
    frame_q_names.push_back(filter_in.q_out_name);
}

void Process::add_display(Display& display_in)
{
    display_in.process = (void*)this;
    display = &display_in;

    if (!display->vfq_out_name.empty())
        frame_q_names.push_back(display->vfq_out_name);
    if (!display->afq_out_name.empty())
        frame_q_names.push_back(display->afq_out_name);
}

void Process::cleanup()
{
    std::cout << "cleanup 2" << std::endl;
    for (int i = 0; i < ops.size(); i++) {
        ops[i]->join();
        delete ops[i];
    }

    std::cout << "cleanup 3" << std::endl;
    for (PKT_Q_MAP::iterator q = pkt_queues.begin(); q != pkt_queues.end(); ++q) {
        if (q->second)
            delete q->second;
    }

    std::cout << "cleanup 4" << std::endl;
    for (FRAME_Q_MAP::iterator q = frame_queues.begin(); q != frame_queues.end(); ++q) {
        if (q->second)
            delete q->second;
    }
    std::cout << "cleanup finish" << std::endl;
}

void Process::run()
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

        std::cout << "display done" << std::endl;

    }

    cleanup();
}



}