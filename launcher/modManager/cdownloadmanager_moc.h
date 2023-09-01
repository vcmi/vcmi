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

#include <QSharedPointer>
#include <QtNetwork/QNetworkReply>

class QFile;

class CDownloadManager : public QObject
{
	Q_OBJECT

	struct FileEntry
	{
		enum Status
		{
			IN_PROGRESS,
			FINISHED,
			FAILED
		};

		QNetworkReply * reply;
		QSharedPointer<QFile> file;
		QString filename;
		Status status;
		qint64 bytesReceived;
		qint64 totalSize;
	};

	QStringList encounteredErrors;

	QNetworkAccessManager manager;

	QList<FileEntry> currentDownloads;

	FileEntry & getEntry(QNetworkReply * reply);

public:
	CDownloadManager();

	// returns true if download with such URL is in progress/queued
	// FIXME: not sure what's right place for "mod download in progress" check
	bool downloadInProgress(const QUrl & url) const;

	// returns network reply so caller can connect to required signals
	void downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal = 0);

public slots:
	void downloadFinished(QNetworkReply * reply);
	void downloadProgressChanged(qint64 bytesReceived, qint64 bytesTotal);

signals:
	// for status bar updates. Merges all queued downloads into one
	void downloadProgress(qint64 currentAmount, qint64 maxAmount);

	// called when all files were downloaded and manager goes to idle state
	// Lists contains files that were successfully downloaded / failed to download
	void finished(QStringList savedFiles, QStringList failedFiles, QStringList errors);
};
