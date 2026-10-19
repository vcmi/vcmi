/*
 * modarchiveinstaller.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QStringList>
#include <functional>

/// File-system-only installation work. All callbacks run in the calling thread.
/// The caller owns the source archive and is responsible for refreshing mod state.
namespace ModArchiveInstaller
{
enum class Stage
{
	Inspecting,
	Extracting,
	ExtractingContent,
	Committing
};

struct Request
{
	QString archivePath;
	QString modsPath;
	/// Repository ID or the existing single-mod filename convention. Ignored for collections.
	QString modIdHint;
	QStringList protectedModIds;
	bool fullModExtraction = false;
};

struct Progress
{
	Stage stage = Stage::Inspecting;
	QString modId;
	quint64 completedBytes = 0;
	quint64 totalBytes = 0;
	int completedMods = 0;
	int totalMods = 0;
};

struct Result
{
	QStringList installedMods;
	QStringList errors;
	bool canceled = false;
	bool mapArchive = false;
	bool saveArchive = false;
};

Result install(const Request & request,
	const std::function<void(const Progress &)> & progress = {},
	const std::function<bool()> & cancel = {});
}
