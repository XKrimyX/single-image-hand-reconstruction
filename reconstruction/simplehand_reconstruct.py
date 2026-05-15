#!/usr/bin/env python3
"""simpleHand 单张图片重建命令行入口。

输入一张已经裁剪到单手区域的图片，输出 OBJ、STL 和 JSON 结果文件。
"""

import argparse
import json
import os
import pickle
import sys
import time
import traceback
from datetime import datetime
from pathlib import Path

import cv2
import numpy as np


DEFAULT_SIMPLEHAND_ROOT = str(Path(__file__).resolve().parent / "simplehand_vendor")
DEFAULT_INPUT_SIZE = 224


def parse_args():
    parser = argparse.ArgumentParser(description="使用 simpleHand 对单张手部图片进行三维网格重建")
    parser.add_argument("--image", required=True, help="输入手部图片路径，要求图片已经是单手区域")
    parser.add_argument("--checkpoint", required=True, help="simpleHand checkpoint 路径，例如 epoch_200_rerun1")
    parser.add_argument("--output-dir", default="outputs/mesh", help="输出目录")
    parser.add_argument("--device", choices=["auto", "cuda", "cpu"], default="auto", help="推理设备")
    parser.add_argument("--simplehand-root", default=DEFAULT_SIMPLEHAND_ROOT, help="simpleHand 源码目录，默认使用项目内置 simplehand_vendor")
    parser.add_argument("--mano-pkl", default="", help="可选 MANO_RIGHT_C.pkl 路径，默认使用 simpleHand/models/MANO_RIGHT_C.pkl")
    parser.add_argument("--input-size", type=int, default=DEFAULT_INPUT_SIZE, help="模型输入尺寸，默认 224")
    return parser.parse_args()


def make_output_paths(image_path, output_dir):
    stem = Path(image_path).stem
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = Path(output_dir).expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    base = output_dir / f"{stem}_{timestamp}"
    return {
        "timestamp": timestamp,
        "obj_path": str(base.with_suffix(".obj")),
        "stl_path": str(base.with_suffix(".stl")),
        "result_json_path": str(base.parent / f"{base.name}_result.json"),
    }


def write_result_json(path, result):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)


def resolve_device(device_name):
    import torch

    if device_name == "auto":
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if device_name == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("指定了 device=cuda，但当前 PyTorch 环境不可用 CUDA")
    return torch.device(device_name)


def letterbox_image(image, input_size):
    height, width = image.shape[:2]
    side = max(height, width)
    pad_top = (side - height) // 2
    pad_bottom = side - height - pad_top
    pad_left = (side - width) // 2
    pad_right = side - width - pad_left

    padded = cv2.copyMakeBorder(
        image,
        pad_top,
        pad_bottom,
        pad_left,
        pad_right,
        cv2.BORDER_CONSTANT,
        value=(0, 0, 0),
    )
    resized = cv2.resize(padded, (input_size, input_size), interpolation=cv2.INTER_LINEAR)
    preprocess_info = {
        "original_width": int(width),
        "original_height": int(height),
        "letterbox": True,
        "square_size": int(side),
        "pad_top": int(pad_top),
        "pad_bottom": int(pad_bottom),
        "pad_left": int(pad_left),
        "pad_right": int(pad_right),
        "input_size": int(input_size),
        "color_order": "BGR",
        "tensor_value_range": "0-255 float32",
    }
    return resized, preprocess_info


def load_image_tensor(image_path, input_size, device):
    import torch

    image = cv2.imread(str(image_path), cv2.IMREAD_COLOR)
    if image is None:
        raise FileNotFoundError(f"无法读取输入图片：{image_path}")

    resized, preprocess_info = letterbox_image(image, input_size)
    tensor = resized.astype(np.float32).transpose(2, 0, 1)
    tensor = torch.from_numpy(tensor).unsqueeze(0).to(device)
    return tensor, preprocess_info


def prepare_simplehand_import(simplehand_root):
    simplehand_root = Path(simplehand_root).expanduser().resolve()
    if not simplehand_root.exists():
        raise FileNotFoundError(f"simpleHand 源码目录不存在：{simplehand_root}")
    required_files = ["cfg.py", "hand_net.py", "models/modules.py", "models/losses.py", "models/mano_torch.py"]
    missing = [name for name in required_files if not (simplehand_root / name).exists()]
    if missing:
        raise FileNotFoundError(f"simpleHand 源码目录缺少文件：{', '.join(missing)}")

    sys.path.insert(0, str(simplehand_root))
    os.chdir(str(simplehand_root))
    return simplehand_root


