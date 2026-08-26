/*
 * Copyright (C) 2019-2021 Ashar Khan <ashar786khan@gmail.com>
 * Modified for browser-based manual submission.
 *
 * This file is part of CP Editor.
 */

#include "Extensions/CFTool.hpp"
#include "Core/EventLogger.hpp"
#include "Core/MessageLogger.hpp"
#include "generated/SettingsHelper.hpp"
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>

namespace Extensions
{

CFTool::CFTool(const QString &path, MessageLogger *logger) : CFToolPath(path)
{
    LOG_INFO(INFO_OF(path))
    log = logger;
}

CFTool::~CFTool() = default;

void CFTool::submit(const QString &filePath, const QString &url)
{
    LOG_INFO(INFO_OF(filePath) << INFO_OF(url));

    if (!parseCfUrl(url, problemContestId, problemCode))
    {
        log->error(tr("CF Tool"), tr("Failed to parse the Codeforces problem URL: %1").arg(url));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        log->error(tr("CF Tool"), tr("Failed to read source file: %1").arg(filePath));
        return;
    }

    QTextStream in(&file);
    const QString sourceCode = in.readAll();
    file.close();

    if (sourceCode.trimmed().isEmpty())
    {
        log->error(tr("CF Tool"), tr("Source code is empty!"));
        return;
    }

    // Copy the complete source to the clipboard before opening Codeforces.
    QGuiApplication::clipboard()->setText(sourceCode);

    const QString targetUrl = buildSubmitUrl(url);
    if (targetUrl.isEmpty())
    {
        log->error(tr("CF Tool"), tr("Could not construct the Codeforces submission URL."));
        return;
    }

    log->info(tr("CF Tool"), tr("Source copied to clipboard (%1 chars)").arg(sourceCode.length()));
    log->info(tr("CF Tool"), tr("Opening the Codeforces submission page..."));

    if (!QDesktopServices::openUrl(QUrl(targetUrl)))
    {
        log->error(tr("CF Tool"), tr("Failed to open the Codeforces submission page in the browser."));
        showToastMessage(tr("Failed to open browser"));
        return;
    }

    // Important: do not automate Chrome. Codeforces anti-bot protections can
    // block scripted interaction. The user only needs to paste and press Submit.
    log->info(tr("CF Tool"),
              tr("Submission page opened for Problem %1. Paste with Ctrl+V/Cmd+V and click Submit.")
                  .arg(problemCode));
    showToastMessage(tr("Code copied — paste and click Submit"));
}

QString CFTool::buildSubmitUrl(const QString &url)
{
    QRegularExpression matchContest(
        "^https?://(?:www\\.)?codeforces\\.com/(contest|gym)/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    auto match = matchContest.match(url);
    if (match.hasMatch())
    {
        const QString type = match.captured(1);
        const QString contestId = match.captured(2);
        const QString problem = match.captured(3);
        return QString("https://codeforces.com/%1/%2/submit/%3").arg(type, contestId, problem);
    }

    QRegularExpression matchProblemset(
        "^https?://(?:www\\.)?codeforces\\.com/problemset/problem/([1-9][0-9]*)/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    match = matchProblemset.match(url);
    if (match.hasMatch())
    {
        const QString contestId = match.captured(1);
        const QString problem = match.captured(2);
        return QString("https://codeforces.com/contest/%1/submit/%2").arg(contestId, problem);
    }

    QRegularExpression matchGroup(
        "^https?://(?:www\\.)?codeforces\\.com/group/([^/]+)/contest/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    match = matchGroup.match(url);
    if (match.hasMatch())
    {
        const QString group = match.captured(1);
        const QString contestId = match.captured(2);
        const QString problem = match.captured(3);
        return QString("https://codeforces.com/group/%1/contest/%2/submit/%3").arg(group, contestId, problem);
    }

    return QString();
}

bool CFTool::check(const QString &path)
{
    Q_UNUSED(path);
    // Browser/manual mode does not require cf-tool to be installed.
    return true;
}

void CFTool::updatePath(const QString &p)
{
    LOG_INFO(INFO_OF(p));
    CFToolPath = p;
}

bool CFTool::parseCfUrl(const QString &url, QString &contestId, QString &problemCode)
{
    QRegularExpression matchContest(
        "^https?://(?:www\\.)?codeforces\\.com/(?:contest|gym)/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    auto match = matchContest.match(url);
    if (match.hasMatch())
    {
        contestId = match.captured(1);
        problemCode = match.captured(2).toUpper();
        return true;
    }

    QRegularExpression matchProblemset(
        "^https?://(?:www\\.)?codeforces\\.com/problemset/problem/([1-9][0-9]*)/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    match = matchProblemset.match(url);
    if (match.hasMatch())
    {
        contestId = match.captured(1);
        problemCode = match.captured(2).toUpper();
        return true;
    }

    QRegularExpression matchGroup(
        "^https?://(?:www\\.)?codeforces\\.com/group/([^/]+)/contest/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$");
    match = matchGroup.match(url);
    if (match.hasMatch())
    {
        contestId = match.captured(2);
        problemCode = match.captured(3).toUpper();
        return true;
    }

    return false;
}

void CFTool::onReadReady() {}

void CFTool::onFinished(int exitCode, QProcess::ExitStatus e)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(e);
}

void CFTool::showToastMessage(const QString &message)
{
    if (SettingsHelper::isCFShowToastMessages())
        emit requestToastMessage(tr("Contest %1 Problem %2").arg(problemContestId).arg(problemCode), message);
}

} // namespace Extensions
