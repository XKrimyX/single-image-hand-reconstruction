#pragma once

#include "AppConfig.h"

#include <QMainWindow>

class QAction;
class ImageViewerWidget;
class ModelViewerWidget;
class QPushButton;
class ReconstructionPanel;
class ReconstructionRunner;
class QSplitter;
struct ReconstructionResult;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void openImage();
    void saveProcessedImage();
    void startUndistort();
    void startReconstruct();
    void saveScreenshot();
    void exportObj();
    void exportStl();
    void openSettings();
    void handleImageLoaded(const QString &filePath, const QSize &size);
    void handleTaskStarted(const QString &taskName);
    void handleUndistortFinished(bool ok, const QString &outputImagePath, const QString &message);
    void handleReconstructFinished(bool ok, const ReconstructionResult &result, const QString &message, qint64 elapsedMs);

private:
    void buildUi();
    void createToolbar();
    void createImagePanel(QSplitter *mainSplitter);
    void createModelPanel(QSplitter *mainSplitter);
    void loadCameraParams();
    void updateActionState();
    QString makeOutputPath(const QString &subDir, const QString &suffix) const;
    QPushButton *makeSmallButton(const QString &text);

    AppConfig m_config;
    ImageViewerWidget *m_imageViewer = nullptr;
    ModelViewerWidget *m_modelViewer = nullptr;
    ReconstructionPanel *m_panel = nullptr;
    ReconstructionRunner *m_runner = nullptr;

    QAction *m_openAction = nullptr;
    QAction *m_undistortAction = nullptr;
    QAction *m_reconstructAction = nullptr;
    QAction *m_settingsAction = nullptr;

    QPushButton *m_saveImageButton = nullptr;
    QPushButton *m_saveScreenshotButton = nullptr;
    QPushButton *m_exportObjButton = nullptr;
    QPushButton *m_exportStlButton = nullptr;

    QString m_currentImagePath;
    QString m_processedImagePath;
    QString m_currentMeshPath;
    QString m_currentStlPath;
    bool m_cameraLoaded = false;
};
