# 相机标定预处理模块

本模块用于手部三维重建系统的预处理实验。

当前支持：

- 棋盘格相机标定；
- 保存可复用的相机内参和畸变系数；
- 批量图像去畸变；
- 生成去畸变前后对比图，用于论文和答辩演示。

## 运行环境

脚本需要安装 `opencv-python` 和 `numpy` 的 Python 环境。本机已验证环境为：

```bash
/Users/krimy/miniconda3/envs/pytorch/bin/python
```

## 目录结构

```text
preprocessing/camera_calibration/
├── calibrate.py
├── undistort.py
├── data/
│   ├── calibration_images/    # 棋盘格标定图片
│   └── validation_images/     # 去畸变验证图片
└── outputs/
    ├── result.npz             # Python 使用的完整标定结果
    ├── camera_params.json     # Qt/C++ 更容易读取的相机参数
    └── undistort/             # 去畸变输出
```

## 复现相机标定

标定图片存放在 `preprocessing/camera_calibration/data/calibration_images/`，当前图片尺寸为 1920 x 1080。

```bash
cd /Users/krimy/projects/i/thesis/GraProj
/Users/krimy/miniconda3/envs/pytorch/bin/python preprocessing/camera_calibration/calibrate.py
```

命令会写入：

```text
preprocessing/camera_calibration/outputs/result.npz
preprocessing/camera_calibration/outputs/camera_params.json
```

`result.npz` 包含 `K`、`dist`、`rvecs`、`tvecs` 和本次标定的元数据，适合 Python 脚本继续使用。

`camera_params.json` 包含相机内参、畸变系数、图像尺寸、RMS 重投影误差等信息，适合后续 Qt 应用启动时读取并显示“相机参数已加载”。

## 生成去畸变结果

验证图片存放在 `preprocessing/camera_calibration/data/validation_images/`，当前验证图片尺寸为 3024 x 4032。

```bash
cd /Users/krimy/projects/i/thesis/GraProj
/Users/krimy/miniconda3/envs/pytorch/bin/python preprocessing/camera_calibration/undistort.py
```

命令会写入：

```text
preprocessing/camera_calibration/outputs/undistort/images/
preprocessing/camera_calibration/outputs/undistort/comparisons/
```

其中 `comparisons` 目录保存左右拼接的原图和去畸变图，可直接用于论文和答辩演示。

## 单张图片去畸变

后续 Qt 应用可以通过 `QProcess` 调用单图模式：

```bash
cd /Users/krimy/projects/i/thesis/GraProj
/Users/krimy/miniconda3/envs/pytorch/bin/python preprocessing/camera_calibration/undistort.py \
  --calibration preprocessing/camera_calibration/outputs/camera_params.json \
  --input-file path/to/input.jpg \
  --output-file outputs/images/input_undistorted.jpg \
  --crop
```

注意：Qt 单张去畸变默认保存完整去畸变图，不使用 `--crop`。`--crop` 主要用于预处理实验或对比图；如果输入图尺寸和标定图尺寸不一致，脚本会自动按输入图尺寸缩放相机内参，避免 ROI 异常导致图像被裁成窄条。

`--calibration` 支持 `.npz` 和 `.json` 两种格式。
