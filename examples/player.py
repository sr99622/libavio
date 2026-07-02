import avio
from argparse import ArgumentParser, Namespace

player = avio.Player()

def process_frame(frame: avio.Frame, uri: str):
    print(f"{frame.pts()} width: {frame.width()} height: {frame.height()}")
    vsi = player.reader.video_stream_index
    print(vsi)
    current_time = player.reader.real_time(vsi, frame.pts())
    print(f"{current_time}")
    duration = player.reader.duration()
    percentage_complete = 100 * (current_time / duration)
    print(percentage_complete)


def main(args: Namespace):
    filename = args.name
    reader = avio.Reader(filename)
    print(reader.uri)
    print(reader.get_stream_info())
    print(f"Duration: {reader.duration()}")
    tb = reader.video_time_base();
    print(f"TB: {tb.num} / {tb.den}")
    print(f"Video Timebase: {reader.video_time_base()}")
    player.reader = reader
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

