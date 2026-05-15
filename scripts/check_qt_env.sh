#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/hand_reconstruction_qt_env_check.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

echo "== Hand Reconstruction Qt environment check =="
echo "Project: $ROOT_DIR"
echo

echo "== Tools =="
command -v cmake
cmake --version | head -1
command -v qmake
qmake -v
command -v qt-cmake
qt-cmake --version | head -1
echo

echo "== Xcode / compiler =="
xcode-select -p
xcrun --find clang++
clang++ --version | head -3
echo

cat > "$TMP_DIR/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.21)
project(HandReconstructionQtEnvCheck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
qt_standard_project_setup()

qt_add_executable(HandReconstructionQtEnvCheck main.cpp)
target_link_libraries(HandReconstructionQtEnvCheck PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)
CMAKE

cat > "$TMP_DIR/main.cpp" <<'CPP'
#include <QApplication>
#include <QLabel>
#include <QOpenGLWidget>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Hand Reconstruction Qt Environment Check");

    auto *layout = new QVBoxLayout(&window);
    layout->addWidget(new QLabel("Qt Widgets + OpenGLWidgets environment OK"));
    layout->addWidget(new QOpenGLWidget());
    window.resize(420, 260);

    return 0;
}
CPP

echo "== Configure =="
cmake -S "$TMP_DIR" -B "$TMP_DIR/build" \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DCMAKE_BUILD_TYPE=Debug
echo

echo "== Build =="
cmake --build "$TMP_DIR/build" --parallel
echo

echo "== Result =="
if [[ -x "$TMP_DIR/build/HandReconstructionQtEnvCheck.app/Contents/MacOS/HandReconstructionQtEnvCheck" ]]; then
  file "$TMP_DIR/build/HandReconstructionQtEnvCheck.app/Contents/MacOS/HandReconstructionQtEnvCheck"
elif [[ -x "$TMP_DIR/build/HandReconstructionQtEnvCheck" ]]; then
  file "$TMP_DIR/build/HandReconstructionQtEnvCheck"
else
  find "$TMP_DIR/build" -maxdepth 3 -type f -perm +111 -print
fi

echo
echo "Qt environment check passed."
