#pragma once

#include "AppConfig.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QProcess>

struct ReconstructionResult
{
    bool success = false;
    int verticesCount = 0;
    int facesCount = 0;
    double inferenceTime = 0.0;
    QString outputObj;
    QString outputStl;
    QString resultJson;
    QString error;
};

class ReconstructionRunner : public QObject
{
    Q_OBJECT

public:
    explicit ReconstructionRunner(QObject *parent = nullptr);

    void setConfig(const AppConfig &config);
    bool isRunning() const;
    void runUndistort(const QString &inputImagePath, const QString &outputImagePath);
    void runReconstruct(const QString &inputImagePath);

signals:
    void started(const QString &taskName);
    void logMessage(const QString &message);
    void undistortFinished(bool ok, const QString &outputImagePath, const QString &message);
    void reconstructFinished(bool ok, const ReconstructionResult &result, const QString &message, qint64 elapsedMs);

private slots:
    void handleReadyRead();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    enum class Task
    {
        None,
        Undistort,
        Reconstruct
    };

    bool startTask(Task task, const QString &taskName, const QString &scriptPath, const QStringList &arguments);
    QString findLatestResultJson(const QString &outputDir) const;
    ReconstructionResult readReconstructionResult(const QString &jsonPath) const;

    AppConfig m_config;
    QProcess *m_process = nullptr;
    QElapsedTimer m_timer;
    Task m_currentTask = Task::None;
    QString m_currentOutputPath;
    QString m_currentOutputDir;
};
