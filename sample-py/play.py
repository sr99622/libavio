import avio

process = avio.Process()

reader = avio.Reader("/home/stephen/Videos/news.mp4")
reader.set_video_out("vpq_reader")
reader.set_audio_out("apq_reader")

videoDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_VIDEO)
videoDecoder.set_video_in(reader.video_out())
videoDecoder.set_video_out("vfq_decoder")

videoFilter = avio.Filter(videoDecoder, "scale=1280x720,format=rgb24")
videoFilter.set_video_in(videoDecoder.video_out())
videoFilter.set_video_out("vfq_filter")

audioDecoder = avio.Decoder(reader, avio.AVMEDIA_TYPE_AUDIO)
audioDecoder.set_audio_in(reader.audio_out())
audioDecoder.set_audio_out("afq_decoder")

display = avio.Display(reader)
display.set_video_in(videoFilter.video_out())
display.set_audio_in(audioDecoder.audio_out())

process.add_reader(reader)
process.add_decoder(videoDecoder)
process.add_filter(videoFilter)
process.add_decoder(audioDecoder)
process.add_display(display)

process.run()