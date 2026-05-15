# simpleHand 权重目录

本目录用于保存作者发布的 simpleHand 预训练权重。

当前已下载到：

- `simplehand_drive/epoch_200_rerun1`
- `simplehand_drive/epoch_200_rerun2`

来源：

- `https://drive.google.com/drive/folders/1BfHjNjxQj3MdsGoq5irCrOskyCA9a64l?usp=drive_link`

使用示例：

```bash
/Users/krimy/miniconda3/envs/pytorch/bin/python reconstruction/simplehand_reconstruct.py \
  --image preprocessing/camera_calibration/data/validation_images/9.jpg \
  --checkpoint reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1 \
  --output-dir outputs/mesh \
  --device auto
```

说明：

- 本项目只使用权重做单张图片推理，不下载 FreiHAND 训练数据，也不接入训练或验证集评估流程。
- 权重文件较大，后续如果用 Git 管理，建议不要直接提交到远程仓库。
