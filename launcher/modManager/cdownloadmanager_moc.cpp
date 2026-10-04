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

#include "../helper.h"
#include "../../vcmiqt/convpathqstring.h"
#include "../../vcmiqt/launcherdirs.h"

#include "../../lib/CConfigHandler.h"

CDownloadManager::CDownloadManager()
	: downloader(*this, settings["launcher"]["httpProxy"].String(), settings["launcher"]["ignoreSslErrors"].Bool())
{
	pollTimer.setInterval(5);
	connect(&pollTimer, &QTimer::timeout, this, [this](){ downloader.poll(); });
}

void CDownloadManager::downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal)
{
	FileEntry entry;
	entry.url = url;
	entry.filename = file;
	entry.filePath = QString{QLatin1String{"%1/%2"}}.arg(CLauncherDirs::downloadsPath(), file);
	entry.bytesReceived = 0;
	entry.totalSize = bytesTotal;
	entry.status = FileEntry::QUEUED;

	currentDownloads.push_back(entry);
	startNextDownload();
}

CDownloadManager::FileEntry & CDownloadManager::getActiveEntry()
{
	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS)
			return entry;
	}
	throw std::runtime_error("Failed to find active download entry");
}

void CDownloadManager::onDownloadFinished(const std::string & errorMessage)
{
	FileEntry & file = getActiveEntry();

	if(errorMessage.empty())
	{
		file.status = FileEntry::FINISHED;
	}
	else
	{
		encounteredErrors += QString::fromStdString(errorMessage);
		file.status = FileEntry::FAILED;
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
			successful += entry.filePath;
		else
			failed += entry.filePath;
	}

	if(downloadComplete)
	{
		pollTimer.stop();
		Q_EMIT finished(successful, failed, encounteredErrors);
	}

	startNextDownload();
}

void CDownloadManager::onDownloadProgress(uint64_t received, uint64_t total)
{
	FileEntry & entry = getActiveEntry();

	entry.bytesReceived = received;
	if(static_cast<qint64>(total) > entry.totalSize)
		entry.totalSize = total;

	quint64 totalAll = 0;
	for(const auto & download : currentDownloads)
		totalAll += download.totalSize > 0 ? download.totalSize : download.bytesReceived;

	quint64 receivedAll = 0;
	for(const auto & download : currentDownloads)
		receivedAll += download.bytesReceived > 0 ? download.bytesReceived : 0;

	if(receivedAll > totalAll)
		totalAll = receivedAll;

	Q_EMIT downloadProgress(entry.filename, receivedAll, totalAll);
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

void CDownloadManager::startNextDownload()
{
	if(hasDownloadInProgress())
		return;

	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::QUEUED)
		{
			entry.status = FileEntry::IN_PROGRESS;
			if(entry.url.isLocalFile())
			{
				// deferred, so callers of downloadFile receive results asynchronously, same as for network downloads
				QTimer::singleShot(0, this, &CDownloadManager::copyLocalFile);
			}
			else
			{
				downloader.start(entry.url.toEncoded().toStdString(), qstringToPath(entry.filePath));
				pollTimer.start();
			}
			break;
		}
	}
}

void CDownloadManager::copyLocalFile()
{
	const FileEntry & entry = getActiveEntry();
	// on Android, local path may be a content:// URI that only performNativeCopy can read
	const QString sourcePath = entry.url.toLocalFile();

	if(Helper::performNativeCopy(sourcePath, entry.filePath))
		onDownloadFinished({});
	else
		onDownloadFinished(tr("Failed to copy file %1").arg(Helper::getRealPath(sourcePath)).toStdString());
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