def load_model(simplehand_root, checkpoint_path, device):
    import torch
    from cfg import _CONFIG
    from hand_net import HandNet

    checkpoint_path = Path(checkpoint_path).expanduser().resolve()
    if not checkpoint_path.exists():
        raise FileNotFoundError(f"checkpoint 不存在：{checkpoint_path}")

    model = HandNet(_CONFIG, pretrained=False)
    try:
        # 优先使用 weights_only=True，避免 torch.load 通过 pickle 反序列化非权重对象。
        checkpoint = torch.load(str(checkpoint_path), map_location="cpu", weights_only=True)
    except TypeError:
        # 兼容较旧 PyTorch：老版本 torch.load 还没有 weights_only 参数。
        checkpoint = torch.load(str(checkpoint_path), map_location="cpu")
    if "state_dict" not in checkpoint:
        raise KeyError("checkpoint 中没有 state_dict 字段")
    model.load_state_dict(checkpoint["state_dict"], strict=True)
    model.to(device)
    model.eval()
    return model


def load_faces(simplehand_root, mano_pkl):
    pkl_path = Path(mano_pkl).expanduser().resolve() if mano_pkl else simplehand_root / "models" / "MANO_RIGHT_C.pkl"
    if not pkl_path.exists():
        raise FileNotFoundError(f"MANO_RIGHT_C.pkl 不存在：{pkl_path}")

    with open(pkl_path, "rb") as f:
        mano_data = pickle.load(f)
    faces = np.asarray(mano_data["f"], dtype=np.int32)
    return faces, str(pkl_path)


def export_obj(path, vertices, faces):
    with open(path, "w", encoding="utf-8") as f:
        f.write("# simpleHand reconstruction OBJ\n")
        for vertex in vertices:
            f.write("v %.8f %.8f %.8f\n" % (vertex[0], vertex[1], vertex[2]))
        for face in faces:
            f.write("f %d %d %d\n" % (face[0] + 1, face[1] + 1, face[2] + 1))


def export_stl(path, vertices, faces):
    try:
        import trimesh
    except ImportError as exc:
        raise RuntimeError("导出 STL 需要安装 trimesh：pip install trimesh") from exc

    mesh = trimesh.Trimesh(vertices=vertices, faces=faces, process=False)
    mesh.export(path)


def run_inference(args, output_paths):
    import torch

    start_time = time.time()
    image_path = Path(args.image).expanduser().resolve()
    checkpoint_path = Path(args.checkpoint).expanduser().resolve()
    mano_pkl = str(Path(args.mano_pkl).expanduser().resolve()) if args.mano_pkl else ""
    simplehand_root = prepare_simplehand_import(args.simplehand_root)
    device = resolve_device(args.device)
    image_tensor, preprocess_info = load_image_tensor(image_path, args.input_size, device)
    model = load_model(simplehand_root, checkpoint_path, device)
    faces, mano_path = load_faces(simplehand_root, mano_pkl)

    with torch.no_grad():
        output = model(image_tensor)

    uv = output["uv"].reshape(1, 21, 2)[0].detach().cpu().numpy()
    joints = output["joints"].reshape(1, 21, 3)[0].detach().cpu().numpy()
    vertices = output["vertices"].reshape(1, 778, 3)[0].detach().cpu().numpy()

    export_obj(output_paths["obj_path"], vertices, faces)
    export_stl(output_paths["stl_path"], vertices, faces)

    elapsed = time.time() - start_time
    return {
        "success": True,
        "error": "",
        "input_image": str(image_path),
        "checkpoint": str(checkpoint_path),
        "simplehand_root": str(simplehand_root),
        "mano_pkl": mano_path,
        "device": str(device),
        "obj_path": str(Path(output_paths["obj_path"]).resolve()),
        "stl_path": str(Path(output_paths["stl_path"]).resolve()),
        "result_json_path": str(Path(output_paths["result_json_path"]).resolve()),
        "vertices_count": int(vertices.shape[0]),
        "faces_count": int(faces.shape[0]),
        "joints": joints.tolist(),
        "uv": uv.tolist(),
        "preprocess": preprocess_info,
        "timestamp": output_paths["timestamp"],
        "inference_time_sec": round(elapsed, 4),
    }


def main():
    args = parse_args()
    output_paths = make_output_paths(args.image, args.output_dir)
    result = {
        "success": False,
        "error": "",
        "input_image": str(Path(args.image).expanduser()),
        "checkpoint": str(Path(args.checkpoint).expanduser()),
        "device": args.device,
        "obj_path": output_paths["obj_path"],
        "stl_path": output_paths["stl_path"],
        "result_json_path": output_paths["result_json_path"],
        "vertices_count": 0,
        "faces_count": 0,
        "joints": [],
        "uv": [],
        "preprocess": {},
        "timestamp": output_paths["timestamp"],
    }

    try:
        result = run_inference(args, output_paths)
        write_result_json(output_paths["result_json_path"], result)
        print(output_paths["result_json_path"])
        return 0
    except Exception as exc:
        result["error"] = str(exc)
        result["traceback"] = traceback.format_exc()
        try:
            write_result_json(output_paths["result_json_path"], result)
        except Exception:
            pass
        print(f"重建失败：{exc}", file=sys.stderr)
        print(f"结果文件：{output_paths['result_json_path']}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
