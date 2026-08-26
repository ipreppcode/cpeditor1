/*
 * Copyright (C) 2019-2021 Ashar Khan <ashar786khan@gmail.com>
 * Modified for browser-based manual submission.
 *
 * This file is part of CP Editor.
 */

#ifndef CFTOOL_HPP
#define CFTOOL_HPP

#include <QObject>
#include <QProcess>

class MessageLogger;

namespace Extensions
{
class CFTool : public QObject
{
    Q_OBJECT

  public:
    CFTool(const QString &path, MessageLogger *logger);
    ~CFTool() override;
    void submit(const QString &filePath, const QString &url);
    static bool check(const QString &path);
    void updatePath(const QString &p);
    static bool parseCfUrl(const QString &url, QString &contestId, QString &problemCode);

  signals:
    void requestToastMessage(const QString &head, const QString &body);

  private slots:
    void onReadReady();
    void onFinished(int exitCode, QProcess::ExitStatus);

  private:
    QString problemContestId, problemCode;
    QProcess *CFToolProcess = nullptr;
    MessageLogger *log;
    QString CFToolPath;

    void showToastMessage(const QString &message);
};
} // namespace Extensions

#endif // CFTOOL_HPP
