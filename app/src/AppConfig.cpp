#include "AppConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

QString AppConfig::projectRoot()
{
    QDir dir(QCoreApplication::applicationDirPath());
    // 开发期可执行文件在 build/app/ 下，所以从程序目录逐级向上寻找项目根目录。
    for (int i = 0; i < 8; ++i) {
        if (QFile::exists(dir.filePath("resources/config/app_config.example.json"))) {
            return dir.absolutePath();
        }
        dir.cdUp();
    }

    return QDir::currentPath();
}

QString AppConfig::resolvePath(const QString &path) const
{
    if (path.isEmpty()) {
        return QString();
    }

    QFileInfo info(path);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }

    return QDir(projectRoot()).absoluteFilePath(path);
}

AppConfig AppConfig::loadDefault()
{
    AppConfig config;
    config.pythonInterpreter = "python";
    config.cameraParamsPath = "preprocessing/camera_calibration/outputs/camera_params.json";
    config.undistortScriptPath = "preprocessing/camera_calibration/undistort.py";
    config.reconstructScriptPath = "reconstruction/simplehand_reconstruct.py";
    config.simplehandCheckpoint = "reconstruction/checkpoints/simplehand_drive/epoch_200_rerun1";
    config.reconstructDevice = "auto";
    config.outputDir = "outputs";

    QFile file(QDir(projectRoot()).filePath("resources/config/app_config.example.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return config;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject object = document.object();
    config.pythonInterpreter = object.value("python_interpreter").toString(config.pythonInterpreter);
    config.cameraParamsPath = object.value("camera_params").toString(config.cameraParamsPath);
    config.undistortScriptPath = object.value("undistort_script").toString(config.undistortScriptPath);
    config.reconstructScriptPath = object.value("reconstruct_script").toString(config.reconstructScriptPath);
    config.simplehandCheckpoint = object.value("simplehand_checkpoint").toString(config.simplehandCheckpoint);
    config.reconstructDevice = object.value("reconstruct_device").toString(config.reconstructDevice);
    config.outputDir = object.value("output_dir").toString(config.outputDir);
    config.autoLoadCameraParams = object.value("auto_load_camera_params").toBool(config.autoLoadCameraParams);
    config.autoUndistortBeforeReconstruct =
        object.value("auto_undistort_before_reconstruct").toBool(config.autoUndistortBeforeReconstruct);

    return config;
}
