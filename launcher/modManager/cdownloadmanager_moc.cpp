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
#include "../../lib/network/HttpDownloader.h"

#include <QPointer>
#include <QTimer>
#include <QVector>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <tuple>
#include <vector>

static constexpr int MAX_PARALLEL_DOWNLOADS = 20;
static constexpr qint64 COPY_BUFFER_SIZE = 1024 * 1024;
static constexpr int COPY_CHUNKS_PER_TURN = 4;
static constexpr qint64 PROGRESS_INTERVAL_MS = 50;

class DownloadWorker final : public QObject, public IHttpDownloaderListener
{
	struct FileEntry
	{
		QUrl url;
		QString filename;
		QString filePath;
		std::shared_ptr<void> access;
		std::unique_ptr<QFile> input;
		std::unique_ptr<QFile> output;
		qint64 bytesReceived = 0;
		qint64 totalSize = 0;
		qint64 sourceSize = 0;
		bool local = false;
		bool active = false;
		bool ownsOutput = false;
	};

	CDownloadManager * owner;
	std::string proxy;
	bool ignoreSslErrors;
	QTimer * pollTimer = nullptr;
	std::unique_ptr<HttpDownloader> downloader;
	std::map<quint64, std::unique_ptr<FileEntry>> entries;
	std::map<HttpDownloadID, quint64> httpRequests;
	std::map<quint64, QString> completedArchives;
	std::array<char, COPY_BUFFER_SIZE> buffer;
	quint64 lastCopiedID = 0;
	QString currentFile;
	QString httpError;
	QElapsedTimer progressClock;
	bool stopping = false;

	static qint64 toSigned(uint64_t bytes)
	{
		return static_cast<qint64>(std::min(bytes, static_cast<uint64_t>(std::numeric_limits<qint64>::max())));
	}

	static void removeArchive(const QString & path)
	{
		QFile file(path);
		if(file.exists() && !file.remove())
			logGlobal->warn("Failed to remove canceled download '%s': %s", path.toStdString(), file.errorString().toStdString());
	}

	void complete(quint64 requestID, const QString & error, bool certificateError = false)
	{
		auto & entry = *entries.at(requestID);
		const auto received = entry.bytesReceived;
		const auto total = entry.totalSize;
		if(entry.input)
			entry.input->close();
		if(entry.output)
			entry.output->close();
		if(!error.isEmpty() && entry.ownsOutput)
			entry.output->remove();
		if(error.isEmpty() && entry.filePath.endsWith(".zip", Qt::CaseInsensitive))
			completedArchives.emplace(requestID, entry.filePath);
		entries.erase(requestID);
		QMetaObject::invokeMethod(owner, [owner = owner, requestID, received, total, error, certificateError]
		{
			owner->downloadCompleted(requestID, received, total, error, certificateError);
		}, Qt::QueuedConnection);
	}

	void startEntry(quint64 requestID)
	{
		auto & entry = *entries.at(requestID);
		entry.active = true;
		if(!entry.local)
		{
			if(!httpError.isEmpty())
			{
				complete(requestID, httpError);
				return;
			}
			try
			{
				if(!downloader)
					downloader = std::make_unique<HttpDownloader>(*this, proxy, ignoreSslErrors);
				const auto downloadID = downloader->start(entry.url.toEncoded().toStdString(), qstringToPath(entry.filePath));
				httpRequests.emplace(downloadID, requestID);
			}
			catch(const std::runtime_error & exception)
			{
				complete(requestID, CDownloadManager::tr("Failed to download %1: %2").arg(entry.filename, QString::fromUtf8(exception.what())));
			}
			return;
		}

		const auto sourcePath = entry.url.isLocalFile() ? entry.url.toLocalFile() : entry.url.toString();
		const auto destination = QFileInfo(entry.filePath).canonicalFilePath();
		if(!destination.isEmpty() && destination == QFileInfo(sourcePath).canonicalFilePath())
		{
			complete(requestID, CDownloadManager::tr("The download source and destination are the same file"));
			return;
		}
		entry.input = std::make_unique<QFile>(Helper::importSourcePath(sourcePath));
		if(!entry.input->open(QIODevice::ReadOnly))
		{
			complete(requestID, CDownloadManager::tr("Failed to read %1: %2").arg(entry.filename, entry.input->errorString()));
			return;
		}
		entry.output = std::make_unique<QFile>(entry.filePath);
		if(!entry.output->open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			complete(requestID, CDownloadManager::tr("Failed to write %1: %2").arg(entry.filename, entry.output->errorString()));
			return;
		}
		entry.ownsOutput = true;
		entry.sourceSize = entry.input->size();
		entry.totalSize = std::max(entry.totalSize, entry.sourceSize);
	}

	void startNextDownloads()
	{
		auto active = std::count_if(entries.begin(), entries.end(), [](const auto & value)
		{
			return value.second->active;
		});
		for(auto it = entries.begin(); it != entries.end() && active < MAX_PARALLEL_DOWNLOADS;)
		{
			if(it->second->active)
			{
				++it;
				continue;
			}
			const auto requestID = it->first;
			++it;
			startEntry(requestID);
			if(entries.count(requestID))
				++active;
		}
	}

