import avio
from argparse import ArgumentParser, Namespace

def process_frame(frame: avio.Frame, uri: str):
    print(f"{frame.pts()} width: {frame.width()} height: {frame.height()}")

def main(args: Namespace):
    filename = args.name
    player = avio.Player(filename)
    player.headless = False
    player.live_stream = False
    #player.disable_audio = True
    player.str_video_filter = "scale=640:360"
    player.renderCallback = process_frame
    player.play()

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("name")
    args = parser.parse_args()
    print(args.name)
    main(args)

