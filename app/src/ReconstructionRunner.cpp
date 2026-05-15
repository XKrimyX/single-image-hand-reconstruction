#include "ReconstructionRunner.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

ReconstructionRunner::ReconstructionRunner(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ReconstructionRunner::handleReadyRead);
    connect(m_process, &QProcess::finished, this, &ReconstructionRunner::handleFinished);
}

void ReconstructionRunner::setConfig(const AppConfig &config)
{
    m_config = config;
}

bool ReconstructionRunner::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void ReconstructionRunner::runUndistort(const QString &inputImagePath, const QString &outputImagePath)
{
    const QString script = m_config.resolvePath(m_config.undistortScriptPath);
    const QString camera = m_config.resolvePath(m_config.cameraParamsPath);
    QStringList arguments;
    arguments << script
              << "--calibration" << camera
              << "--input-file" << inputImagePath
              << "--output-file" << outputImagePath;

    m_currentOutputPath = outputImagePath;
    startTask(Task::Undistort, "去畸变", script, arguments);
}

void ReconstructionRunner::runReconstruct(const QString &inputImagePath)
{
    const QString script = m_config.resolvePath(m_config.reconstructScriptPath);
    const QString checkpoint = m_config.resolvePath(m_config.simplehandCheckpoint);
    const QString outputDir = QDir(m_config.resolvePath(m_config.outputDir)).filePath("mesh");
    QStringList arguments;
    arguments << script
              << "--image" << inputImagePath
              << "--checkpoint" << checkpoint
              << "--output-dir" << outputDir
              << "--device" << m_config.reconstructDevice;

    m_currentOutputPath.clear();
    m_currentOutputDir = outputDir;
    startTask(Task::Reconstruct, "重建", script, arguments);
}

void ReconstructionRunner::handleReadyRead()
{
    const QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput()).trimmed();
    if (!text.isEmpty()) {
        emit logMessage(text);
    }
}

void ReconstructionRunner::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
    const QString message = ok ? "执行完成" : QString("执行失败，退出码：%1").arg(exitCode);
    const qint64 elapsedMs = m_timer.elapsed();

    if (m_currentTask == Task::Undistort) {
        emit undistortFinished(ok, m_currentOutputPath, message);
    } else if (m_currentTask == Task::Reconstruct) {
        ReconstructionResult result;
        if (ok) {
            const QString resultJson = findLatestResultJson(m_currentOutputDir);
            result = readReconstructionResult(resultJson);
            ok = result.success;
        }
        emit reconstructFinished(ok, result, ok ? message : (result.error.isEmpty() ? message : result.error), elapsedMs);
    }

    m_currentTask = Task::None;
    m_currentOutputPath.clear();
    m_currentOutputDir.clear();
}

bool ReconstructionRunner::startTask(Task task, const QString &taskName, const QString &scriptPath, const QStringList &arguments)
{
    if (isRunning()) {
        emit logMessage("已有任务正在运行。");
        return false;
    }

    const QString python = m_config.pythonInterpreter;
    if (!QFileInfo::exists(python)) {
        emit logMessage(QString("Python 解释器不存在：%1").arg(python));
        return false;
    }

    if (!QFileInfo::exists(scriptPath)) {
        emit logMessage(QString("%1脚本不存在：%2").arg(taskName, scriptPath));
        return false;
    }

    if (task == Task::Reconstruct) {
        QDir().mkpath(m_currentOutputDir);
    } else {
        QFileInfo outputInfo(m_currentOutputPath);
        QDir().mkpath(outputInfo.absolutePath());
    }

    m_currentTask = task;
    m_timer.restart();
    emit started(taskName);
    // QProcess 不会卡住界面；Python 输出会通过 readyReadStandardOutput 信号逐步回到日志框。
    emit logMessage(QString("启动%1：%2 %3").arg(taskName, python, arguments.join(' ')));
    m_process->start(python, arguments);
    return true;
}

QString ReconstructionRunner::findLatestResultJson(const QString &outputDir) const
{
    QDir dir(outputDir);
    const QFileInfoList files = dir.entryInfoList(QStringList() << "*_result.json", QDir::Files, QDir::Time);
    if (files.isEmpty()) {
        return QString();
    }
    return files.first().absoluteFilePath();
}

ReconstructionResult ReconstructionRunner::readReconstructionResult(const QString &jsonPath) const
{
    ReconstructionResult result;
    result.resultJson = jsonPath;

    if (jsonPath.isEmpty()) {
        result.error = "重建脚本没有生成 *_result.json。";
        return result;
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = "无法读取重建结果 JSON：" + jsonPath;
        return result;
    }

    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    result.success = object.value("success").toBool(false);
    result.verticesCount = object.value("vertices_count").toInt(0);
    result.facesCount = object.value("faces_count").toInt(0);
    // 兼容当前脚本字段 obj_path/stl_path/inference_time_sec，以及后续可能改成的 output_obj/output_stl/inference_time。
    result.inferenceTime = object.value("inference_time").toDouble(object.value("inference_time_sec").toDouble(0.0));
    result.outputObj = object.value("output_obj").toString(object.value("obj_path").toString());
    result.outputStl = object.value("output_stl").toString(object.value("stl_path").toString());
    result.error = object.value("error").toString();

    if (!result.outputObj.isEmpty()) {
        result.outputObj = m_config.resolvePath(result.outputObj);
    }
    if (!result.outputStl.isEmpty()) {
        result.outputStl = m_config.resolvePath(result.outputStl);
    }

    if (result.success && (result.outputObj.isEmpty() || result.outputStl.isEmpty())) {
        result.success = false;
        result.error = "重建成功但 JSON 中缺少 OBJ 或 STL 输出路径。";
    }

    return result;
}
