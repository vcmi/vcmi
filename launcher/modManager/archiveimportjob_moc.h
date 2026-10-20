/*
 * archiveimportjob_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../importsource.h"
#include "modarchiveinstaller.h"

#include <QStringList>
#include <QThread>
#include <QVector>

#include <atomic>
#include <memory>

class QTemporaryDir;

struct ArchiveImportSource
{
	ImportSource file;
	QString modIdHint;
	std::shared_ptr<QTemporaryDir> temporaryDirectory;
};

struct ArchiveImportResult
{
	QStringList installedMods;
	QStringList errors;
	QVector<ArchiveImportSource> mapArchives;
	QVector<ArchiveImportSource> saveArchives;
	bool canceled = false;
};

/// Owns all import I/O. UI and mod state are only accessed by the caller.
class ArchiveImportJob final : public QThread
{
	Q_OBJECT

	QVector<ArchiveImportSource> sources;
	ModArchiveInstaller::Request request;
	QString cachePath;
	std::atomic<bool> canceled{false};
	ArchiveImportResult outcome;

	void run() override;
	void reportProgress(const QString & description, quint64 current, quint64 total);
	bool prepareSource(ArchiveImportSource & source);

public:
	ArchiveImportJob(QVector<ArchiveImportSource> sources, ModArchiveInstaller::Request request, QString cachePath, QObject * parent);
	~ArchiveImportJob() override;

	void cancel();
	const ArchiveImportResult & result() const;

signals:
	void progress(QString description, quint64 current, quint64 total);
};
