/*
 * cdownloadmanager_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "cdownloadmanager_moc.h"

#include "../../vcmiqt/launcherdirs.h"

#include "../../lib/CConfigHandler.h"

CDownloadManager::CDownloadManager()
{
	connect(&manager, SIGNAL(finished(QNetworkReply *)),
		SLOT(downloadFinished(QNetworkReply *)));
	connect(&manager, &QNetworkAccessManager::sslErrors, [](QNetworkReply * reply, const QList<QSslError> & errors) {
		if(settings["launcher"]["ignoreSslErrors"].Bool())
			reply->ignoreSslErrors();
	});
}

void CDownloadManager::downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal)
{
	FileEntry entry;
	entry.url = url;
	entry.file.reset(new QFile(QString{QLatin1String{"%1/%2"}}.arg(CLauncherDirs::downloadsPath(), file)));
	entry.bytesReceived = 0;
	entry.totalSize = bytesTotal;
	entry.filename = file;
	entry.reply = nullptr;
	entry.status = FileEntry::QUEUED;

	if(entry.file->open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		// file prepared, download will start when this entry reaches queue head
	}
	else
	{
		entry.status = FileEntry::FAILED;
		encounteredErrors += entry.file->errorString();
	}

	// even if failed - add it into list to report it in finished() call
	currentDownloads.push_back(entry);
	startNextDownload();
}

CDownloadManager::FileEntry & CDownloadManager::getEntry(QNetworkReply * reply)
{
	assert(reply);
	for(auto & entry : currentDownloads)
	{
		if(entry.reply == reply)
			return entry;
	}
	throw std::runtime_error("Failed to find download entry");
}

void CDownloadManager::downloadFinished(QNetworkReply * reply)
{
	FileEntry & file = getEntry(reply);

	QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	QUrl redirectUrl = possibleRedirectUrl.toUrl();

	if(possibleRedirectUrl.isValid())
	{
		file.file->resize(0);
		file.file->seek(0);
		file.bytesReceived = 0;

		file.reply->deleteLater();
		file.reply = nullptr;
		file.url = reply->url().resolved(redirectUrl);
		startDownload(file);
		return;
	}

	if(file.reply->error())
	{
		encounteredErrors += file.reply->errorString();
		file.file->remove();
		file.status = FileEntry::FAILED;
	}
	else
	{
		file.file->write(file.reply->readAll());
		file.file->close();
		file.status = FileEntry::FINISHED;
	}

	Q_EMIT downloadFileFinished(file.filename);

	bool downloadComplete = true;
	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS || entry.status == FileEntry::QUEUED)
		{
			downloadComplete = false;
			break;
		}
	}

	QStringList successful;
	QStringList failed;

	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::FINISHED)
			successful += entry.file->fileName();
		else
			failed += entry.file->fileName();
	}

	if(downloadComplete)
		Q_EMIT finished(successful, failed, encounteredErrors);

	file.reply->deleteLater();
	file.reply = nullptr;
	startNextDownload();
}

void CDownloadManager::downloadProgressChanged(qint64 bytesReceived, qint64 bytesTotal)
{
	auto reply = dynamic_cast<QNetworkReply *>(sender());
	FileEntry & entry = getEntry(reply);

	entry.file->write(entry.reply->readAll());
	entry.bytesReceived = bytesReceived;
	if(bytesTotal > entry.totalSize)
		entry.totalSize = bytesTotal;

	quint64 total = 0;
	for(auto & entry : currentDownloads)
		total += entry.totalSize > 0 ? entry.totalSize : entry.bytesReceived;

	quint64 received = 0;
	for(auto & entry : currentDownloads)
		received += entry.bytesReceived > 0 ? entry.bytesReceived : 0;

	if(received > total)
		total = received;

	Q_EMIT downloadProgress(entry.filename, received, total);
}

bool CDownloadManager::downloadInProgress(const QUrl & url) const
{
	for(auto & entry : currentDownloads)
	{
		if(entry.url == url && (entry.status == FileEntry::QUEUED || entry.status == FileEntry::IN_PROGRESS))
			return true;
	}
	return false;
}

void CDownloadManager::startDownload(FileEntry & entry)
{
	QNetworkRequest request(entry.url);
	entry.reply = manager.get(request);
	entry.status = FileEntry::IN_PROGRESS;

	connect(entry.reply, SIGNAL(downloadProgress(qint64,qint64)),
		SLOT(downloadProgressChanged(qint64,qint64)));
}

void CDownloadManager::startNextDownload()
{
	if(hasDownloadInProgress())
		return;

	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::QUEUED)
		{
			startDownload(entry);
			break;
		}
	}
}

bool CDownloadManager::hasDownloadInProgress() const
{
	for(const auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS)
			return true;
	}
	return false;
}
