#pragma once

#include <QString>

struct AppConfig
{
    QString pythonInterpreter;
    QString cameraParamsPath;
    QString undistortScriptPath;
    QString reconstructScriptPath;
    QString simplehandCheckpoint;
    QString reconstructDevice = "auto";
    QString outputDir;
    bool autoLoadCameraParams = true;
    bool autoUndistortBeforeReconstruct = false;

    static AppConfig loadDefault();
    static QString projectRoot();
    QString resolvePath(const QString &path) const;
};
