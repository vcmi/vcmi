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

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QThread>
#include <QUrl>

#include <map>

class DownloadWorker;

class CDownloadManager : public QObject
{
	Q_OBJECT

	friend class DownloadWorker;

	struct FileEntry
	{
		QUrl url;
		QString filename;
		QString filePath;
		qint64 bytesReceived = 0;
		qint64 totalSize = 0;
		bool active = true;
		QString error;
	};

	QThread workerThread;
	DownloadWorker * worker;
	QString downloadsDirectory;
	std::map<quint64, FileEntry> currentDownloads;
	quint64 nextRequestID = 0;
	QStringList encounteredErrors;
	QString currentFile;
	QElapsedTimer progressClock;
	bool certificateErrorEncountered = false;
	bool completionScheduled = false;

	void updateProgress(quint64 requestID, qint64 received, qint64 total);
	void emitProgress();
	void downloadCompleted(quint64 requestID, qint64 received, qint64 total, const QString & error, bool certificateError);
	void finishDownloads();

public:
	explicit CDownloadManager(QObject * parent = nullptr);
	~CDownloadManager() override;

	// returns true if download with such URL is in progress/queued
	bool downloadInProgress(const QUrl & url) const;

	void downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal = 0);

	/// returns true if any download failed because certificate of the server could not be verified
	bool hasCertificateErrors() const;

signals:
	// for status bar updates. Merges all queued downloads into one
	void downloadProgress(QString currentFile, qint64 currentAmount, qint64 maxAmount);
	void downloadFileFinished(QString fileName);

	// called when all files were downloaded and manager goes to idle state
	// Lists contains files that were successfully downloaded / failed to download
	void finished(QStringList savedFiles, QStringList failedFiles, QStringList errors);
};
