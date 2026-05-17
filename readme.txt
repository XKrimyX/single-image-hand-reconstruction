基于单张图像的手部三维模型重建系统运行说明

一、开发环境

开发系统：macOS
Qt：Qt 6.11.0
CMake：Homebrew CMake
C++ 标准：C++17
Python 环境：conda 环境 pytorch
Python 版本：建议 3.10 或兼容版本

本项目使用 Qt Widgets 编写桌面界面，使用 QProcess 调用 Python 脚本完成图像去畸变和三维手部重建。


二、Windows 部署/运行环境

Windows 电脑需要准备：

1. Qt 6.x for Windows，建议 MSVC 2022 64-bit 版本。
2. Visual Studio 2022 或 Build Tools for Visual Studio 2022。
3. CMake。
4. Python 或 conda 环境。
5. requirements.txt 中列出的 Python 第三方库。

安装 Python 依赖：

pip install -r requirements.txt

PyTorch 如果安装失败，请根据本机 CPU/CUDA 情况到 PyTorch 官网选择 Windows 对应安装命令。只做运行验证时建议使用 CPU 版。


三、模型文件

模型 checkpoint 文件较大，不放在 GitHub 中，请单独复制。

需要放置到：

reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1

如果没有该文件，Qt 程序可以编译和启动，但点击“开始重建”会失败。


四、主要文件和文件夹说明

CMakeLists.txt
项目根目录 CMake 入口。

app/
Qt 桌面应用源码目录。

app/src/
Qt 主窗口、二维图像查看器、三维模型查看器、重建任务调用和设置窗口等 C++ 源码。

preprocessing/
预处理脚本目录。

preprocessing/camera_calibration/undistort.py
单张图片或批量图片去畸变脚本。

preprocessing/camera_calibration/outputs/camera_params.json
相机参数文件，Qt 启动时读取该文件判断相机参数是否存在。

reconstruction/
三维手部重建脚本目录。

reconstruction/simplehand_reconstruct.py
simpleHand 单张图片重建入口，输出 OBJ、STL 和 result.json。

reconstruction/simplehand_vendor/
simpleHand 推理所需的内置代码和 MANO 相关文件。

resources/config/app_config.json
程序运行配置文件。Windows 上需要根据实际 Python 路径修改 python_interpreter。

outputs/
程序运行输出目录。重建生成的 OBJ、STL、JSON 和截图会写入该目录。

requirements.txt
Python 第三方依赖列表。


五、修改配置文件

程序只读取：

resources/config/app_config.json

Windows 上重点修改 python_interpreter，例如：

"python_interpreter": "C:/Users/yourname/miniconda3/envs/handrecon/python.exe"

如果已经在命令行激活了正确的 conda 环境，也可以保留：

"python_interpreter": "python"

建议保持：

"reconstruct_device": "cpu"


六、命令行编译 Qt 程序

请打开 x64 Native Tools Command Prompt for VS 2022，然后进入项目目录，例如：

cd /d D:\GraProj

配置 CMake：

cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64

请把 C:/Qt/6.x.x/msvc2022_64 改成自己电脑上真实的 Qt 安装路径。

编译 Release 版本：

cmake --build build --target HandReconstruction --config Release -j4

生成的程序通常位于：

build\app\Release\HandReconstruction.exe


七、运行流程

1. 启动 HandReconstruction.exe。
2. 点击“打开图片”，选择 JPG、PNG 或 BMP 手部图片。
3. 如需预处理并且图片尺寸与相机参数匹配，可点击“去畸变”。
4. 点击“开始重建”。
5. 重建成功后，右侧显示三维手部模型。
6. 可以导出 OBJ、STL 或保存三维预览截图。


八、常见问题

1. CMake 找不到 Qt

检查 CMAKE_PREFIX_PATH 是否指向 Qt 的 MSVC 版本目录，例如 C:/Qt/6.x.x/msvc2022_64。

2. Python 解释器不存在

检查 resources/config/app_config.json 中 python_interpreter 是否正确。

3. 开始重建失败

检查 checkpoint 是否已经放到 reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1，并确认 requirements.txt 中的依赖已安装。

4. 去畸变失败

当前相机参数对应 1920 x 1080 图片。若输入图片尺寸不同，可以跳过去畸变，直接开始重建。
