/*
 * cdownloadmanager_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/network/HttpDownloader.h"

#include <QTimer>
#include <QUrl>

class CDownloadManager : public QObject, public IHttpDownloaderListener
{
	Q_OBJECT

	struct FileEntry
	{
		enum Status
		{
			QUEUED,
			IN_PROGRESS,
			FINISHED,
			FAILED
		};

		QUrl url;
		QString filename;
		QString filePath;
		Status status;
		qint64 bytesReceived;
		qint64 totalSize;
	};

	QStringList encounteredErrors;

	HttpDownloader downloader;
	QTimer pollTimer;

	QList<FileEntry> currentDownloads;

	FileEntry & getActiveEntry();
	void copyLocalFile();
	void startNextDownload();
	bool hasDownloadInProgress() const;

	void onDownloadProgress(uint64_t received, uint64_t total) override;
	void onDownloadFinished(const std::string & errorMessage) override;

public:
	CDownloadManager();

	// returns true if download with such URL is in progress/queued
	// FIXME: not sure what's right place for "mod download in progress" check
	bool downloadInProgress(const QUrl & url) const;

	void downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal = 0);

signals:
	// for status bar updates. Merges all queued downloads into one
	void downloadProgress(QString currentFile, qint64 currentAmount, qint64 maxAmount);
	void downloadFileFinished(QString fileName);

	// called when all files were downloaded and manager goes to idle state
	// Lists contains files that were successfully downloaded / failed to download
	void finished(QStringList savedFiles, QStringList failedFiles, QStringList errors);
};
