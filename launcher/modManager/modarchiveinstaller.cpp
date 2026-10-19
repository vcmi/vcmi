/*
 * modarchiveinstaller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modarchiveinstaller.h"

#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/ModScope.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QHash>
#include <QSet>

#include <limits>

namespace ModArchiveInstaller
{
namespace
{
struct InstallError
{
	QString message;
};

struct Canceled
{
};

[[noreturn]] void fail(const QString & message)
{
	throw InstallError{message};
}

QString pathKey(const QString & path)
{
	return path.normalized(QString::NormalizationForm_C).toCaseFolded();
}

bool isPortableComponent(const QString & component)
{
	if(component.isEmpty() || component == "." || component == ".."
		|| component.endsWith('.') || component.endsWith(' '))
		return false;
	for(const auto character : component)
	{
		if(character.unicode() < 32 || QStringLiteral("<>:\"/\\|?*").contains(character))
			return false;
	}

	const auto deviceName = component.section('.', 0, 0).toLower();
	if(deviceName == "con" || deviceName == "prn" || deviceName == "aux" || deviceName == "nul")
		return false;
	if(deviceName.size() == 4 && (deviceName.startsWith("com") || deviceName.startsWith("lpt"))
		&& deviceName.back() >= QChar{'1'} && deviceName.back() <= QChar{'9'})
		return false;
	return true;
}

bool isValidModId(const QString & id)
{
	return isPortableComponent(id) && !id.contains('.')
		&& id != "mapeditor" && !ModScope::isScopeReserved(id.toStdString());
}

struct ArchiveEntry
{
	QString path;
	QStringList parts;
	bool directory = false;
};

/// Validate every entry before writing anything. Include implicit directories so that
/// spelling differences cannot produce different installations on Linux and Windows.
std::vector<ArchiveEntry> inspectEntries(ZipArchive & archive)
{
	std::vector<ArchiveEntry> result;
	QHash<QString, QString> spellings;
	QHash<QString, bool> kinds;
	QSet<QString> explicitEntries;
	for(const auto & entry : archive.entries())
	{
		const QByteArray encoded(entry.name.data(), static_cast<int>(entry.name.size()));
		QString path = QString::fromUtf8(encoded);
		if(path.toUtf8() != encoded || path.contains(QChar{0}))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an invalid UTF-8 filename"));
		path.replace('\\', '/');
		if(path.startsWith('/'))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an absolute path: %1").arg(path));

		QStringList parts;
		for(const auto & part : path.split('/'))
		{
			if(part.isEmpty() || part == ".")
				continue;
			if(!isPortableComponent(part))
				fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an unsafe or unsupported path: %1").arg(path));
			parts.push_back(part);
		}
		if(parts.empty())
		{
			if(entry.directory)
			{
				result.push_back({{}, {}, true});
				continue;
			}
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an empty filename"));
		}
		if(entry.symlink)
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains a symbolic link: %1").arg(path));

		path = parts.join('/');
		QString prefix;
		for(int index = 0; index < parts.size(); ++index)
		{
			if(!prefix.isEmpty())
				prefix += '/';
			prefix += parts[index];
			const QString key = pathKey(prefix);
			const bool directory = index + 1 != parts.size() || entry.directory;
			if(spellings.contains(key) && spellings.value(key) != prefix)
				fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains filenames that differ only in case or Unicode spelling: %1").arg(prefix));
			if(kinds.contains(key) && kinds.value(key) != directory)
				fail(QCoreApplication::translate("ModArchiveInstaller", "Archive uses the same path for a file and a directory: %1").arg(prefix));
			spellings.insert(key, prefix);
			kinds.insert(key, directory);
		}
		const auto key = pathKey(path);
		if(explicitEntries.contains(key))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains a duplicate path: %1").arg(path));
		explicitEntries.insert(key);
		result.push_back({path, parts, entry.directory});
	}
	return result;
}

struct ModPlan
{
	QString prefix;
	QString id;
	std::vector<size_t> entries;
	QStringList manifests;
};

QString relativePath(const ModPlan & mod, const ArchiveEntry & entry)
{
	return mod.prefix.isEmpty() ? entry.path : entry.path.mid(mod.prefix.size() + 1);
}

bool isModManifest(const ArchiveEntry & entry)
{
	return !entry.directory && !entry.parts.empty()
		&& entry.parts.back().compare("mod.json", Qt::CaseInsensitive) == 0;
}

bool isSubmodManifest(const QString & path)
{
	const auto parts = path.split('/');
	if(parts.size() % 2 != 1)
		return false;
	for(int index = 0; index + 1 < parts.size(); index += 2)
	{
		if(parts[index].compare("Mods", Qt::CaseInsensitive) != 0)
			return false;
		if(!isValidModId(parts[index + 1].toLower()))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an invalid submod ID: %1").arg(parts[index + 1]));
	}
	return true;
}

std::vector<ModPlan> makePlan(const Request & request, const std::vector<ArchiveEntry> & entries, Result & result)
{
	std::vector<size_t> manifests;
	std::vector<size_t> roots;
	bool collection = false;
	bool hasMaps = false;
	bool hasSaves = false;
	for(size_t index = 0; index < entries.size(); ++index)
	{
		const auto & entry = entries[index];
		if(isModManifest(entry))
			manifests.push_back(index);
		if(!entry.directory)
		{
			const auto path = entry.path.toLower();
			hasMaps |= path.endsWith(".h3m") || path.endsWith(".h3c") || path.endsWith(".vmap") || path.endsWith(".vcmp");
			hasSaves |= path.endsWith(".vsgm1");
		}
	}
	if(manifests.empty())
	{
		result.mapArchive = hasMaps;
		result.saveArchive = !hasMaps && hasSaves;
		if(!result.mapArchive && !result.saveArchive)
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive does not contain mod metadata, maps, or saves"));
		return {};
	}

	// A root manifest always takes precedence over nested submods.
	for(const auto index : manifests)
	{
		if(entries[index].parts.size() == 1)
			roots.push_back(index);
	}
	if(roots.empty())
	{
		for(const auto index : manifests)
		{
			if(entries[index].parts.size() == 2)
				roots.push_back(index);
		}
		collection = roots.size() > 1;
	}
	if(roots.empty())
	{
		for(const auto index : manifests)
		{
			const auto & parts = entries[index].parts;
			if(parts.size() == 3 && parts.front().compare("Mods", Qt::CaseInsensitive) == 0)
				roots.push_back(index);
		}
		collection = !roots.empty();
	}
	if(roots.empty())
	{
		QString wrapper;
		for(const auto index : manifests)
		{
			const auto & parts = entries[index].parts;
			if(parts.size() == 4 && parts[1].compare("Mods", Qt::CaseInsensitive) == 0)
			{
				if(!wrapper.isEmpty() && wrapper != parts.front())
					fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains multiple unrelated mod collections"));
				wrapper = parts.front();
				roots.push_back(index);
			}
		}
		collection = !roots.empty();
	}
	if(roots.empty())
	{
		// Retain the legacy wrapper/mod/mod.json layout, as well as a collection
		// sharing that wrapper. Deeper arbitrary mod.json files are not roots.
		QString wrapper;
		for(const auto index : manifests)
		{
			const auto & parts = entries[index].parts;
			if(parts.size() == 3)
			{
				if(!wrapper.isEmpty() && wrapper != parts.front())
					fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains multiple unrelated mod directories"));
				wrapper = parts.front();
				roots.push_back(index);
			}
		}
		collection = roots.size() > 1;
	}
	if(roots.empty())
		fail(QCoreApplication::translate("ModArchiveInstaller", "Archive has an unsupported mod directory layout"));

	QSet<QString> ids;
	QSet<QString> protectedIds;
	for(const auto & id : request.protectedModIds)
		protectedIds.insert(pathKey(id));
	std::vector<ModPlan> mods;
	for(const auto index : roots)
	{
		const auto & manifest = entries[index];
		const auto prefix = manifest.parts.mid(0, manifest.parts.size() - 1).join('/');
		QString id;
		if(!collection && !request.modIdHint.isEmpty())
			id = request.modIdHint.toLower();
		else if(manifest.parts.size() > 1)
			id = manifest.parts[manifest.parts.size() - 2].toLower();
		else
			id = QFileInfo(request.archivePath).completeBaseName().toLower();
		if(!isValidModId(id))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains an invalid or reserved mod ID: %1").arg(id));
		if(ids.contains(pathKey(id)))
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive contains more than one mod with ID %1").arg(id));
		ids.insert(pathKey(id));
		if(protectedIds.contains(pathKey(id)))
		{
			result.errors.push_back(QCoreApplication::translate("ModArchiveInstaller", "Mod is protected and cannot be replaced: %1").arg(id));
			continue;
		}

		ModPlan plan{prefix, id, {}, {}};
		for(size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
		{
			const auto & entry = entries[entryIndex];
			if(!prefix.isEmpty() && entry.path != prefix && !entry.path.startsWith(prefix + '/'))
				continue;
			const auto relative = relativePath(plan, entry);
			if(relative.section('/', 0, 0).compare(".github", Qt::CaseInsensitive) == 0)
				continue;
			plan.entries.push_back(entryIndex);
			if(isModManifest(entry) && isSubmodManifest(relative))
				plan.manifests.push_back(relative);
		}
		mods.push_back(std::move(plan));
	}
	return mods;
}

void validateManifests(const QString & stagingPath, const ModPlan & mod)
{
	// Mod metadata is small. Do not let an invalid multi-gigabyte "mod.json"
	// turn the otherwise bounded-memory importer into a whole-file allocation.
	constexpr qint64 maxManifestSize = 16 * 1024 * 1024;
	for(const auto & manifest : mod.manifests)
	{
		const QString path = QDir(stagingPath).filePath(manifest);
		QFile file(path);
		if(!file.open(QIODevice::ReadOnly) || file.size() > maxManifestSize)
			fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot read mod metadata: %1").arg(path));
		const auto data = file.read(maxManifestSize + 1);
		if(file.error() != QFileDevice::NoError || data.size() != file.size())
			fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot read mod metadata: %1").arg(path));
		JsonParsingSettings parserSettings;
		parserSettings.strict = true;
		const JsonNode metadata(data.constData(), data.size(), parserSettings, path.toStdString());
		if(!metadata.isStruct() || !metadata["modType"].isString() || metadata["modType"].String().empty())
			fail(QCoreApplication::translate("ModArchiveInstaller", "Invalid mod metadata: %1").arg(manifest));
	}
}

struct TimingLog
{
	QString archivePath;
	qint64 inspect = 0;
	qint64 extract = 0;
	qint64 content = 0;
	qint64 commit = 0;
	~TimingLog()
	{
		logGlobal->info("Mod archive import '%s': inspect %lld ms, extract %lld ms, content %lld ms, commit %lld ms",
			qUtf8Printable(archivePath), static_cast<long long>(inspect), static_cast<long long>(extract),
			static_cast<long long>(content), static_cast<long long>(commit));
	}
};

class Installer
{
	const Request & request;
	const std::function<void(const Progress &)> & onProgress;
	const std::function<bool()> & isCanceled;
	Progress progress;
	QElapsedTimer progressTimer;
	bool cancellationRequested = false;

	void checkCanceled()
	{
		if(cancellationRequested || (isCanceled && isCanceled()))
		{
			cancellationRequested = true;
			throw Canceled{};
		}
	}

	void report(bool force = false)
	{
		if(onProgress && (force || progressTimer.elapsed() >= 100))
		{
			onProgress(progress);
			progressTimer.restart();
		}
	}

	void addTotal(quint64 amount)
	{
		if(amount > std::numeric_limits<quint64>::max() - progress.totalBytes)
			fail(QCoreApplication::translate("ModArchiveInstaller", "Archive is too large"));
		progress.totalBytes += amount;
	}

	bool extractedBytes(uint64_t amount)
	{
		if(cancellationRequested || (isCanceled && isCanceled()))
		{
			cancellationRequested = true;
			return false;
		}
		progress.completedBytes += amount;
		report();
		return true;
	}

	void extractArchiveEntry(ZipArchive & archive, const QString & root, size_t index, const QString & relative)
	{
		checkCanceled();
		if(!archive.extractTo(qstringToPath(root), index, qstringToPath(relative),
			[this](uint64_t amount) { return extractedBytes(amount); }))
		{
			checkCanceled();
			fail(QCoreApplication::translate("ModArchiveInstaller", "Failed to extract archive entry: %1").arg(relative));
		}
	}

	void extractContent(const QString & modPath)
	{
		QStringList archives;
		QDirIterator iterator(modPath, QDir::Files | QDir::Hidden | QDir::NoSymLinks, QDirIterator::Subdirectories);
		while(iterator.hasNext())
		{
			checkCanceled();
			iterator.next();
			if(iterator.fileName().compare("content.zip", Qt::CaseInsensitive) == 0)
				archives.push_back(iterator.filePath());
		}
		for(const auto & archivePath : archives)
		{
			checkCanceled();
			QString contentPath = QFileInfo(archivePath).dir().filePath("content");
			const auto siblings = QFileInfo(archivePath).dir().entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
			for(const auto & sibling : siblings)
			{
				if(pathKey(sibling.fileName()) != "content")
					continue;
				if(!sibling.isDir() || sibling.isSymLink())
					fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot create content directory: %1").arg(sibling.absoluteFilePath()));
				contentPath = sibling.absoluteFilePath();
			}
			{
				ZipArchive archive(qstringToPath(archivePath));
				const auto entries = inspectEntries(archive);
				// A nested archive must not silently overwrite content already supplied
				// by the outer ZIP, including case-insensitive collisions.
				QSet<QString> existingFiles;
				QHash<QString, QString> existingSpellings;
				QDirIterator existing(contentPath, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
				while(existing.hasNext())
				{
					existing.next();
					const auto relative = QDir(contentPath).relativeFilePath(existing.filePath());
					existingSpellings.insert(pathKey(relative), relative);
					if(!existing.fileInfo().isDir())
						existingFiles.insert(pathKey(relative));
				}
				for(size_t index = 0; index < entries.size(); ++index)
				{
					const auto & entry = entries[index];
					QString prefix;
					for(const auto & component : entry.parts)
					{
						prefix = prefix.isEmpty() ? component : prefix + '/' + component;
						if(existingFiles.contains(pathKey(prefix))
							|| (existingSpellings.contains(pathKey(prefix)) && existingSpellings.value(pathKey(prefix)) != prefix))
							fail(QCoreApplication::translate("ModArchiveInstaller", "content.zip conflicts with an existing file: %1").arg(prefix));
					}
					if(!entry.directory && existingSpellings.contains(pathKey(entry.path)))
						fail(QCoreApplication::translate("ModArchiveInstaller", "content.zip conflicts with an existing path: %1").arg(entry.path));
					addTotal(archive.entries()[index].uncompressedSize);
				}
				report(true);
				for(size_t index = 0; index < entries.size(); ++index)
					extractArchiveEntry(archive, contentPath, index, entries[index].path);
			}
			// Windows cannot delete an archive while its native FILE handle is open.
			if(!QFile::remove(archivePath))
				fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot remove extracted content.zip: %1").arg(archivePath));
		}
	}

	QString existingModPath(const QString & modsPath, const QString & id)
	{
		QString result;
		const auto entries = QDir(modsPath).entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
		for(const auto & entry : entries)
		{
			if(pathKey(entry.fileName()) != pathKey(id))
				continue;
			if(!result.isEmpty())
				fail(QCoreApplication::translate("ModArchiveInstaller", "Installed mod has ambiguous directory names: %1").arg(id));
			if(entry.isSymLink() || !entry.isDir())
				fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot replace a mod path that is a link or a file: %1").arg(entry.absoluteFilePath()));
			const QFileInfo repository(QDir(entry.absoluteFilePath()).filePath(".git"));
			if(repository.exists() || repository.isSymLink())
				fail(QCoreApplication::translate("ModArchiveInstaller", "Mod directory is a git repository and can not be managed by launcher"));
			result = entry.absoluteFilePath();
		}
		return result;
	}

public:
	Installer(const Request & request, const std::function<void(const Progress &)> & onProgress,
		const std::function<bool()> & isCanceled)
		: request(request)
		, onProgress(onProgress)
		, isCanceled(isCanceled)
	{
		progressTimer.start();
	}

	Result run()
	{
		Result result;
		TimingLog timings{request.archivePath};
		QElapsedTimer timer;
		try
		{
			checkCanceled();
			report(true);
			timer.start();
			ZipArchive archive(qstringToPath(request.archivePath));
			const auto entries = inspectEntries(archive);
			const auto mods = makePlan(request, entries, result);
			progress.totalMods = static_cast<int>(mods.size());
			for(const auto & mod : mods)
			{
				for(const auto index : mod.entries)
					addTotal(archive.entries()[index].uncompressedSize);
			}
			timings.inspect = timer.elapsed();
			if(mods.empty())
				return result;

			QDir modsDirectory(request.modsPath);
			if(!modsDirectory.mkpath("."))
				fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot create Mods directory: %1").arg(request.modsPath));
			const QString modsPath = QFileInfo(modsDirectory.absolutePath()).canonicalFilePath();
			if(modsPath.isEmpty())
				fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot resolve Mods directory: %1").arg(request.modsPath));
			// Resolve Mods first: it may itself be a link to a different volume.
			// Staging next to the real directory keeps directory renames on one volume.
			const QString temporaryPattern = QFileInfo(modsPath).dir().filePath(".vcmi-import-XXXXXX");
			for(const auto & mod : mods)
			{
				checkCanceled();
				progress.modId = mod.id;
				progress.stage = Stage::Extracting;
				report(true);
				QTemporaryDir staging(temporaryPattern);
				try
				{
					if(!staging.isValid())
						fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot create a staging directory next to %1").arg(modsPath));
					const auto oldPath = existingModPath(modsPath, mod.id);
					const auto newPath = QDir(staging.path()).filePath("new");
					if(!QDir().mkpath(newPath))
						fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot create a staging directory: %1").arg(newPath));
					timer.restart();
					for(const auto index : mod.entries)
						extractArchiveEntry(archive, newPath, index, relativePath(mod, entries[index]));
					validateManifests(newPath, mod);
					timings.extract += timer.elapsed();

					if(request.fullModExtraction)
					{
						progress.stage = Stage::ExtractingContent;
						report(true);
						timer.restart();
						extractContent(newPath);
						timings.content += timer.elapsed();
					}
					checkCanceled();
					progress.stage = Stage::Committing;
					report(true);
					timer.restart();
					const QString backupPath = QDir(staging.path()).filePath("old");
					const QString targetPath = QDir(modsPath).filePath(mod.id);
					if(existingModPath(modsPath, mod.id) != oldPath)
						fail(QCoreApplication::translate("ModArchiveInstaller", "Mod directory changed during installation: %1").arg(mod.id));
					if(!oldPath.isEmpty() && !QDir().rename(oldPath, backupPath))
						fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot back up installed mod: %1").arg(oldPath));
					// Cancellation is intentionally not observed between these two renames.
					// The previous installation must always be restored if commit fails.
					if(!QDir().rename(newPath, targetPath))
					{
						if(!oldPath.isEmpty() && !QDir().rename(backupPath, oldPath))
						{
							staging.setAutoRemove(false);
							fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot install mod or restore its previous directory. Backup preserved at %1").arg(backupPath));
						}
						fail(QCoreApplication::translate("ModArchiveInstaller", "Cannot move the extracted mod into %1").arg(targetPath));
					}
					result.installedMods.push_back(mod.id);
					if(!oldPath.isEmpty() && !QDir(backupPath).removeRecursively())
					{
						staging.setAutoRemove(false);
						result.errors.push_back(QCoreApplication::translate("ModArchiveInstaller", "Mod %1 was installed, but its backup could not be removed: %2").arg(mod.id, backupPath));
					}
					timings.commit += timer.elapsed();
				}
				catch(const InstallError & error)
				{
					result.errors.push_back(QStringLiteral("%1: %2").arg(mod.id, error.message));
				}
				catch(const std::exception & error)
				{
					result.errors.push_back(QStringLiteral("%1: %2").arg(mod.id, QString::fromUtf8(error.what())));
				}
				++progress.completedMods;
				report(true);
			}
		}
		catch(const Canceled &)
		{
			result.canceled = true;
		}
		catch(const InstallError & error)
		{
			result.errors.push_back(error.message);
		}
		catch(const std::exception & error)
		{
			result.errors.push_back(QString::fromUtf8(error.what()));
		}
		return result;
	}
};
}

Result install(const Request & request, const std::function<void(const Progress &)> & progress,
	const std::function<bool()> & cancel)
{
	return Installer(request, progress, cancel).run();
}
}
