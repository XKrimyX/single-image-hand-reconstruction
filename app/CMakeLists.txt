cmake_minimum_required(VERSION 3.20)

project(HandReconstructionApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

qt_add_executable(HandReconstruction
    src/main.cpp
    src/AppConfig.cpp
    src/AppConfig.h
    src/ImageViewerWidget.cpp
    src/ImageViewerWidget.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/ModelViewerWidget.cpp
    src/ModelViewerWidget.h
    src/ReconstructionPanel.cpp
    src/ReconstructionPanel.h
    src/ReconstructionRunner.cpp
    src/ReconstructionRunner.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
)

target_link_libraries(HandReconstruction PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
