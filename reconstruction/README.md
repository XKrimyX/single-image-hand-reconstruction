# 重建层

本目录用于接入 simpleHand / A Simple Baseline for Efficient Hand Mesh Reconstruction，并提供稳定的命令行入口供 Qt 调用。

## 当前命令

已实现单张手部区域图片推理入口：

```bash
/Users/krimy/miniconda3/envs/pytorch/bin/python reconstruction/simplehand_reconstruct.py \
  --image /path/to/input.jpg \
  --checkpoint reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1 \
  --output-dir outputs/mesh \
  --device auto
```

兼容入口 `reconstruction/reconstruct.py` 会转发到同一实现。

## 输入和预处理

- 输入图片应已经裁剪为单手区域，当前不做 bbox 检测。
- 使用 `cv2.imread()` 读取 BGR 图片。
- 预处理保持长宽比，先填充为正方形，再 resize 到 `224 x 224`。
- 输入 tensor 保持 `0-255 float32`，不在脚本中重复归一化；simpleHand 的 `HandNet.forward()` 内部会执行 `image / 255 - 0.5`。

## 输出

输出文件根据输入图片名和时间戳生成，避免覆盖。例如输入 `test_hand.jpg`：

```text
outputs/mesh/
├── test_hand_20260514_153012.obj
├── test_hand_20260514_153012.stl
└── test_hand_20260514_153012_result.json
```

`result.json` 包含：

- `success`、`error`
- `input_image`、`checkpoint`、`device`
- `obj_path`、`stl_path`、`result_json_path`
- `vertices_count`、`faces_count`
- `joints`、`uv`
- `preprocess`
- `timestamp`、`inference_time_sec`

命令成功返回 `0`；失败返回非 `0`，并尽量写出失败版 `result.json`，方便 Qt 通过 `QProcess` 捕获错误。

## simpleHand 资源

simpleHand 推理所需代码已经内置到：

- `reconstruction/simplehand_vendor/`

脚本默认使用项目内置目录，不再需要在 Qt 设置里选择外部 simpleHand 源码目录。

内置目录包含：

- `HandNet`
- `cfg.py`
- `models/modules.py`
- `models/losses.py`
- `models/mano_torch.py`
- `models/position_embedding.py`
- `models/MANO_RIGHT_C.pkl`
- `models/MANO_LEFT_C.pkl`

当前已下载作者发布的两个 checkpoint：

- `reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1`
- `reconstruction/checkpoints/simplehand_drive/epoch_200_rerun2`

下载来源为 simpleHand README 中的 Google Drive 文件夹。已停止并清理 FreiHAND 数据集 zip 的下载，本项目当前不需要训练数据。

## 依赖

建议使用当前已验证的 Python 环境：

```bash
/Users/krimy/miniconda3/envs/pytorch/bin/python
```

新增或确认的主要依赖：

- `torch`
- `opencv-python`
- `numpy`
- `timm==0.9.12`
- `hiera-transformer==0.1.2`
- `trimesh`

其中 `trimesh` 用于导出 STL；OBJ 当前由脚本直接写出。

## 当前限制

- 只实现单张图片推理闭环，不做训练、验证集评估或指标计算。
- 不依赖 FreiHAND `train.json` / `eval.json`。
- 当前不做手部检测，输入图片需要提前裁剪到单手区域。
- simpleHand 原代码主要面向 CUDA；本脚本支持 `device=auto/cuda/cpu`，CPU 属于尽力支持，推理速度会明显更慢。
