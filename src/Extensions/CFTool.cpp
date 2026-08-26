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
#include <QUrlQuery>

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

    // Copy source first. No browser DOM or keyboard automation is used.
    QGuiApplication::clipboard()->setText(sourceCode);

    QUrl problemUrl(url);
    problemUrl.setQuery(QUrlQuery());
    problemUrl.setFragment(QString());

    log->info(tr("CF Tool"), tr("Source copied to clipboard (%1 chars)").arg(sourceCode.length()));
    log->info(tr("CF Tool"), tr("Opening the Codeforces problem page in your default browser..."));

    if (!QDesktopServices::openUrl(problemUrl))
    {
        log->error(tr("CF Tool"), tr("Failed to open the Codeforces problem page in the browser."));
        showToastMessage(tr("Failed to open browser"));
        return;
    }

    log->info(tr("CF Tool"),
              tr("Problem page opened. Paste the code with Ctrl+V/Cmd+V, then click Submit normally."));
    showToastMessage(tr("Code copied — paste and click Submit"));
}

bool CFTool::check(const QString &path)
{
    Q_UNUSED(path);
    return true;
}

void CFTool::updatePath(const QString &p)
{
    LOG_INFO(INFO_OF(p));
    CFToolPath = p;
}

bool CFTool::parseCfUrl(const QString &url, QString &contestId, QString &problemCode)
{
    LOG_INFO(INFO_OF(url));

    auto match = QRegularExpression(
                     R"(^https?://(?:www\.)?codeforces\.com/(?:contest|gym)/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$)")")
                     .match(url);
    if (match.hasMatch())
    {
        contestId = match.captured(1);
        problemCode = match.captured(2).toUpper();
        return true;
    }

    match = QRegularExpression(
                R"(^https?://(?:www\.)?codeforces\.com/problemset/problem/([1-9][0-9]*)/([A-Za-z][0-9]?)(?:[/?#].*)?$)")")
                .match(url);
    if (match.hasMatch())
    {
        contestId = match.captured(1);
        problemCode = match.captured(2).toUpper();
        return true;
    }

    match = QRegularExpression(
                R"(^https?://(?:www\.)?codeforces\.com/group/([^/]+)/contest/([1-9][0-9]*)/problem/([A-Za-z][0-9]?)(?:[/?#].*)?$)")")
                .match(url);
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
