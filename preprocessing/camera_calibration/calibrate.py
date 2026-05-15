"""相机标定脚本。

默认读取 preprocessing/camera_calibration/data/calibration_images/ 中的棋盘格图片，
计算相机内参 K 和畸变系数 dist，并保存到 preprocessing/camera_calibration/outputs/。
"""

import argparse
import json
from pathlib import Path

import cv2
import numpy as np


# =========================
# 1. 默认参数
# =========================

BASE_DIR = Path(__file__).resolve().parent
IMAGE_DIR = BASE_DIR / "data" / "calibration_images"
OUTPUT_DIR = BASE_DIR / "outputs"
NPZ_FILE = OUTPUT_DIR / "result.npz"
JSON_FILE = OUTPUT_DIR / "camera_params.json"

# 棋盘格“内角点”数量，不是格子数量。当前标定板为 8 x 8 个内角点。
CHECKERBOARD = (8, 8)

# 每个棋盘格的实际边长，单位是毫米。这个值用于建立真实世界坐标。
SQUARE_SIZE = 12.0


def read_images(image_dir):
    """读取目录中的常见图片文件。"""
    image_files = []
    for suffix in ("*.jpg", "*.jpeg", "*.png", "*.bmp"):
        image_files.extend(image_dir.glob(suffix))
    return sorted(image_files)


def create_world_points():
    """生成棋盘格角点在真实世界中的坐标。

    棋盘格是平面，所以 z 坐标都为 0。
    例如第一个点是 (0, 0, 0)，第二个点是 (12, 0, 0)。
    """
    points = np.zeros((CHECKERBOARD[0] * CHECKERBOARD[1], 3), np.float32)
    points[:, :2] = np.mgrid[0 : CHECKERBOARD[0], 0 : CHECKERBOARD[1]].T.reshape(-1, 2)
    points = points * SQUARE_SIZE
    return points


def save_camera_params_json(
    json_file,
    K,
    dist,
    image_size,
    rms_error,
    used_files,
    npz_file,
):
    """保存一份 Qt/C++ 更容易读取的 JSON 相机参数文件。"""
    payload = {
        "camera_matrix": K.tolist(),
        "distortion_coefficients": dist.reshape(-1).tolist(),
        "image_size": [int(image_size[0]), int(image_size[1])],
        "checkerboard_inner_corners": [int(CHECKERBOARD[0]), int(CHECKERBOARD[1])],
        "square_size_mm": float(SQUARE_SIZE),
        "rms_error": float(rms_error),
        "used_files": used_files,
        "npz_file": str(npz_file),
    }
    json_file.parent.mkdir(parents=True, exist_ok=True)
    json_file.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def run_calibration(
    image_dir=IMAGE_DIR,
    npz_file=NPZ_FILE,
    json_file=JSON_FILE,
    show_preview=False,
):
    """执行相机标定，并保存标定结果。"""
    image_files = read_images(image_dir)
    if len(image_files) == 0:
        raise RuntimeError(f"没有找到标定图片：{image_dir}")

    npz_file.parent.mkdir(parents=True, exist_ok=True)

    # obj_points 保存真实世界中的角点坐标。
    # img_points 保存图片中检测到的角点坐标。
    obj_points = []
    img_points = []
    used_files = []
    image_size = None
    world_points = create_world_points()

    # 亚像素优化条件：最多迭代 30 次，或者误差小于 0.001 时停止。
    criteria = (
        cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER,
        30,
        0.001,
    )

    for image_file in image_files:
        image = cv2.imread(str(image_file))
        if image is None:
            print(f"跳过无法读取的图片：{image_file}")
            continue

        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        image_size = gray.shape[::-1]  # OpenCV 标定需要格式为 (宽, 高)

        # 在灰度图中查找棋盘格内角点。
        found, corners = cv2.findChessboardCorners(gray, CHECKERBOARD)
        if not found:
            print(f"未检测到棋盘格角点：{image_file.name}")
            continue

        # 将角点位置优化到亚像素级，提高标定精度。
        better_corners = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)

        obj_points.append(world_points)
        img_points.append(better_corners)
        used_files.append(image_file.name)
        print(f"检测成功：{image_file.name}")

        # show_preview 只在需要人工检查角点时使用，默认关闭，便于批量运行。
        if show_preview:
            preview = image.copy()
            cv2.drawChessboardCorners(preview, CHECKERBOARD, better_corners, found)
            preview = cv2.resize(preview, None, fx=0.2, fy=0.2, interpolation=cv2.INTER_AREA)
            cv2.imshow("Calibration corners", preview)
            key = cv2.waitKey(0) & 0xFF
            if key == ord("q"):
                break

    if show_preview:
        cv2.destroyAllWindows()

    if len(obj_points) == 0 or image_size is None:
        raise RuntimeError("没有任何图片成功检测到棋盘格角点，无法标定。")

    # calibrateCamera 会根据真实世界角点和图像角点，计算相机参数。
    rms_error, K, dist, rvecs, tvecs = cv2.calibrateCamera(
        obj_points,
        img_points,
        image_size,
        None,
        None,
    )

    # 保存 npz 结果，后续 Python 脚本可以直接读取 K 和 dist 做去畸变。
    np.savez(
        npz_file,
        K=K,
        dist=dist,
        rvecs=rvecs,
        tvecs=tvecs,
        image_size=np.array(image_size),
        checkerboard=np.array(CHECKERBOARD),
        square_size_mm=np.array(SQUARE_SIZE),
        used_files=np.array(used_files),
        rms_error=np.array(rms_error),
    )
    save_camera_params_json(json_file, K, dist, image_size, rms_error, used_files, npz_file)

    print("\n===== 标定完成 =====")
    print(f"成功检测图片数：{len(obj_points)} / {len(image_files)}")
    print(f"图像尺寸：{image_size[0]} x {image_size[1]}")
    print("相机内参矩阵 K：")
    print(K)
    print("畸变系数 dist：")
    print(dist.ravel())
    print(f"RMS 重投影误差：{rms_error:.6f}")
    print(f"NPZ 结果已保存：{npz_file}")
    print(f"JSON 参数已保存：{json_file}")


def main():
    parser = argparse.ArgumentParser(description="相机标定脚本")
    parser.add_argument("--images", type=Path, default=IMAGE_DIR, help="标定图片目录")
    parser.add_argument("--output", type=Path, default=NPZ_FILE, help="NPZ 标定结果保存路径")
    parser.add_argument("--json-output", type=Path, default=JSON_FILE, help="JSON 相机参数保存路径")
    parser.add_argument("--preview", action="store_true", help="显示角点检测预览")
    args = parser.parse_args()

    run_calibration(args.images, args.output, args.json_output, args.preview)


if __name__ == "__main__":
    main()
