import avio
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("name")
args = parser.parse_args()

print(args.name)

#filename = "/Users/stephen/projects/wabash/assets/short.mp4"
filename = args.name
player = avio.Player(filename)
player.headless = False
player.live_stream = False
player.str_video_filter = "scale=1280:720"
player.play()