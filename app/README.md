# Qt 应用层

本目录包含 Qt Widgets + CMake + C++17 桌面应用代码，用于连接图像处理、三维重建和模型预览流程：

- 打开 JPG、PNG、BMP 图片。
- 显示二维图像，并支持缩放、适应窗口、左右 90 度旋转、镜像和保存处理图。
- 启动时读取 `resources/config/app_config.example.json`。
- 自动检查相机参数文件是否存在。
- 通过 `QProcess` 调用 Python 去畸变脚本。
- 通过 `QProcess` 调用 simpleHand 重建脚本，并读取 `*_result.json` 更新界面状态。
- 按设计稿搭建左侧二维图像、右侧三维预览、底部信息面板和状态栏。
- 三维预览区可加载 OBJ/STL，并支持旋转、缩放、平移、重置视角和线框/实体切换。

当前结构：

```text
app/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── AppConfig.h
│   ├── AppConfig.cpp
│   ├── MainWindow.h
│   ├── MainWindow.cpp
│   ├── ImageViewerWidget.h
│   ├── ImageViewerWidget.cpp
│   ├── ModelViewerWidget.h
│   ├── ModelViewerWidget.cpp
│   ├── ReconstructionPanel.h
│   ├── ReconstructionPanel.cpp
│   ├── ReconstructionRunner.h
│   ├── ReconstructionRunner.cpp
│   ├── SettingsDialog.h
│   └── SettingsDialog.cpp
└── resources/
```

## 必须理解的 Qt 概念

- `QApplication`：整个 Qt 程序的入口对象，负责事件循环。`main.cpp` 里 `app.exec()` 开始等待鼠标、键盘和窗口事件。
- `QMainWindow`：主窗口类型。本项目的 `MainWindow` 管理顶部工具栏、中心区域、底部状态栏和总体状态切换。
- `QWidget`：所有界面零件的基类。按钮、标签、图像区、信息面板本质上都是 QWidget 或其子类。
- `signal/slot`：Qt 的事件连接方式。例如按钮 `clicked` 信号连接到 `MainWindow::openImage()` 槽函数，点击按钮时自动调用函数。
- `QGraphicsView`：二维图像查看区使用它实现。可以理解为一个带滚动、缩放和平移能力的画布视图。
- `QOpenGLWidget`：三维预览区的基础。当前 `ModelViewerWidget` 在其中读取 OBJ/STL 三角网格，并用 `QPainter` 绘制可交互预览。
- `QProcess`：Qt 调用外部命令的类。本项目用它调用 Python 脚本，避免把算法代码直接写进 C++ 界面层。
- `QDialog`：弹出式对话框。当前设置窗口就是 `SettingsDialog`，用于修改 Python、脚本和输出目录等运行参数。
- `CMake AUTOMOC`：Qt 带 `Q_OBJECT` 的类需要额外生成元对象代码。`app/CMakeLists.txt` 中开启 `CMAKE_AUTOMOC` 后，CMake 会自动完成这一步。

## 构建和运行

在项目根目录运行：

```bash
/opt/homebrew/bin/cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
/opt/homebrew/bin/cmake --build build --target HandReconstruction -j4
./build/app/HandReconstruction
```

也可以在 Qt Creator 中打开项目根目录的 `CMakeLists.txt`，选择已经验证过的桌面 Kit 构建运行。

## 当前入口说明

- 去畸变按钮调用：

```bash
python preprocessing/camera_calibration/undistort.py --calibration camera_params.json --input-file input.jpg --output-file output.jpg --crop
```

- 开始重建按钮调用：

```bash
python reconstruction/simplehand_reconstruct.py --image input.jpg --checkpoint reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1 --output-dir outputs/mesh --device auto
```

脚本会在 `outputs/mesh` 下生成 OBJ、STL 和 `*_result.json`。Qt 在进程结束后读取 JSON 中的 `success`、`vertices_count`、`faces_count`、`obj_path`、`stl_path`、`inference_time_sec` 等字段，并更新底部信息面板。

simpleHand 推理所需源码已经放在 `reconstruction/simplehand_vendor/`，设置窗口中不再需要选择外部 simpleHand 源码目录。

## 三维模型预览操作

- 左键拖动：旋转模型。
- 右键或中键拖动：平移模型。
- 鼠标滚轮：缩放模型。
- `视角重置`：恢复默认旋转、缩放和平移。
- `适应窗口`：把缩放和平移恢复到适合当前窗口的状态。
- `线框/实体`：切换网格线框和实体面片显示。

当前查看器支持：

- simpleHand 导出的 OBJ。
- ASCII STL。
- Binary STL。

说明：当前三维预览是轻量级查看器，用于快速查看重建结果；它不是完整 CAD/医学三维渲染引擎。后续若需要更真实的光照、深度缓冲或大模型性能，可以把绘制部分升级为真正的 OpenGL shader 渲染。

## Windows 打包思路

Windows 不能直接使用 macOS 上编译出来的程序，需要在 Windows 机器或 Windows 虚拟机中重新构建。

建议流程：

1. 安装 Qt 6 for Windows，勾选 `MSVC 2022 64-bit` 或 `MinGW 64-bit` 套件。
2. 安装对应编译器：
   - MSVC 路线：安装 Visual Studio 2022，勾选“使用 C++ 的桌面开发”。
   - MinGW 路线：使用 Qt 安装器自带 MinGW。
3. 安装 CMake，并用 Qt Creator 打开项目根目录的 `CMakeLists.txt`。
4. 选择 Windows 桌面 Kit 构建，生成 `HandReconstruction.exe`。
5. 用 Qt 自带部署工具复制运行时依赖：

```bat
windeployqt path\to\HandReconstruction.exe
```

6. 把运行所需资源一起放到发布目录：
   - `resources/config/app_config.example.json`
   - `preprocessing/camera_calibration/undistort.py`
   - `preprocessing/camera_calibration/outputs/camera_params.json`
   - 后续的 `reconstruction/reconstruct.py`
   - 模型 checkpoint 或 MANO/simpleHand 资源
7. 安装或打包 Python 环境。首版最简单做法是在目标机器安装 Python/conda，并在设置窗口里选择 `python.exe`；更正式的做法是用 PyInstaller 或 conda-pack 把 Python 推理环境随程序一起发。

注意：当前配置模板里有 macOS 绝对 Python 路径，Windows 发布时要改成 Windows 路径，例如：

```json
{
  "python_interpreter": "C:/Users/yourname/miniconda3/envs/pytorch/python.exe"
}
```

如果后续改成 ONNX Runtime C++ 推理，Windows 打包会更简单，因为可以减少 Python 环境依赖。

## 开发时参考

- `design/ui_mockup.png`
- `design/ui_mockup.html`
- `docs/qt_development_environment.md`
- `resources/config/app_config.example.json`
