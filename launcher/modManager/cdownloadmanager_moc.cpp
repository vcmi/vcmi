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

static constexpr int MAX_PARALLEL_DOWNLOADS = 20;

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
	entry.downloadID = 0;

	entry.bytesReceived = 0;
	entry.totalSize = bytesTotal;
	entry.status = FileEntry::QUEUED;

	if(!downloadsPaused)
		openDownloadFile(entry);

	// even if failed - add it into list to report it in finished() call
	currentDownloads.push_back(entry);
	startNextDownloads();

	currentDownloads.push_back(entry);
	startNextDownloads();
}

bool CDownloadManager::openDownloadFile(FileEntry & entry)
{
	QDir().mkpath(CLauncherDirs::downloadsPath());
	entry.file.reset(new QFile(QString{QLatin1String{"%1/%2"}}.arg(CLauncherDirs::downloadsPath(), entry.filename)));

	if(entry.file->open(QIODevice::WriteOnly | QIODevice::Truncate))
		return true;

	entry.status = FileEntry::FAILED;
	encounteredErrors += entry.file->errorString();
	return false;
}

CDownloadManager::FileEntry & CDownloadManager::getEntry(QNetworkReply * reply)
{
	for(auto & entry : currentDownloads)
	{
		if(entry.reply == reply)
			return entry;
	}

	throw std::runtime_error("No matching download entry found");
}

CDownloadManager::FileEntry & CDownloadManager::getEntry(HttpDownloadID download)

{
	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS && !entry.url.isLocalFile() && entry.downloadID == download)
			return entry;
	}
	throw std::runtime_error("Failed to find download entry " + std::to_string(download));
}

void CDownloadManager::onDownloadFinished(HttpDownloadID download, const std::string & errorMessage, bool certificateError)
{
	if(ignoredReplies.remove(reply))
	{
		reply->deleteLater();
		return;
	}

	FileEntry & file = getEntry(reply);

	if(certificateError)
		certificateErrorEncountered = true;

	finishEntry(file, errorMessage);


void CDownloadManager::finishEntry(FileEntry & file, const std::string & errorMessage)
{
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

	startNextDownloads();
}

void CDownloadManager::onDownloadProgress(HttpDownloadID download, uint64_t received, uint64_t total)
{
	FileEntry & entry = getEntry(download);

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

void CDownloadManager::pauseDownloads()
{
	if(downloadsPaused)
		return;

	downloadsPaused = true;
	for(auto & entry : currentDownloads)
	{
		if(entry.status != FileEntry::QUEUED && entry.status != FileEntry::IN_PROGRESS)
			continue;

		if(entry.reply)
		{
			disconnect(entry.reply, nullptr, this, nullptr);
			ignoredReplies.insert(entry.reply);
			entry.reply->abort();
			entry.reply->deleteLater();
			entry.reply = nullptr;
		}
		if(entry.file)
		{
			entry.file->close();
			entry.file->remove();
			entry.file.reset();
		}
		entry.status = FileEntry::QUEUED;
		entry.bytesReceived = 0;
	}
}

void CDownloadManager::resumeDownloads()
{
	if(!downloadsPaused)
		return;

	downloadsPaused = false;
	for(auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::QUEUED)
			openDownloadFile(entry);
	}
	startNextDownload();
}

void CDownloadManager::startDownload(FileEntry & entry)

{
	int downloadsInProgress = countDownloadsInProgress();

	connect(entry.reply, SIGNAL(downloadProgress(qint64,qint64)),
		SLOT(downloadProgressChanged(qint64,qint64)));
}

void CDownloadManager::startNextDownload()
{
	if(downloadsPaused)
		return;

	if(hasDownloadInProgress())
		return;

	for(auto & entry : currentDownloads)
	{
		if(entry.status != FileEntry::QUEUED)
			continue;
	}
}

void CDownloadManager::startNextDownloads()
{
	int downloadsInProgress = countDownloadsInProgress();

	for(qsizetype index = 0; index < currentDownloads.size() && downloadsInProgress < MAX_PARALLEL_DOWNLOADS; ++index)
	{
		FileEntry & entry = currentDownloads[index];
		if(entry.status != FileEntry::QUEUED)
			continue;
	}

	{
		FileEntry & entry = currentDownloads[index];
		if(entry.status != FileEntry::QUEUED)
			continue;

		entry.status = FileEntry::IN_PROGRESS;
		++downloadsInProgress;

		if(entry.url.isLocalFile())
		{
			// deferred, so callers of downloadFile receive results asynchronously, same as for network downloads
			// entries are never removed from the list, so index remains valid
			QTimer::singleShot(0, this, [this, index](){ copyLocalFile(currentDownloads[index]); });
		}
		else
		{
			entry.downloadID = downloader.start(entry.url.toEncoded().toStdString(), qstringToPath(entry.filePath));
			pollTimer.start();
		}
	}
}

void CDownloadManager::copyLocalFile(FileEntry & entry)
{
	// on Android, local path may be a content:// URI that only performNativeCopy can read
	const QString sourcePath = entry.url.toLocalFile();

	if(Helper::performNativeCopy(sourcePath, entry.filePath))
		finishEntry(entry, {});
	else
		finishEntry(entry, tr("Failed to copy file %1").arg(Helper::getRealPath(sourcePath)).toStdString());
}

bool CDownloadManager::hasCertificateErrors() const
{
	return certificateErrorEncountered;
}

int CDownloadManager::countDownloadsInProgress() const
{
	int result = 0;
	for(const auto & entry : currentDownloads)
	{
		if(entry.status == FileEntry::IN_PROGRESS)
			++result;
	}
	return result;
}
