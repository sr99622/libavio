import avio

#filename = "/Users/stephen/projects/wabash/assets/short.mp4"
filename = "/Users/stephen/Movies/news.mp4"
player = avio.Player(filename)
player.headless = False
player.live_stream = False
player.str_video_filter = "scale=1280:720"
player.play()