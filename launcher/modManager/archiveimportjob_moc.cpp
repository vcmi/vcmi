/*
 * archiveimportjob_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "archiveimportjob_moc.h"
#include "modarchiveinstaller.h"

#include "../helper.h"

#include <QElapsedTimer>

ArchiveImportJob::ArchiveImportJob(QVector<ArchiveImportSource> sources, ModArchiveInstaller::Request request, QString cachePath, QObject * parent)
	: QThread(parent)
	, sources(std::move(sources))
	, request(std::move(request))
	, cachePath(std::move(cachePath))
{
}

ArchiveImportJob::~ArchiveImportJob()
{
	cancel();
	wait();
}

void ArchiveImportJob::cancel()
{
	canceled.store(true);
}

const ArchiveImportResult & ArchiveImportJob::result() const
{
	return outcome;
}

void ArchiveImportJob::reportProgress(const QString & description, quint64 current, quint64 total)
{
	QMetaObject::invokeMethod(this, [this, description, current, total]
	{
		Q_EMIT progress(description, current, total);
	}, Qt::QueuedConnection);
}

bool ArchiveImportJob::prepareSource(ArchiveImportSource & source)
{
	bool needsCopy = source.file.path.startsWith(QLatin1String("content://"), Qt::CaseInsensitive);
#ifdef VCMI_IOS
	// Qt's import picker normally returns an application-local copy. Other
	// callers can supply an external URL; keep its access token until copied.
	needsCopy = needsCopy || !QFileInfo(source.file.path).absoluteFilePath().startsWith(QDir::homePath() + '/');
#endif
	if(!needsCopy)
		return true;

	if(!QDir().mkpath(cachePath))
	{
		outcome.errors.push_back(tr("Failed to create import cache: %1").arg(cachePath));
		return false;
	}
	auto temporary = std::make_shared<QTemporaryDir>(QDir(cachePath).filePath("import-XXXXXX"));
	if(!temporary->isValid())
	{
		outcome.errors.push_back(tr("Failed to create import cache: %1").arg(cachePath));
		return false;
	}

	QFile input(Helper::importSourcePath(source.file.path));
	QFile output(temporary->filePath("archive.zip"));
	if(!input.open(QIODevice::ReadOnly))
	{
#ifdef VCMI_ANDROID
		// Retain the picker fallback for providers that Qt cannot open.
		reportProgress(tr("Preparing archive. %p%"), 0, 0);
		if(Helper::performNativeCopy(source.file.path, output.fileName()) && !canceled.load())
		{
			source.file.path = output.fileName();
			source.temporaryDirectory = std::move(temporary);
			return true;
		}
#endif
		outcome.errors.push_back(tr("Failed to copy %1: %2").arg(source.file.path, input.errorString()));
		return false;
	}
	if(!output.open(QIODevice::WriteOnly))
	{
		outcome.errors.push_back(tr("Failed to copy %1: %2").arg(source.file.path, output.errorString()));
		return false;
	}

	const qint64 size = input.size();
	quint64 copied = 0;
	QByteArray buffer(1024 * 1024, Qt::Uninitialized);
	QElapsedTimer elapsed;
	elapsed.start();
	reportProgress(tr("Preparing archive. %p%"), 0, size > 0 ? size : 0);
	while(!canceled.load())
	{
		const qint64 count = input.read(buffer.data(), buffer.size());
		if(count < 0)
		{
			outcome.errors.push_back(tr("Failed to read %1: %2").arg(source.file.path, input.errorString()));
			return false;
		}
		if(count == 0)
			break;
		qint64 written = 0;
		while(written < count)
		{
			const qint64 result = output.write(buffer.constData() + written, count - written);
			if(result <= 0)
			{
				outcome.errors.push_back(tr("Failed to write import cache: %1").arg(output.errorString()));
				return false;
			}
			written += result;
		}
		copied += static_cast<quint64>(count);
		if(elapsed.elapsed() >= 100)
		{
			reportProgress(tr("Preparing archive. %p%"), copied, size > 0 ? size : 0);
			elapsed.restart();
		}
	}
	if(canceled.load())
		return false;
	if((size > 0 && copied != static_cast<quint64>(size)) || !output.flush())
	{
		outcome.errors.push_back(tr("Failed to copy complete archive: %1").arg(source.file.path));
		return false;
	}
	output.close();
	source.file.path = output.fileName();
	source.temporaryDirectory = std::move(temporary);
	return true;
}

void ArchiveImportJob::run()
{
	QElapsedTimer elapsed;
	elapsed.start();
	for(auto & queuedSource : sources)
	{
		if(canceled.load())
			break;
		auto source = std::move(queuedSource);
		try
		{
			if(!prepareSource(source))
				continue;

			request.archivePath = source.file.path;
			request.modIdHint = source.modIdHint;
			QElapsedTimer progressTimer;
			progressTimer.start();
			auto report = [this, &progressTimer](const ModArchiveInstaller::Progress & value)
			{
				if(value.completedBytes != 0 && value.completedBytes != value.totalBytes && progressTimer.elapsed() < 100)
					return;
				progressTimer.restart();
				QString description;
				switch(value.stage)
				{
					case ModArchiveInstaller::Stage::Inspecting:
						description = tr("Inspecting archive...");
						break;
					case ModArchiveInstaller::Stage::Extracting:
						description = tr("Installing mod %1 (%2/%3). %p%").arg(value.modId).arg(value.completedMods + 1).arg(value.totalMods);
						break;
					case ModArchiveInstaller::Stage::ExtractingContent:
						description = tr("Extracting content.zip for %1. %p%").arg(value.modId);
						break;
					case ModArchiveInstaller::Stage::Committing:
						description = tr("Finishing mod %1...").arg(value.modId);
						break;
				}
				reportProgress(description, value.completedBytes, value.totalBytes);
			};
			const auto result = ModArchiveInstaller::install(request, report, [this] { return canceled.load(); });
			outcome.installedMods.append(result.installedMods);
			outcome.errors.append(result.errors);
			if(result.mapArchive && !canceled.load())
				outcome.mapArchives.push_back(std::move(source));
			else if(result.saveArchive && !canceled.load())
				outcome.saveArchives.push_back(std::move(source));
			if(result.canceled)
				canceled.store(true);
		}
		catch(const std::exception & exception)
		{
			outcome.errors.push_back(tr("Failed to import %1: %2").arg(source.file.path, QString::fromUtf8(exception.what())));
		}
	}
	// Release every unprocessed source on cancellation as well. Map sources
	// have transferred their shared lifetime to the result for GUI consumption.
	sources.clear();
	outcome.canceled = canceled.load();
	logGlobal->info("Archive import finished in %d ms: %d mods installed", elapsed.elapsed(), outcome.installedMods.size());
}
