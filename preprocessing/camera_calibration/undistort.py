"""图像去畸变脚本。

默认读取 preprocessing/camera_calibration/outputs/result.npz 中的标定结果，
对 preprocessing/camera_calibration/data/validation_images/ 中的图片做去畸变，
并保存去畸变图片和左右对比图。
"""

import argparse
import json
from pathlib import Path

import cv2
import numpy as np


# =========================
# 1. 默认路径
# =========================

BASE_DIR = Path(__file__).resolve().parent
CALIBRATION_FILE = BASE_DIR / "outputs" / "result.npz"
INPUT_DIR = BASE_DIR / "data" / "validation_images"
OUTPUT_DIR = BASE_DIR / "outputs" / "undistort"


def read_images(image_dir):
    """读取目录中的常见图片文件。"""
    image_files = []
    for suffix in ("*.jpg", "*.jpeg", "*.png", "*.bmp"):
        image_files.extend(image_dir.glob(suffix))
    return sorted(image_files)


def load_camera_parameters(calibration_file):
    """从 npz 或 json 文件读取相机内参和畸变参数。"""
    if not calibration_file.exists():
        raise FileNotFoundError(f"没有找到标定结果文件：{calibration_file}")

    suffix = calibration_file.suffix.lower()
    if suffix == ".npz":
        data = np.load(calibration_file)
        image_size = None
        if "image_size" in data:
            image_size = tuple(int(v) for v in data["image_size"])
        return data["K"], data["dist"], image_size

    if suffix == ".json":
        payload = json.loads(calibration_file.read_text(encoding="utf-8"))
        K = np.array(payload["camera_matrix"], dtype=np.float64)
        dist = np.array(payload["distortion_coefficients"], dtype=np.float64).reshape(1, -1)
        image_size = tuple(payload["image_size"]) if "image_size" in payload else None
        return K, dist, image_size

    raise ValueError(f"不支持的相机参数文件格式：{calibration_file}")


def validate_image_size(calibration_size, image_size, image_name="输入图片"):
    """检查待处理图片尺寸是否与标定图片一致。"""
    if calibration_size is None:
        return

    # 不在这里缩放内参。尺寸不一致时直接报错，避免把错误参数悄悄用于图片。
    calibration_width, calibration_height = calibration_size
    image_width, image_height = image_size
    if calibration_width != image_width or calibration_height != image_height:
        raise ValueError(
            f"{image_name}尺寸为 {image_width} x {image_height}，"
            f"与标定图片尺寸 {calibration_width} x {calibration_height} 不一致。"
            "请使用同一相机、同一分辨率采集的图片，或重新标定相机。"
        )


def make_comparison(original, undistorted, max_width=1600):
    """把原图和去畸变图左右拼接，方便观察效果。"""
    height, width = original.shape[:2]

    # 两张图左右拼接后可能太大，所以这里按最大宽度缩小。
    scale = min(1.0, max_width / float(width * 2))
    if scale < 1.0:
        new_size = (int(width * scale), int(height * scale))
        original = cv2.resize(original, new_size, interpolation=cv2.INTER_AREA)
        undistorted = cv2.resize(undistorted, new_size, interpolation=cv2.INTER_AREA)

    comparison = np.hstack((original, undistorted))

    # 在对比图上写简单标签，方便直接观察左右两侧结果。
    cv2.putText(comparison, "Original", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2)
    cv2.putText(
        comparison,
        "Undistorted",
        (original.shape[1] + 20, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        1.0,
        (0, 255, 0),
        2,
    )

    return comparison


def undistort_image(image, K, dist):
    """对单张 OpenCV 图像去畸变，返回完整图和裁剪图。"""
    height, width = image.shape[:2]

    # 计算新的相机内参矩阵，同时得到有效图像区域 roi。
    new_K, roi = cv2.getOptimalNewCameraMatrix(K, dist, (width, height), 1, (width, height))

    # 根据 K 和 dist 去除镜头畸变。
    undistorted = cv2.undistort(image, K, dist, None, new_K)

    # 去畸变后边缘可能有黑边，所以根据 roi 保存一个裁剪版本。
    x, y, roi_width, roi_height = roi
    if roi_width > 0 and roi_height > 0:
        cropped = undistorted[y : y + roi_height, x : x + roi_width]
    else:
        cropped = undistorted

    return undistorted, cropped


def run_undistort_file(calibration_file, input_file, output_file, crop=False):
    """对单张图片去畸变。这个入口方便 Qt 后续用 QProcess 调用。"""
    K, dist, calibration_size = load_camera_parameters(calibration_file)
    image = cv2.imread(str(input_file))
    if image is None:
        raise RuntimeError(f"无法读取图片：{input_file}")

    height, width = image.shape[:2]
    # Qt 里点“去畸变”时也会走到这里，所以尺寸检查放在脚本层最稳。
    validate_image_size(calibration_size, (width, height), input_file.name)
    undistorted, cropped = undistort_image(image, K, dist)
    output_file.parent.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(output_file), cropped if crop else undistorted)
    print(f"去畸变结果已保存：{output_file}")


