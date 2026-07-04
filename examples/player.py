import avio
import cv2
import numpy as np
from argparse import ArgumentParser, Namespace

player = avio.Player()

#'''
def draw_progress_bar(
    image: np.ndarray,
    progress: float,
    x: int,
    y: int,
    width: int,
    height: int,
    color=(0, 255, 0),
    outline_color=(255, 255, 255),
    thickness: int = 2,
) -> np.ndarray:
    progress = max(0.0, min(1.0, progress))

    # Outline rectangle
    cv2.rectangle(
        image,
        (x, y),
        (x + width, y + height),
        outline_color,
        thickness,
    )

    # Filled progress region, inset so it stays inside outline
    inset = thickness
    inner_x = x + inset
    inner_y = y + inset
    inner_w = width - 2 * inset
    inner_h = height - 2 * inset

    fill_w = int(inner_w * progress)

    if fill_w > 0:
        cv2.rectangle(
            image,
            (inner_x, inner_y),
            (inner_x + fill_w, inner_y + inner_h),
            color,
            -1,
        )

    return image
#'''

def process_frame(frame: avio.Frame, uri: str):
    current_time = player.reader.real_time(player.reader.video_stream_index, frame.pts())
    duration = player.reader.duration()
    progress = current_time / duration

    img = np.array(frame, copy = False)

    #'''
    draw_progress_bar(img, progress, 10, 10, 200, 10)
    cv2.putText(
        img,
        f"{current_time}",
        (10, 50),                      # (x, y) position of the baseline
        cv2.FONT_HERSHEY_SIMPLEX,      # font
        0.5,                           # font scale
        (255, 255, 255),               # BGR color (green)
        2,                             # thickness
        cv2.LINE_AA                    # anti-aliased
    )
    #'''

def main(args: Namespace):
    filename = args.name
    #reader = avio.Reader(filename)
    #video_encoder = avio.Encoder()
    #player.reader = reader
    player.uri = filename
    player.output_filename = "test.mp4"
    player.add_video_encoder = True
    player.headless = False
    player.live_stream = False
    #player.disable_audio = True
    player.str_video_filter = "format=rgb24,scale=640:360"
    player.renderCallback = process_frame
    player.play()

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("name")
    args = parser.parse_args()
    print(args.name)
    main(args)

