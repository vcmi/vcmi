/*
 * demo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>

class CDownloadManager;

class Demo : public QObject
{
	Q_OBJECT

    CDownloadManager * dlManager;

    std::function<void ()> onFinish;
    std::function<void ()> onError;
    std::function<void (float percent)> onProgress;

private slots:
	void downloadProgress(qint64 current, qint64 max);
	void downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors);

public:
    Demo(std::function<void ()> onFinish = nullptr, std::function<void ()> onError = nullptr, std::function<void (float percent)> onProgress = nullptr);
	void download();
	void install(QString filename);
};
