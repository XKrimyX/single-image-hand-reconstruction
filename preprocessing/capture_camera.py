"""调用摄像头拍摄手部图片。

运行后会打开摄像头预览窗口：
- 按空格或 Enter 保存当前画面；
- 按 q 或 Esc 退出程序。
"""

import argparse
from datetime import datetime
from pathlib import Path

import cv2


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = PROJECT_ROOT / "outputs" / "images" / "camera"
DEFAULT_PREVIEW_WIDTH = 960
DEFAULT_PREVIEW_HEIGHT = 540


def make_image_path(output_dir, prefix, image_format):
    """根据前缀和时间戳生成不会覆盖旧图片的文件名。"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    suffix = image_format.lower().lstrip(".")
    return output_dir / f"{prefix}_{timestamp}.{suffix}"


def resize_for_preview(frame, max_width, max_height):
    """把预览画面缩放到屏幕内容易查看的尺寸。"""
    height, width = frame.shape[:2]
    scale = min(max_width / float(width), max_height / float(height), 1.0)
    if scale >= 1.0:
        return frame

    new_size = (int(width * scale), int(height * scale))
    return cv2.resize(frame, new_size, interpolation=cv2.INTER_AREA)


def draw_help_text(frame, saved_count, last_saved_path):
    """在预览画面上显示按键提示和保存状态。"""
    preview = frame.copy()
    cv2.putText(
        preview,
        "Space/Enter: capture   q/Esc: quit",
        (20, 35),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2,
        cv2.LINE_AA,
    )
    cv2.putText(
        preview,
        f"Saved: {saved_count}",
        (20, 70),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2,
        cv2.LINE_AA,
    )
    if last_saved_path:
        cv2.putText(
            preview,
            Path(last_saved_path).name,
            (20, 105),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 255, 255),
            2,
            cv2.LINE_AA,
        )
    return preview


def open_camera(camera_index, width=None, height=None):
    """打开摄像头，并按需设置采集分辨率。"""
    cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        raise RuntimeError(f"无法打开摄像头：{camera_index}")

    if width:
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    if height:
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

    return cap


def capture_camera(
    camera_index=0,
    output_dir=DEFAULT_OUTPUT_DIR,
    prefix="camera",
    image_format="jpg",
    width=None,
    height=None,
    preview_width=DEFAULT_PREVIEW_WIDTH,
    preview_height=DEFAULT_PREVIEW_HEIGHT,
):
    """显示摄像头预览，并根据按键保存照片。"""
    output_dir.mkdir(parents=True, exist_ok=True)
    cap = open_camera(camera_index, width, height)

    window_name = "Camera Capture"
    saved_count = 0
    last_saved_path = ""

    print("摄像头已打开。")
    print("按空格或 Enter 保存照片，按 q 或 Esc 退出。")
    print(f"输出目录：{output_dir}")
    print(f"预览窗口最大尺寸：{preview_width} x {preview_height}，保存照片仍使用原始采集画面。")

    try:
        cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(window_name, preview_width, preview_height)

        while True:
            ok, frame = cap.read()
            if not ok or frame is None:
                raise RuntimeError("无法从摄像头读取画面")

            preview_frame = resize_for_preview(frame, preview_width, preview_height)
            preview = draw_help_text(preview_frame, saved_count, last_saved_path)
            cv2.imshow(window_name, preview)
            key = cv2.waitKey(1) & 0xFF

            if key in (ord("q"), 27):
                break

            if key in (ord(" "), 13):
                image_path = make_image_path(output_dir, prefix, image_format)
                if not cv2.imwrite(str(image_path), frame):
                    raise RuntimeError(f"保存照片失败：{image_path}")
                saved_count += 1
                last_saved_path = str(image_path)
                print(f"已保存：{image_path}")
    finally:
        cap.release()
        cv2.destroyAllWindows()

    print(f"采集结束，共保存 {saved_count} 张照片。")


def main():
    parser = argparse.ArgumentParser(description="调用摄像头拍摄图片")
    parser.add_argument("--camera", type=int, default=0, help="摄像头编号，默认 0")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="照片保存目录")
    parser.add_argument("--prefix", default="camera", help="输出文件名前缀")
    parser.add_argument("--format", choices=["jpg", "png", "bmp"], default="jpg", help="输出图片格式")
    parser.add_argument("--width", type=int, help="可选采集宽度，例如 1920")
    parser.add_argument("--height", type=int, help="可选采集高度，例如 1080")
    parser.add_argument("--preview-width", type=int, default=DEFAULT_PREVIEW_WIDTH, help="预览窗口最大宽度")
    parser.add_argument("--preview-height", type=int, default=DEFAULT_PREVIEW_HEIGHT, help="预览窗口最大高度")
    args = parser.parse_args()

    capture_camera(
        args.camera,
        args.output_dir,
        args.prefix,
        args.format,
        args.width,
        args.height,
        args.preview_width,
        args.preview_height,
    )


if __name__ == "__main__":
    main()
