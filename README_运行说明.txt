基于单张图像的手部三维模型重建系统 - Windows 运行说明

一、项目说明

本项目是一个 Qt 桌面应用。程序可以打开手部图片，调用 Python 重建脚本生成 OBJ/STL 三维手部模型，并在 Qt 界面中预览和导出模型。

当前仓库只保留编译运行 Qt 程序需要的源码、配置和脚本。模型 checkpoint 文件体积较大，不放在 GitHub 中，需要单独复制。


二、需要准备的环境

1. Qt / C++ 环境

建议安装：

- Qt 6.x for Windows
- Qt Creator
- MSVC 2022 64-bit Kit
- CMake

推荐使用 Qt Creator 打开项目根目录下的 CMakeLists.txt，然后选择 Desktop Qt 6.x MSVC 2022 64-bit Kit 构建。


2. Python 环境

建议使用 conda 创建环境：

conda create -n handrecon python=3.10 -y
conda activate handrecon

需要安装的 Python 依赖：

pip install opencv-python numpy timm==0.9.12 hiera-transformer==0.1.2 trimesh

PyTorch 请根据老师电脑或测试电脑环境安装 CPU 版或 CUDA 版。只做演示验证时建议使用 CPU 版，稳定性更高。


三、模型文件放置位置

请将单独发送的 checkpoint 文件放到下面目录：

reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1

如果有第二个 checkpoint，也可以放到：

reconstruction/checkpoints/simplehand_drive/epoch_200_rerun2

注意：如果没有 epoch_200_rerun1，程序可以启动，但点击“开始重建”会失败。


四、修改配置文件

程序只读取下面这个配置文件：

resources/config/app_config.json

Windows 上需要重点修改 python_interpreter，例如：

{
  "python_interpreter": "C:/Users/yourname/miniconda3/envs/handrecon/python.exe",
  "camera_params": "preprocessing/camera_calibration/outputs/camera_params.json",
  "undistort_script": "preprocessing/camera_calibration/undistort.py",
  "reconstruct_script": "reconstruction/simplehand_reconstruct.py",
  "simplehand_checkpoint": "reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1",
  "reconstruct_device": "cpu",
  "output_dir": "outputs",
  "auto_load_camera_params": true,
  "auto_undistort_before_reconstruct": false
}

路径建议使用 /，例如 C:/Users/xxx/...，这样 JSON 中不需要写双反斜杠。


五、使用 Qt Creator 编译运行

1. 打开 Qt Creator。
2. 选择 File -> Open File or Project。
3. 选择项目根目录下的 CMakeLists.txt。
4. Kit 选择 Desktop Qt 6.x MSVC 2022 64-bit。
5. 点击 Configure Project。
6. 选择 Release 或 Debug 构建。
7. 点击 Build。
8. 点击 Run。

程序启动后：

1. 点击“打开图片”，选择 JPG/PNG/BMP 手部图片。
2. 如果图片与相机参数分辨率一致，可以点击“去畸变”。
3. 点击“开始重建”。
4. 重建成功后，右侧会显示三维模型。
5. 可以保存截图、导出 OBJ 或导出 STL。


六、命令行编译方式

如果不用 Qt Creator，也可以使用 Windows 命令行。

请先打开 x64 Native Tools Command Prompt for VS 2022，然后进入项目目录：

cd /d D:\GraProj

配置项目：

cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64

编译程序：

cmake --build build --target HandReconstruction --config Release -j4

生成的程序通常位于：

build\app\Release\HandReconstruction.exe


七、常见问题

1. 程序提示 Python 解释器不存在

检查 resources/config/app_config.json 中的 python_interpreter 是否是 Windows 上真实存在的 python.exe 路径。


2. 点击开始重建失败

优先检查：

- checkpoint 是否放在 reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1
- Python 依赖是否安装完整
- app_config.json 中 reconstruct_device 是否设置为 cpu
- 输入图片是否能被 OpenCV 读取


3. 去畸变失败

当前去畸变脚本要求输入图片尺寸与相机标定图片尺寸一致。当前相机参数对应 1920 x 1080 图片。如果输入图片尺寸不同，可以跳过去畸变，直接点击“开始重建”。


4. Qt Creator 找不到 Qt

确认安装了 Qt 6 Windows 版本，并选择了 MSVC 2022 64-bit Kit。命令行构建时需要把 CMAKE_PREFIX_PATH 改为本机 Qt 路径。


5. CPU 推理较慢

CPU 推理可能需要几秒或更久，属于正常情况。演示时建议等待底部日志输出完成。
