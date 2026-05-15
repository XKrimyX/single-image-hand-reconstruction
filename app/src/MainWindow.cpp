#include "MainWindow.h"

#include "ImageViewerWidget.h"
#include "ModelViewerWidget.h"
#include "ReconstructionPanel.h"
#include "ReconstructionRunner.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_config = AppConfig::loadDefault();
    m_runner = new ReconstructionRunner(this);
    m_runner->setConfig(m_config);

    buildUi();
    loadCameraParams();
    updateActionState();

    connect(m_runner, &ReconstructionRunner::started, this, &MainWindow::handleTaskStarted);
    connect(m_runner, &ReconstructionRunner::logMessage, m_panel, &ReconstructionPanel::appendLog);
    connect(m_runner, &ReconstructionRunner::undistortFinished, this, &MainWindow::handleUndistortFinished);
    connect(m_runner, &ReconstructionRunner::reconstructFinished, this, &MainWindow::handleReconstructFinished);
}

void MainWindow::buildUi()
{
    setWindowTitle("单张图像手部三维重建系统");
    resize(1440, 900);

    createToolbar();

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    createImagePanel(mainSplitter);
    createModelPanel(mainSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    m_panel = new ReconstructionPanel(this);

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(10, 10, 10, 6);
    layout->setSpacing(10);
    layout->addWidget(mainSplitter, 1);
    layout->addWidget(m_panel);
    setCentralWidget(content);

    statusBar()->showMessage("就绪");

    setStyleSheet(
        "QMainWindow { background: #edf1f5; }"
        "QWidget { color: #243447; }"
        "QToolBar { background: #ffffff; border: 0; border-bottom: 1px solid #d9e2ec; spacing: 8px; padding: 8px; }"
        "QToolButton { padding: 8px 12px; border: 1px solid #cfd8e3; border-radius: 5px; background: #ffffff; color: #243447; }"
        "QToolButton:hover, QPushButton:hover { background: #eef6ff; }"
        "QToolButton:disabled, QPushButton:disabled { color: #98a2b3; background: #f4f6f8; }"
        "QGroupBox, QWidget#ReconstructionPanel { background: #ffffff; border: 1px solid #d9e2ec; border-radius: 6px; }"
        "QLabel#SectionTitle, QLabel#PanelTitle { font-weight: 700; color: #1f2937; font-size: 14px; }"
        "QLabel#InfoName { color: #667085; }"
        "QLabel#InfoValue { color: #344054; }"
        "QWidget#InfoCell { background: #f8fbff; border: 1px solid #d9e2ec; border-radius: 4px; }"
        "QPushButton { padding: 7px 10px; border: 1px solid #cfd8e3; border-radius: 5px; background: #ffffff; color: #243447; }"
        "QLineEdit { padding: 6px; border: 1px solid #cfd8e3; border-radius: 4px; background: #ffffff; color: #243447; }"
        "QCheckBox { color: #243447; }"
        "QTextEdit { border: 1px solid #d9e2ec; border-radius: 5px; background: #fbfdff; color: #344054; }"
        "QStatusBar { background: #ffffff; border-top: 1px solid #d9e2ec; color: #475467; }");
}

void MainWindow::createToolbar()
{
    auto *toolbar = addToolBar("主流程");
    toolbar->setMovable(false);

    m_openAction = toolbar->addAction("打开图片");
    m_undistortAction = toolbar->addAction("去畸变");
    m_reconstructAction = toolbar->addAction("开始重建");
    m_settingsAction = toolbar->addAction("设置");

    connect(m_openAction, &QAction::triggered, this, &MainWindow::openImage);
    connect(m_undistortAction, &QAction::triggered, this, &MainWindow::startUndistort);
    connect(m_reconstructAction, &QAction::triggered, this, &MainWindow::startReconstruct);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
}

void MainWindow::createImagePanel(QSplitter *mainSplitter)
{
    auto *container = new QWidget(mainSplitter);
    auto *title = new QLabel("二维图像工作区", container);
    title->setObjectName("SectionTitle");

    m_imageViewer = new ImageViewerWidget(container);
    connect(m_imageViewer, &ImageViewerWidget::imageLoaded, this, &MainWindow::handleImageLoaded);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(makeSmallButton("放大"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(0)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::zoomIn);
    buttonRow->addWidget(makeSmallButton("缩小"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(1)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::zoomOut);
    buttonRow->addWidget(makeSmallButton("适应窗口"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(2)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::fitToWindow);
    buttonRow->addWidget(makeSmallButton("左转90"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(3)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::rotateLeft);
    buttonRow->addWidget(makeSmallButton("右转90"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(4)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::rotateRight);
    buttonRow->addWidget(makeSmallButton("镜像"));
    connect(qobject_cast<QPushButton *>(buttonRow->itemAt(5)->widget()), &QPushButton::clicked, m_imageViewer, &ImageViewerWidget::mirrorHorizontal);
    m_saveImageButton = makeSmallButton("保存处理图");
    buttonRow->addWidget(m_saveImageButton);
    connect(m_saveImageButton, &QPushButton::clicked, this, &MainWindow::saveProcessedImage);
    buttonRow->addStretch();

    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->addWidget(title);
    layout->addWidget(m_imageViewer, 1);
    layout->addLayout(buttonRow);
}

void MainWindow::createModelPanel(QSplitter *mainSplitter)
{
    auto *container = new QWidget(mainSplitter);
    auto *title = new QLabel("三维模型预览区", container);
    title->setObjectName("SectionTitle");

    m_modelViewer = new ModelViewerWidget(container);

    auto *buttonRow = new QHBoxLayout;
    auto *resetButton = makeSmallButton("视角重置");
    auto *fitButton = makeSmallButton("适应窗口");
    auto *wireButton = makeSmallButton("线框/实体");
    m_saveScreenshotButton = makeSmallButton("保存截图");
    m_exportObjButton = makeSmallButton("导出 OBJ");
    m_exportStlButton = makeSmallButton("导出 STL");

    buttonRow->addWidget(resetButton);
    buttonRow->addWidget(fitButton);
    buttonRow->addWidget(wireButton);
    buttonRow->addWidget(m_saveScreenshotButton);
    buttonRow->addWidget(m_exportObjButton);
    buttonRow->addWidget(m_exportStlButton);
    buttonRow->addStretch();

    connect(resetButton, &QPushButton::clicked, m_modelViewer, &ModelViewerWidget::resetView);
    connect(fitButton, &QPushButton::clicked, m_modelViewer, &ModelViewerWidget::fitModel);
    connect(wireButton, &QPushButton::clicked, this, [this]() {
        static bool wireframe = false;
        wireframe = !wireframe;
        m_modelViewer->setWireframeMode(wireframe);
    });
    connect(m_saveScreenshotButton, &QPushButton::clicked, this, &MainWindow::saveScreenshot);
    connect(m_exportObjButton, &QPushButton::clicked, this, &MainWindow::exportObj);
    connect(m_exportStlButton, &QPushButton::clicked, this, &MainWindow::exportStl);

    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->addWidget(title);
    layout->addWidget(m_modelViewer, 1);
    layout->addLayout(buttonRow);
}

void MainWindow::openImage()
{
    // QFileDialog 是 Qt 提供的系统文件选择框，用户选完路径后再交给 ImageViewerWidget 显示。
    const QString filePath = QFileDialog::getOpenFileName(
        this, "打开手部图片", QString(), "图片文件 (*.jpg *.jpeg *.png *.bmp)");
    if (filePath.isEmpty()) {
        return;
    }

    if (!m_imageViewer->loadImage(filePath)) {
        QMessageBox::warning(this, "打开失败", "无法读取所选图片。");
        return;
    }

    m_currentImagePath = filePath;
    m_processedImagePath.clear();
    m_currentMeshPath.clear();
    m_currentStlPath.clear();
    m_modelViewer->clearMesh();
    m_panel->setOutputPath("-");
    updateActionState();
}

void MainWindow::saveProcessedImage()
{
    if (!m_imageViewer->hasImage()) {
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "保存处理图", makeOutputPath("images", ".png"), "PNG 图片 (*.png);;JPG 图片 (*.jpg)");
    if (filePath.isEmpty()) {
        return;
    }

    if (m_imageViewer->saveCurrentImage(filePath)) {
        statusBar()->showMessage("处理图已保存：" + filePath);
        m_panel->appendLog("处理图已保存：" + filePath);
    }
}

void MainWindow::startUndistort()
{
    if (m_currentImagePath.isEmpty() || !m_cameraLoaded || m_runner->isRunning()) {
        return;
    }

    const QString outputPath = makeOutputPath("images", "_undistorted.jpg");
    m_panel->setPreprocessStatus("去畸变中");
    // 去畸变属于 Python 预处理层，这里只启动外部脚本并等待它返回结果。
    m_runner->runUndistort(m_currentImagePath, outputPath);
    updateActionState();
}

void MainWindow::startReconstruct()
{
    if (m_currentImagePath.isEmpty() || m_runner->isRunning()) {
        return;
    }

    const QString inputPath = m_processedImagePath.isEmpty() ? m_currentImagePath : m_processedImagePath;
    m_panel->setModelStatus("重建中");
    // simpleHand 脚本会在 outputs/mesh 下自行生成 OBJ、STL 和 *_result.json。
    m_runner->runReconstruct(inputPath);
    updateActionState();
}

void MainWindow::saveScreenshot()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "保存三维预览截图", makeOutputPath("screenshots", ".png"), "PNG 图片 (*.png)");
    if (filePath.isEmpty()) {
        return;
    }

    if (m_modelViewer->renderToImage().save(filePath)) {
        statusBar()->showMessage("截图已保存：" + filePath);
        m_panel->appendLog("截图已保存：" + filePath);
    }
}

void MainWindow::exportObj()
{
    if (m_currentMeshPath.isEmpty()) {
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "导出 OBJ", QFileInfo(m_currentMeshPath).fileName(), "OBJ 模型 (*.obj)");
    if (!filePath.isEmpty()) {
        QFile::remove(filePath);
    }
    if (!filePath.isEmpty() && QFile::copy(m_currentMeshPath, filePath)) {
        statusBar()->showMessage("OBJ 已导出：" + filePath);
        m_panel->appendLog("OBJ 已导出：" + filePath);
    }
}

void MainWindow::exportStl()
{
    if (m_currentStlPath.isEmpty()) {
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "导出 STL", QFileInfo(m_currentStlPath).fileName(), "STL 模型 (*.stl)");
    if (!filePath.isEmpty()) {
        QFile::remove(filePath);
    }
    if (!filePath.isEmpty() && QFile::copy(m_currentStlPath, filePath)) {
        statusBar()->showMessage("STL 已导出：" + filePath);
        m_panel->appendLog("STL 已导出：" + filePath);
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 这里先把设置保存在本次运行内存中；后续需要持久化时再写入用户配置文件。
    m_config = dialog.config();
    m_runner->setConfig(m_config);
    loadCameraParams();
    updateActionState();
    m_panel->appendLog("设置已更新。");
}

void MainWindow::handleImageLoaded(const QString &filePath, const QSize &size)
{
    m_panel->setCurrentImage(filePath);
    m_panel->setImageStatus(QString("已加载，%1 x %2").arg(size.width()).arg(size.height()));
    m_panel->setPreprocessStatus("未处理");
    m_panel->setModelStatus("未重建");
    m_panel->setMeshStats(0, 0);
    m_panel->setInferenceTime("-");
    m_panel->appendLog("图片加载完毕：" + filePath);
    statusBar()->showMessage("图片加载完毕");
}

void MainWindow::handleTaskStarted(const QString &taskName)
{
    statusBar()->showMessage(taskName + "运行中");
    updateActionState();
}

void MainWindow::handleUndistortFinished(bool ok, const QString &outputImagePath, const QString &message)
{
    if (ok) {
        m_processedImagePath = outputImagePath;
        m_panel->setPreprocessStatus("已去畸变");
        m_panel->setOutputPath(outputImagePath);
        statusBar()->showMessage("去畸变完成");
    } else {
        m_panel->setPreprocessStatus("去畸变失败");
        statusBar()->showMessage(message);
    }
    updateActionState();
}

void MainWindow::handleReconstructFinished(bool ok, const ReconstructionResult &result, const QString &message, qint64 elapsedMs)
{
    if (ok) {
        m_currentMeshPath = result.outputObj;
        m_currentStlPath = result.outputStl;
        m_modelViewer->setMeshInfo(result.outputObj, result.verticesCount, result.facesCount);
        m_panel->setModelStatus("已重建");
        m_panel->setMeshStats(result.verticesCount, result.facesCount);
        m_panel->setOutputPath(QString("OBJ: %1\nSTL: %2").arg(result.outputObj, result.outputStl));
        m_panel->setInferenceTime(QString::number(result.inferenceTime, 'f', 2) + " s");
        m_panel->appendLog("重建结果 JSON：" + result.resultJson);
        statusBar()->showMessage("重建完成");
    } else {
        m_currentMeshPath.clear();
        m_currentStlPath.clear();
        m_panel->setModelStatus("重建失败");
        m_panel->setInferenceTime(QString::number(elapsedMs / 1000.0, 'f', 2) + " s");
        statusBar()->showMessage(message);
        m_panel->appendLog("重建失败：" + message);
    }
    updateActionState();
}

void MainWindow::loadCameraParams()
{
    const QString cameraPath = m_config.resolvePath(m_config.cameraParamsPath);
    m_cameraLoaded = m_config.autoLoadCameraParams && QFileInfo::exists(cameraPath);
    m_panel->setCameraStatus(m_cameraLoaded ? "已加载" : "未加载");
    m_panel->appendLog(m_cameraLoaded ? "相机参数已加载：" + cameraPath : "未找到相机参数：" + cameraPath);
}

void MainWindow::updateActionState()
{
    // 集中管理按钮状态，避免任务运行中重复点击导致多个 Python 进程同时执行。
    const bool hasImage = !m_currentImagePath.isEmpty();
    const bool running = m_runner && m_runner->isRunning();
    const bool hasMesh = !m_currentMeshPath.isEmpty();
    const bool hasStl = !m_currentStlPath.isEmpty();

    m_openAction->setEnabled(!running);
    m_undistortAction->setEnabled(hasImage && m_cameraLoaded && !running);
    m_reconstructAction->setEnabled(hasImage && !running);
    m_saveImageButton->setEnabled(hasImage && !running);
    m_saveScreenshotButton->setEnabled(hasMesh && !running);
    m_exportObjButton->setEnabled(hasMesh && !running);
    m_exportStlButton->setEnabled(hasStl && !running);
}

QString MainWindow::makeOutputPath(const QString &subDir, const QString &suffix) const
{
    const QString baseName = m_currentImagePath.isEmpty() ? "output" : QFileInfo(m_currentImagePath).completeBaseName();
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString outputDir = QDir(m_config.resolvePath(m_config.outputDir)).filePath(subDir);
    QDir().mkpath(outputDir);
    return QDir(outputDir).filePath(QString("%1_%2%3").arg(baseName, stamp, suffix));
}

QPushButton *MainWindow::makeSmallButton(const QString &text)
{
    auto *button = new QPushButton(text, this);
    button->setMinimumHeight(32);
    return button;
}