def run_undistort(calibration_file=CALIBRATION_FILE, input_dir=INPUT_DIR, output_dir=OUTPUT_DIR, show_preview=False):
    """批量执行去畸变，并保存结果。"""
    K, dist, calibration_size = load_camera_parameters(calibration_file)

    image_files = read_images(input_dir)
    if len(image_files) == 0:
        raise RuntimeError(f"没有找到待处理图片：{input_dir}")

    image_output_dir = output_dir / "images"
    comparison_output_dir = output_dir / "comparisons"
    image_output_dir.mkdir(parents=True, exist_ok=True)
    comparison_output_dir.mkdir(parents=True, exist_ok=True)

    success_count = 0

    for image_file in image_files:
        image = cv2.imread(str(image_file))
        if image is None:
            print(f"跳过无法读取的图片：{image_file}")
            continue

        height, width = image.shape[:2]
        validate_image_size(calibration_size, (width, height), image_file.name)
        undistorted, cropped = undistort_image(image, K, dist)
        comparison = make_comparison(image, undistorted)

        name = image_file.stem
        undistorted_path = image_output_dir / f"{name}_undistorted.jpg"
        cropped_path = image_output_dir / f"{name}_undistorted_cropped.jpg"
        comparison_path = comparison_output_dir / f"{name}_comparison.jpg"

        cv2.imwrite(str(undistorted_path), undistorted)
        cv2.imwrite(str(cropped_path), cropped)
        cv2.imwrite(str(comparison_path), comparison)

        success_count += 1
        print(f"处理完成：{image_file.name}")

        if show_preview:
            cv2.imshow("Undistortion comparison", comparison)
            key = cv2.waitKey(0) & 0xFF
            if key == ord("q"):
                break

    if show_preview:
        cv2.destroyAllWindows()

    print("\n===== 去畸变完成 =====")
    print(f"成功处理图片数：{success_count} / {len(image_files)}")
    print(f"去畸变图片目录：{image_output_dir}")
    print(f"对比图目录：{comparison_output_dir}")


def main():
    parser = argparse.ArgumentParser(description="图像去畸变脚本")
    parser.add_argument("--calibration", type=Path, default=CALIBRATION_FILE, help="标定结果文件")
    parser.add_argument("--input-dir", type=Path, default=INPUT_DIR, help="待去畸变图片目录")
    parser.add_argument("--output-dir", type=Path, default=OUTPUT_DIR, help="输出目录")
    parser.add_argument("--input-file", type=Path, help="单张待去畸变图片")
    parser.add_argument("--output-file", type=Path, help="单张去畸变图片保存路径")
    parser.add_argument("--crop", action="store_true", help="单张去畸变时保存裁剪版本")
    parser.add_argument("--preview", action="store_true", help="显示去畸变对比预览")
    args = parser.parse_args()

    if args.input_file or args.output_file:
        if not args.input_file or not args.output_file:
            raise ValueError("--input-file 和 --output-file 必须同时使用")
        run_undistort_file(args.calibration, args.input_file, args.output_file, args.crop)
    else:
        run_undistort(args.calibration, args.input_dir, args.output_dir, args.preview)


if __name__ == "__main__":
    main()
