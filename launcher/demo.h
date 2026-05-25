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

class IDemoInstallerCallback
{
public:
	virtual void onInstallFinished() = 0;
	virtual void onInstallError() = 0;
	virtual void onInstallProgress(float percent) = 0;
	virtual ~IDemoInstallerCallback() = default;
};

class DemoInstaller : public QObject
{
	Q_OBJECT

	IDemoInstallerCallback * callback;
    CDownloadManager * dlManager;
    bool usedAlternative = false;

private:
	void startDownload(const QUrl & url);
private slots:
	void downloadProgress(qint64 current, qint64 max);
	void downloadFinished(QStringList savedFiles, QStringList failedFiles, QStringList errors);

public:
    DemoInstaller(IDemoInstallerCallback * callback = nullptr);
	void download();
	void install(QString filename);
};
