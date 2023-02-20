import build.avio as avio
import platform

player = avio.Player()


try :
    if platform.system() == "Linux":
        reader = avio.Reader("/home/stephen/Videos/news.mp4")
    else:
        reader = avio.Reader("C:/Users/sr996/Videos/news.mp4")

    reader.set_video_out("vpq_reader")
    reader.set_audio_out("apq_reader")

    videoDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_VIDEO)
    videoDecoder.set_video_in(reader.video_out())
    videoDecoder.set_video_out("vfq_decoder")

    #videoFilter = avio.Filter(videoDecoder, "scale=1280x720,format=rgb24")
    videoFilter = avio.Filter(videoDecoder, "null")
    videoFilter.set_video_in(videoDecoder.video_out())
    videoFilter.set_video_out("vfq_filter")

    audioDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_AUDIO)
    audioDecoder.set_audio_in(reader.audio_out())
    audioDecoder.set_audio_out("afq_decoder")

    display = avio.Display(reader)
    display.set_video_in(videoFilter.video_out())
    display.set_audio_in(audioDecoder.audio_out())

    player.add_reader(reader)
    player.add_decoder(videoDecoder)
    player.add_filter(videoFilter)
    player.add_decoder(audioDecoder)
    player.add_display(display)

    player.run()
except Exception as e:
    print(e)