	void copyChunk(quint64 requestID)
	{
		auto & entry = *entries.at(requestID);
		currentFile = entry.filename;
		const auto amount = entry.input->read(buffer.data(), buffer.size());
		if(amount < 0)
		{
			complete(requestID, CDownloadManager::tr("Failed to read %1: %2").arg(entry.filename, entry.input->errorString()));
			return;
		}
		if(amount == 0)
		{
			if(entry.sourceSize > 0 && entry.bytesReceived != entry.sourceSize)
				complete(requestID, CDownloadManager::tr("Failed to copy complete file: %1").arg(entry.filename));
			else if(!entry.output->flush())
				complete(requestID, CDownloadManager::tr("Failed to write %1: %2").arg(entry.filename, entry.output->errorString()));
			else
				complete(requestID, {});
			return;
		}
		qint64 written = 0;
		while(written < amount)
		{
			const auto result = entry.output->write(buffer.data() + written, amount - written);
			if(result <= 0)
			{
				complete(requestID, CDownloadManager::tr("Failed to write %1: %2").arg(entry.filename, entry.output->errorString()));
				return;
			}
			written += result;
		}
		entry.bytesReceived += written;
	}

	void copyLocalFiles()
	{
		QElapsedTimer turnClock;
		turnClock.start();
		for(int count = 0; count < COPY_CHUNKS_PER_TURN && !entries.empty(); ++count)
		{
			auto it = entries.upper_bound(lastCopiedID);
			if(it == entries.end())
				it = entries.begin();
			const auto first = it;
			while(!it->second->active || !it->second->local)
			{
				if(++it == entries.end())
					it = entries.begin();
				if(it == first)
					break;
			}
			if(!it->second->active || !it->second->local)
				return;
			lastCopiedID = it->first;
			copyChunk(lastCopiedID);
			if(turnClock.elapsed() >= 8)
				break;
		}
	}

	void reportProgress()
	{
		if(entries.empty() || (progressClock.isValid() && progressClock.elapsed() < PROGRESS_INTERVAL_MS))
			return;
		progressClock.start();
		std::vector<std::tuple<quint64, qint64, qint64>> progress;
		for(const auto & [requestID, entry] : entries)
			progress.emplace_back(requestID, entry->bytesReceived, entry->totalSize);
		QMetaObject::invokeMethod(owner, [owner = owner, progress = std::move(progress), filename = currentFile]
		{
			for(const auto & [requestID, received, total] : progress)
				owner->updateProgress(requestID, received, total);
			owner->currentFile = filename;
			owner->emitProgress();
		}, Qt::QueuedConnection);
	}

	void poll()
	{
		startNextDownloads();
		if(downloader && downloader->isActive())
		{
			try
			{
				downloader->poll();
			}
			catch(const std::runtime_error & exception)
			{
				httpError = QString::fromUtf8(exception.what());
				downloader->cancel();
				const auto failed = std::exchange(httpRequests, {});
				for(const auto & [downloadID, requestID] : failed)
					complete(requestID, httpError);
			}
		}
		copyLocalFiles();
		startNextDownloads();
		reportProgress();
		if(entries.empty())
			pollTimer->stop();
	}

	void onDownloadProgress(HttpDownloadID download, uint64_t received, uint64_t total) override
	{
		auto & entry = *entries.at(httpRequests.at(download));
		entry.bytesReceived = toSigned(received);
		entry.totalSize = std::max(entry.totalSize, toSigned(total));
		currentFile = entry.filename;
	}

	void onDownloadFinished(HttpDownloadID download, const std::string & errorMessage, bool certificateError) override
	{
		const auto requestID = httpRequests.at(download);
		httpRequests.erase(download);
		complete(requestID, QString::fromStdString(errorMessage), certificateError);
	}

public:
	DownloadWorker(CDownloadManager * owner, std::string proxy, bool ignoreSslErrors)
		: owner(owner)
		, proxy(std::move(proxy))
		, ignoreSslErrors(ignoreSslErrors)
	{
	}

	void downloadFile(quint64 requestID, const QUrl & url, const QString & filename, const QString & path, qint64 bytesTotal, std::shared_ptr<void> access)
	{
		if(stopping)
			return;
		if(!pollTimer)
		{
			pollTimer = new QTimer(this);
			pollTimer->setInterval(5);
			connect(pollTimer, &QTimer::timeout, this, [this]
			{
				poll();
			});
		}
		auto entry = std::make_unique<FileEntry>();
		entry->url = url;
		entry->filename = filename;
		entry->filePath = path;
		entry->totalSize = std::max(qint64(0), bytesTotal);
		entry->local = url.isLocalFile() || url.scheme() == QLatin1String("content");
		entry->access = std::move(access);
		entries.emplace(requestID, std::move(entry));
		pollTimer->start();
	}

	void releaseCompletedArchives(const QVector<quint64> & requestIDs)
	{
		for(const auto requestID : requestIDs)
			completedArchives.erase(requestID);
	}

