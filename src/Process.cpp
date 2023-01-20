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
        while (pkt_q->second->size() > 0) {
            AVPacket* tmp = pkt_q->second->pop();
            av_packet_free(&tmp);
        }
    }
    FRAME_Q_MAP::iterator frame_q;
    for (frame_q = frame_queues.begin(); frame_q != frame_queues.end(); ++frame_q) {
        while (frame_q->second->size() > 0) {
            Frame f;
            frame_q->second->pop(f);
        }
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

void Process::add_encoder(Encoder& encoder_in)
{
    if (encoder_in.mediaType == AVMEDIA_TYPE_VIDEO) {
        videoEncoder = &encoder_in;
        writer = videoEncoder->writer;
    }
    if (encoder_in.mediaType == AVMEDIA_TYPE_AUDIO) {
        audioEncoder = &encoder_in;
        writer = audioEncoder->writer;
    }
    pkt_q_names.push_back(encoder_in.pkt_q_name);
    frame_q_names.push_back(encoder_in.frame_q_name);
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

void Process::add_frame_drain(const std::string& frame_q_name)
{
    frame_q_drain_names.push_back(frame_q_name);
}

void Process::add_packet_drain(const std::string& pkt_q_name)
{
    pkt_q_drain_names.push_back(pkt_q_name);
}

void Process::cleanup()
{
    for (PKT_Q_MAP::iterator q = pkt_queues.begin(); q != pkt_queues.end(); ++q) {
        if (q->second) {
            while (q->second->size() > 0) {
                AVPacket* pkt = q->second->pop();
                av_packet_free(&pkt);
            }
            //q->second->close();
        }
    }

    for (FRAME_Q_MAP::iterator q = frame_queues.begin(); q != frame_queues.end(); ++q) {
        if (q->second) {
            while (q->second->size() > 0) {
                Frame f;
                q->second->pop(f);
            }
        //q->second->close();
        }
    }

    for (int i = 0; i < ops.size(); i++) {
        ops[i]->join();
        delete ops[i];
    }

    for (PKT_Q_MAP::iterator q = pkt_queues.begin(); q != pkt_queues.end(); ++q) {
        if (q->second)
            delete q->second;
    }

    for (FRAME_Q_MAP::iterator q = frame_queues.begin(); q != frame_queues.end(); ++q) {
        if (q->second)
            delete q->second;
    }
}

void Process::run()
{
    running = true;
    
    for (const std::string& name : pkt_q_names) {
        if (!name.empty()) {
            if (pkt_queues.find(name) == pkt_queues.end())
                pkt_queues.insert({ name, new Queue<AVPacket*>() });
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
        ops.push_back(new std::thread(decode, videoDecoder,
            pkt_queues[videoDecoder->pkt_q_name], frame_queues[videoDecoder->frame_q_name]));
    }

    if (videoFilter) {
        ops.push_back(new std::thread(filter, videoFilter,
            frame_queues[videoFilter->q_in_name], frame_queues[videoFilter->q_out_name]));
    }

    if (audioDecoder) {
        ops.push_back(new std::thread(decode, audioDecoder,
            pkt_queues[audioDecoder->pkt_q_name], frame_queues[audioDecoder->frame_q_name]));
    }

    if (audioFilter) {
        ops.push_back(new std::thread(filter, audioFilter,
            frame_queues[audioFilter->q_in_name], frame_queues[audioFilter->q_out_name]));
    }

    if (videoEncoder) {
        videoEncoder->pkt_q = pkt_queues[videoEncoder->pkt_q_name];
        videoEncoder->frame_q = frame_queues[videoEncoder->frame_q_name];
        if (videoEncoder->frame_q_max_size > 0) videoEncoder->frame_q->set_max_size(videoEncoder->frame_q_max_size);
        if (writer->enabled) videoEncoder->init();
        ops.push_back(new std::thread(write, videoEncoder->writer, videoEncoder));
    }

    if (audioEncoder) {
        audioEncoder->pkt_q = pkt_queues[audioEncoder->pkt_q_name];
        audioEncoder->frame_q = frame_queues[audioEncoder->frame_q_name];
        if (audioEncoder->frame_q_max_size > 0) audioEncoder->frame_q->set_max_size(audioEncoder->frame_q_max_size);
        if (writer->enabled) audioEncoder->init();
        ops.push_back(new std::thread(write, audioEncoder->writer, audioEncoder));
    }

    for (const std::string& name : frame_q_drain_names)
        ops.push_back(new std::thread(frame_drain, frame_queues[name]));

    for (const std::string& name : pkt_q_drain_names)
        ops.push_back(new std::thread(pkt_drain, pkt_queues[name]));

    if (display) {

        if (!display->vfq_in_name.empty()) display->vfq_in = frame_queues[display->vfq_in_name];
        if (!display->afq_in_name.empty()) display->afq_in = frame_queues[display->afq_in_name];

        if (!display->vfq_out_name.empty()) display->vfq_out = frame_queues[display->vfq_out_name];
        if (!display->afq_out_name.empty()) display->afq_out = frame_queues[display->afq_out_name];

        display->init();

        while (display->display()) {}

        std::cout << "display done" << std::endl;

        if (writer) {
            while (!display->audio_eof)
                SDL_Delay(1);
            writer->enabled = false;
        }

    }

    cleanup();
}



}