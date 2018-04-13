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

#include "../launcherdirs.h"

CDownloadManager::CDownloadManager()
{
	connect(&manager, SIGNAL(finished(QNetworkReply *)),
		SLOT(downloadFinished(QNetworkReply *)));
}

void CDownloadManager::downloadFile(const QUrl & url, const QString & file)
{
	QNetworkRequest request(url);
	FileEntry entry;
	entry.file.reset(new QFile(CLauncherDirs::get().downloadsPath() + '/' + file));
	entry.bytesReceived = 0;
	entry.totalSize = 0;

	if(entry.file->open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		entry.status = FileEntry::IN_PROGRESS;
		entry.reply = manager.get(request);

		connect(entry.reply, SIGNAL(downloadProgress(qint64,qint64)),
			SLOT(downloadProgressChanged(qint64,qint64)));
	}
	else
	{
		entry.status = FileEntry::FAILED;
		entry.reply = nullptr;
		encounteredErrors += entry.file->errorString();
	}

	// even if failed - add it into list to report it in finished() call
	currentDownloads.push_back(entry);
}

CDownloadManager::FileEntry & CDownloadManager::getEntry(QNetworkReply * reply)
{
	assert(reply);
	for(auto & entry : currentDownloads)
	{
		if(entry.reply == reply)
			return entry;
	}
	assert(0);
	static FileEntry errorValue;
	return errorValue;
}

void CDownloadManager::downloadFinished(QNetworkReply * reply)
{
	FileEntry & file = getEntry(reply);

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

	bool downloadComplete = true;
	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS)
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
		emit finished(successful, failed, encounteredErrors);

	file.reply->deleteLater();
	file.reply = nullptr;
}

void CDownloadManager::downloadProgressChanged(qint64 bytesReceived, qint64 bytesTotal)
{
	auto reply = dynamic_cast<QNetworkReply *>(sender());
	FileEntry & entry = getEntry(reply);

	entry.file->write(entry.reply->readAll());
	entry.bytesReceived = bytesReceived;
	entry.totalSize = bytesTotal;

	quint64 total = 0;
	for(auto & entry : currentDownloads)
		total += entry.totalSize > 0 ? entry.totalSize : 0;

	quint64 received = 0;
	for(auto & entry : currentDownloads)
		received += entry.bytesReceived > 0 ? entry.bytesReceived : 0;

	emit downloadProgress(received, total);
}

bool CDownloadManager::downloadInProgress(const QUrl & url)
{
	for(auto & entry : currentDownloads)
	{
		if(entry.reply->url() == url)
			return true;
	}
	return false;
}