	void cancelAll()
	{
		stopping = true;
		if(pollTimer)
			pollTimer->stop();
		if(downloader)
		{
			downloader->cancel();
			downloader.reset();
		}
		for(auto & [requestID, entry] : entries)
		{
			if(entry->input)
				entry->input->close();
			if(entry->output)
				entry->output->close();
			if(entry->ownsOutput)
				entry->output->remove();
		}
		entries.clear();
		httpRequests.clear();
		for(const auto & [requestID, path] : completedArchives)
			removeArchive(path);
		completedArchives.clear();
	}
};

CDownloadManager::CDownloadManager(QObject * parent)
	: QObject(parent)
	, worker(new DownloadWorker(this, settings["launcher"]["httpProxy"].String(), settings["launcher"]["ignoreSslErrors"].Bool()))
	, downloadsDirectory(CLauncherDirs::downloadsPath())
{
	worker->moveToThread(&workerThread);
	connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
	workerThread.start();
}

CDownloadManager::~CDownloadManager()
{
	QMetaObject::invokeMethod(worker, [worker = worker]
	{
		worker->cancelAll();
	}, Qt::BlockingQueuedConnection);
	workerThread.quit();
	workerThread.wait();
}

void CDownloadManager::downloadFile(const QUrl & url, const QString & file, qint64 bytesTotal)
{
	const auto requestID = ++nextRequestID;
	const auto path = QString{QLatin1String{"%1/%2"}}.arg(downloadsDirectory, file);
	currentDownloads.emplace(requestID, FileEntry{url, file, path, 0, std::max(qint64(0), bytesTotal), true, {}});
	const auto source = url.isLocalFile() ? url.toLocalFile() : url.toString();
	auto access = url.isLocalFile() ? Helper::acquireImportAccess(source) : std::shared_ptr<void>{};
	QMetaObject::invokeMethod(worker, [worker = worker, requestID, url, file, path, bytesTotal, access = std::move(access)]
	{
		worker->downloadFile(requestID, url, file, path, bytesTotal, access);
	}, Qt::QueuedConnection);
}

void CDownloadManager::updateProgress(quint64 requestID, qint64 received, qint64 total)
{
	auto it = currentDownloads.find(requestID);
	if(it == currentDownloads.end())
		return;
	it->second.bytesReceived = received;
	it->second.totalSize = std::max(received, total);
}

void CDownloadManager::emitProgress()
{
	if(progressClock.isValid() && progressClock.elapsed() < PROGRESS_INTERVAL_MS)
		return;
	progressClock.start();
	qint64 received = 0;
	qint64 total = 0;
	for(const auto & [requestID, entry] : currentDownloads)
	{
		received += entry.bytesReceived;
		total += entry.totalSize;
	}
	Q_EMIT downloadProgress(currentFile, received, total);
}

void CDownloadManager::downloadCompleted(quint64 requestID, qint64 received, qint64 total, const QString & error, bool certificateError)
{
	auto & entry = currentDownloads.at(requestID);
	updateProgress(requestID, received, total);
	entry.active = false;
	entry.error = error;
	currentFile = entry.filename;
	if(!error.isEmpty())
		encounteredErrors.push_back(error);
	certificateErrorEncountered = certificateErrorEncountered || certificateError;
	if(!completionScheduled)
	{
		completionScheduled = true;
		QTimer::singleShot(0, this, &CDownloadManager::finishDownloads);
	}
	QPointer<CDownloadManager> guard(this);
	Q_EMIT downloadFileFinished(entry.filename);
	if(guard)
		emitProgress();
}

void CDownloadManager::finishDownloads()
{
	completionScheduled = false;
	qint64 received = 0;
	qint64 total = 0;
	QVector<quint64> completedIDs;
	QStringList successful;
	QStringList failed;
	for(const auto & [requestID, entry] : currentDownloads)
	{
		if(entry.active)
			return;
		received += entry.bytesReceived;
		total += entry.totalSize;
		completedIDs.push_back(requestID);
		if(entry.error.isEmpty())
			successful.push_back(entry.filePath);
		else
			failed.push_back(entry.filePath);
	}
	auto errors = std::exchange(encounteredErrors, {});
	currentDownloads.clear();
	QPointer<CDownloadManager> guard(this);
	Q_EMIT downloadProgress(currentFile, received, total);
	if(guard)
	{
		// Release precedes a destructor's queued cancelAll after the GUI takes ownership.
		QMetaObject::invokeMethod(worker, [worker = worker, completedIDs]
		{
			worker->releaseCompletedArchives(completedIDs);
		}, Qt::QueuedConnection);
		Q_EMIT finished(successful, failed, errors);
	}
}

bool CDownloadManager::downloadInProgress(const QUrl & url) const
{
	for(const auto & [requestID, entry] : currentDownloads)
	{
		if(entry.active && entry.url == url)
			return true;
	}
	return false;
}

bool CDownloadManager::hasCertificateErrors() const
{
	return certificateErrorEncountered;
}
