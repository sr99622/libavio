#include "avio.h"

int main(int argc, char** argv)
{
    std::cout << "hello world" << std::endl;

    avio::Process process;

    avio::Reader reader("/home/stephen/Videos/news.mp4");
    reader.set_video_out("vpq_reader");
    reader.set_audio_out("apq_reader");

    avio::Decoder videoDecoder(reader, AVMEDIA_TYPE_VIDEO, AV_HWDEVICE_TYPE_VAAPI);
    videoDecoder.set_video_in(reader.video_out());
    videoDecoder.set_video_out("vfq_decoder");

    avio::Filter videoFilter(videoDecoder, "scale=1280x720,format=rgb24");
    videoFilter.set_video_in(videoDecoder.video_out());
    videoFilter.set_video_out("vfq_filter");
    //videoFilter.show_frames = true;

    avio::Decoder audioDecoder(reader, AVMEDIA_TYPE_AUDIO);
    audioDecoder.set_audio_in(reader.audio_out());
    audioDecoder.set_audio_out("afq_decoder");

    avio::Display display(reader);
    display.set_video_in(videoFilter.video_out());
    //display.set_video_in(videoDecoder.video_out());
    display.set_audio_in(audioDecoder.audio_out());

    process.add_reader(reader);
    process.add_decoder(videoDecoder);
    process.add_filter(videoFilter);
    process.add_decoder(audioDecoder);
    process.add_display(display);

    process.run();

    return 0;
}