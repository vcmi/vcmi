/*
 * modstatecontroller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modstatecontroller.h"

#include "modstatemodel.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/ScopeGuard.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include "../../vcmiqt/jsonutils.h"
#include "../../vcmiqt/launcherdirs.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
void findContentArchives(const QDir & currentDir, QVector<QString> & archives)
{
	const QFileInfoList entries = currentDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for(const QFileInfo & entry : entries)
	{
		if(entry.isDir())
		{
			findContentArchives(QDir(entry.absoluteFilePath()), archives);
			continue;
		}

		if(entry.fileName().compare("content.zip", Qt::CaseInsensitive) == 0)
			archives.push_back(entry.absoluteFilePath());
	}
}

bool extractContentArchives(const QString & modPath, ModStateController::ArchiveImportProgress & progress)
{
	QVector<QString> archives;
	findContentArchives(QDir(modPath), archives);

	for(const QString & archivePath : archives)
	{
		if(progress.canceled)
			return false;

		const QFileInfo archiveInfo(archivePath);
		const QString contentDirPath = archiveInfo.dir().filePath("content");

		if(!QDir(contentDirPath).exists() && !archiveInfo.dir().mkpath("content"))
		{
			logGlobal->error("Failed to create content directory for '%s'", archivePath.toStdString().c_str());
			return false;
		}

		{
			ZipArchive archive(qstringToPath(archivePath));
			const auto & entries = archive.entries();

			for(const auto & entry : entries)
				progress.totalBytes += entry.uncompressedSize;

			for(size_t index = 0; index < entries.size(); ++index)
			{
				if(progress.canceled)
					return false;

				const auto progressCallback = [&progress](uint64_t amount)
				{
					progress.completedBytes += amount;
					return !progress.canceled.load();
				};
				if(!archive.extractTo(qstringToPath(contentDirPath), index, entries[index].name, progressCallback))
				{
					logGlobal->error("Failed to extract '%s' from '%s'", entries[index].name.c_str(), archivePath.toStdString().c_str());
					return false;
				}
			}
		}

		if(!QFile::remove(archivePath))
		{
			logGlobal->error("Failed to remove extracted archive '%s'", archivePath.toStdString().c_str());
			return false;
		}
	}

	return true;
}

bool asciiEqualsNoCase(std::string_view value, std::string_view expected)
{
	if(value.size() != expected.size())
		return false;

	for(size_t index = 0; index < value.size(); ++index)
	{
		if(std::tolower(static_cast<unsigned char>(value[index])) != std::tolower(static_cast<unsigned char>(expected[index])))
			return false;
	}
	return true;
}

bool asciiEndsWithNoCase(std::string_view value, std::string_view suffix)
{
	if(value.size() < suffix.size())
		return false;
	return asciiEqualsNoCase(value.substr(value.size() - suffix.size()), suffix);
}

bool asciiStartsWithNoCase(std::string_view value, std::string_view prefix)
{
	if(value.size() < prefix.size())
		return false;
	return asciiEqualsNoCase(value.substr(0, prefix.size()), prefix);
}

std::string normalizeZipName(const std::string & name)
{
	std::string result = name;
	std::replace(result.begin(), result.end(), '\\', '/');
	return result;
}

bool isUnderRoot(const std::string & name, const std::string & root)
{
	if(root.empty())
		return true;
	return name == root || (name.size() > root.size() && name.compare(0, root.size(), root) == 0 && name[root.size()] == '/');
}

std::string relativeToRoot(const std::string & name, const std::string & root)
{
	if(root.empty())
		return name;
	if(name == root)
		return {};
	return name.substr(root.size() + 1);
}

std::vector<std::string> detectModRoots(const std::vector<ZipArchive::Entry> & entries, const std::vector<std::string> & names)
{
	size_t minDepth = std::numeric_limits<size_t>::max();
	for(size_t index = 0; index < entries.size(); ++index)
	{
		const auto & name = names[index];
		if(entries[index].directory || !asciiEndsWithNoCase(name, "mod.json"))
			continue;

		const size_t depth = static_cast<size_t>(std::count(name.begin(), name.end(), '/')) + 1;
		minDepth = std::min(minDepth, depth);
	}

	std::vector<std::string> roots;
	if(minDepth == std::numeric_limits<size_t>::max())
		return roots;

	for(size_t index = 0; index < entries.size(); ++index)
	{
		const auto & name = names[index];
		if(entries[index].directory || !asciiEndsWithNoCase(name, "mod.json"))
			continue;

		const size_t depth = static_cast<size_t>(std::count(name.begin(), name.end(), '/')) + 1;
		if(depth != minDepth)
			continue;

		const size_t slash = name.rfind('/');
		const std::string root = slash == std::string::npos ? std::string{} : name.substr(0, slash);
		if(std::find(roots.begin(), roots.end(), root) == roots.end())
			roots.push_back(root);
	}

	return roots;
}

bool isModsCollectionRoot(const std::string & root)
{
	std::string lower = root;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char character)
	{
		return static_cast<char>(std::tolower(character));
	});
	return lower == "mods" || lower.rfind("mods/", 0) == 0 || lower.find("/mods/") != std::string::npos;
}

QString modIdForRoot(const std::string & root, const QString & hint, size_t rootCount)
{
	// A single mod keeps the archive filename as its ID, the same convention the launcher used before bundles were supported.
	// An explicit Mods/ wrapper describes a collection, so the directory name is the ID instead.
	if(rootCount == 1 && !hint.isEmpty() && !isModsCollectionRoot(root))
		return hint.toLower();
	if(root.empty())
		return hint.toLower();

	const size_t slash = root.rfind('/');
	return QString::fromStdString(root.substr(slash == std::string::npos ? 0 : slash + 1)).toLower();
}

bool isValidModId(const QString & id)
{
	return !id.isEmpty() && id != "." && id != ".." && !id.contains('/') && !id.contains('\\');
}

QString importTranslate(const char * text)
{
	return QCoreApplication::translate("ModStateController", text);
}

struct ModPlan
{
	std::string root;
	QString id;
	std::vector<size_t> entries;
	quint64 totalBytes = 0;
};
}

QString ModStateController::modIdFromArchivePath(const QString & archivePath)
{
	QString name = QFileInfo(archivePath).completeBaseName().toLower();
	name.remove(QRegularExpression("-[0-9a-f]{40}$"));
	name.remove(QRegularExpression("-vcmi-.+$"));
	name.remove(QRegularExpression("-main$"));
	return name;
}


ModStateController::ModStateController(std::shared_ptr<ModStateModel> modList)
	: modList(modList)
{
}

ModStateController::~ModStateController() = default;

void ModStateController::setRepositoryData(const JsonNode & repomap)
{
	modList->setRepositoryData(repomap);
}

bool ModStateController::addError(QString modname, QString message)
{
	recentErrors.push_back(QString("%1: %2").arg(modname).arg(message));
	return false;
}

QStringList ModStateController::getErrors()
{
	QStringList ret = recentErrors;
	recentErrors.clear();
	return ret;
}

bool ModStateController::uninstallMod(QString modname)
{
	return canUninstallMod(modname) && doUninstallMod(modname);
}

bool ModStateController::enableMods(QStringList modlist)
{
	for (const auto & modname : modlist)
		if (!canEnableMod(modname))
			return false;

	modList->doEnableMods(modlist);
	return true;
}

bool ModStateController::disableMod(QString modname)
{
	if (!canDisableMod(modname))
		return false;
	modList->doDisableMod(modname);
	return true;
}

bool ModStateController::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not uninstall submod"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod is not installed"));

	if(!isModManageable(modname))
		return addError(modname, getUnmanageableReason(modname));

	return true;
}

QString ModStateController::getUnmanageableReason(const QString & modname)
{
	const QString modDirectory = findModDirectory(modname.section('.', 0, 0));

	if(modDirectory.isEmpty())
		return tr("Mod is not located in user data directory and can not be managed by launcher");

	// .git may also be a file, in case of git worktrees and submodules
	if(QFileInfo::exists(modDirectory + "/.git"))
		return tr("Mod directory is a git repository and can not be managed by launcher");

	return {};
}

bool ModStateController::isModManageable(QString modname)
{
	return getUnmanageableReason(modname).isEmpty();
}

bool ModStateController::canEnableMod(QString modname)
{
	if (!modList->isModExists(modname))
		return false;

	auto mod = modList->getMod(modname);

	if(modList->isModEnabled(modname))
		return addError(modname, tr("Mod is already enabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	//check for compatibility
	if(!mod.isCompatible())
		return addError(modname, tr("Mod is not compatible, please update VCMI and check the latest mod revisions"));

	if (mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage().toStdString())
		return addError(modname, tr("Can not enable translation mod for a different language!"));

	for(const auto & modEntry : mod.getDependencies())
	{
		if(!modList->isModExists(modEntry)) // required mod is not available
			return addError(modname, tr("Required mod %1 is missing").arg(modEntry));
	}

	return true;
}

bool ModStateController::canDisableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(!modList->isModEnabled(modname))
		return addError(modname, tr("Mod is already disabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	return true;
}

QString ModStateController::getStagingPath(const QString & modname)
{
	return CLauncherDirs::modsPath() + "/." + modname + ".staging";
}

QString ModStateController::getBackupPath(const QString & modname)
{
	return CLauncherDirs::modsPath() + "/." + modname + ".backup";
}

QString ModStateController::findModDirectory(const QString & modname)
{
	const QDir modsDir(CLauncherDirs::modsPath());
	for(const auto & entry : modsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		if(entry.compare(modname, Qt::CaseInsensitive) == 0)
			return modsDir.filePath(entry);
	return {};
}

ModStateController::ArchiveImportResult ModStateController::installArchive(const QString & archivePath, const QString & modIdHint,
	bool fullModExtraction, const QStringList & protectedMods, ArchiveImportProgress & progress)
{
	ArchiveImportResult result;
	progress.completedBytes = 0;
	progress.totalBytes = 0;
	progress.completedMods = 0;
	progress.totalMods = 0;

	if(!QFile::exists(archivePath))
	{
		result.errors.push_back(importTranslate("Mod archive is missing"));
		return result;
	}

	std::unique_ptr<ZipArchive> archive;
	try
	{
		archive = std::make_unique<ZipArchive>(qstringToPath(archivePath));
	}
	catch(const std::exception & exception)
	{
		result.errors.push_back(importTranslate("Mod archive is invalid or corrupted: %1").arg(QString::fromUtf8(exception.what())));
		return result;
	}

	const auto & archiveEntries = archive->entries();
	std::vector<std::string> normalizedNames;
	normalizedNames.reserve(archiveEntries.size());
	for(const auto & entry : archiveEntries)
		normalizedNames.push_back(normalizeZipName(entry.name));

	const auto roots = detectModRoots(archiveEntries, normalizedNames);
	if(roots.empty())
	{
		bool hasMaps = false;
		bool hasSaves = false;
		for(const auto & name : normalizedNames)
		{
			hasMaps |= asciiEndsWithNoCase(name, ".h3m")
				|| asciiEndsWithNoCase(name, ".h3c")
				|| asciiEndsWithNoCase(name, ".vmap")
				|| asciiEndsWithNoCase(name, ".vcmp");
			hasSaves |= asciiEndsWithNoCase(name, ".vsgm1");
		}
		result.mapArchive = hasMaps;
		result.saveArchive = !hasMaps && hasSaves;
		if(!result.mapArchive && !result.saveArchive)
			result.errors.push_back(importTranslate("Archive does not contain mods, maps or saves"));
		return result;
	}

	QString hint = modIdHint.toLower();
	if(hint.isEmpty())
		hint = modIdFromArchivePath(archivePath);

	std::vector<ModPlan> plans;
	QStringList plannedIds;
	for(const auto & root : roots)
	{
		const QString id = modIdForRoot(root, hint, roots.size());
		if(!isValidModId(id))
		{
			result.errors.push_back(importTranslate("Archive contains an invalid mod ID: %1").arg(id));
			continue;
		}
		if(plannedIds.contains(id, Qt::CaseInsensitive))
		{
			result.errors.push_back(importTranslate("Archive contains more than one mod with ID %1").arg(id));
			continue;
		}
		plannedIds.push_back(id);

		ModPlan plan;
		plan.root = root;
		plan.id = id;
		plans.push_back(std::move(plan));
	}

	for(size_t index = 0; index < archiveEntries.size(); ++index)
	{
		const auto & name = normalizedNames[index];
		for(auto & plan : plans)
		{
			if(!isUnderRoot(name, plan.root))
				continue;

			const auto relative = relativeToRoot(name, plan.root);
			if(!relative.empty()
				&& !asciiEqualsNoCase(relative, ".github")
				&& !asciiStartsWithNoCase(relative, ".github/"))
			{
				plan.entries.push_back(index);
				plan.totalBytes += archiveEntries[index].uncompressedSize;
			}
			break;
		}
	}

	for(const auto & plan : plans)
		progress.totalBytes += plan.totalBytes;
	progress.totalMods = static_cast<int>(plans.size());

	for(const auto & plan : plans)
	{
		if(progress.canceled)
		{
			result.canceled = true;
			return result;
		}

		if(protectedMods.contains(plan.id, Qt::CaseInsensitive) && !findModDirectory(plan.id).isEmpty())
		{
			result.errors.push_back(importTranslate("Mod %1 can not be managed by launcher").arg(plan.id));
			++progress.completedMods;
			continue;
		}

		logGlobal->info("Installing mod '%s' from '%s'", plan.id.toStdString(), archivePath.toStdString());
		progress.setCurrentMod(plan.id);

		const QString stagingPath = getStagingPath(plan.id);
		const QString backupPath = getBackupPath(plan.id);
		bool stagingExists = false;

		try
		{
			// leftovers of an installation that was interrupted, e.g. by a crash
			for(const auto & path : {stagingPath, backupPath})
				if(QDir(path).exists() && !removeModDir(path))
					throw std::runtime_error(importTranslate("Failed to remove directory %1").arg(path).toStdString());

			if(!QDir().mkpath(stagingPath))
				throw std::runtime_error(importTranslate("Failed to create directory %1").arg(stagingPath).toStdString());
			stagingExists = true;

			auto removeStaging = vstd::makeScopeGuard([&stagingExists, &stagingPath]()
			{
				if(stagingExists && !removeModDir(stagingPath))
					logGlobal->error("Failed to remove directory '%s'", stagingPath.toStdString());
			});

			for(const size_t index : plan.entries)
			{
				if(progress.canceled)
				{
					result.canceled = true;
					return result;
				}

				const auto progressCallback = [&progress](uint64_t amount)
				{
					progress.completedBytes += amount;
					return !progress.canceled.load();
				};
				const std::string relative = relativeToRoot(normalizedNames[index], plan.root);
				if(!archive->extractTo(qstringToPath(stagingPath), index, relative, progressCallback))
				{
					if(progress.canceled)
					{
						result.canceled = true;
						return result;
					}
					throw std::runtime_error(importTranslate("Failed to extract archive entry: %1").arg(QString::fromStdString(relative)).toStdString());
				}
			}

			if(fullModExtraction && !extractContentArchives(stagingPath, progress))
			{
				if(progress.canceled)
				{
					result.canceled = true;
					return result;
				}
				throw std::runtime_error(importTranslate("Failed to extract mod data").toStdString());
			}

			QDir githubDir(stagingPath + "/.github");
			if(githubDir.exists())
				githubDir.removeRecursively();

			// Existing mod is renamed instead of deleted, so a locked directory fails without leaving a partially removed mod
			const QString lockedDirectoryError = importTranslate("Failed to replace directory %1. Close all applications that may be using it, such as file explorer, and try again.");
			const QString existingDirectory = findModDirectory(plan.id);
			const QString targetDirectory = CLauncherDirs::modsPath() + '/' + plan.id;

			if(!existingDirectory.isEmpty() && !QDir().rename(existingDirectory, backupPath))
				throw std::runtime_error(lockedDirectoryError.arg(existingDirectory).toStdString());

			if(!QDir().rename(stagingPath, targetDirectory))
			{
				if(!existingDirectory.isEmpty() && !QDir().rename(backupPath, existingDirectory))
					logGlobal->error("Failed to restore previous version of mod '%s' from '%s'", plan.id.toStdString(), backupPath.toStdString());
				throw std::runtime_error(lockedDirectoryError.arg(targetDirectory).toStdString());
			}
			stagingExists = false;

			if(!existingDirectory.isEmpty() && !removeModDir(backupPath))
				logGlobal->error("Failed to remove previous version of mod '%s' from '%s'", plan.id.toStdString(), backupPath.toStdString());

			result.installedMods.push_back(plan.id);
		}
		catch(const std::exception & exception)
		{
			if(progress.canceled)
				result.canceled = true;
			else
				result.errors.push_back(QStringLiteral("%1: %2").arg(plan.id, QString::fromUtf8(exception.what())));
		}

		++progress.completedMods;
		if(result.canceled)
			return result;
	}

	return result;
}

bool ModStateController::doUninstallMod(QString modname)
{
	ResourcePath resID(std::string("Mods/") + modname.toStdString(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = pathToQString(*CResourceHandler::get()->getResourceName(resID));

	if(!QDir(modDir).exists())
		return addError(modname, tr("Mod data was not found"));

	QDir modFullDir(modDir);
	if(!removeModDir(modDir))
		return addError(modname, tr("Mod is located in a protected directory, please remove it manually:\n") + modFullDir.absolutePath());

	return true;
}

bool ModStateController::removeModDir(QString path)
{
	// issues 2673 and 2680 its why you do not recursively remove without sanity check
	QDir checkDir(path);
	QDir dir(path);
	
	if(!checkDir.cdUp() || QString::compare("Mods", checkDir.dirName(), Qt::CaseInsensitive))
		return false;
#ifndef VCMI_MOBILE // ios and android applications are stored in the isolated container
	if(!checkDir.cdUp() || QString::compare("vcmi", checkDir.dirName(), Qt::CaseInsensitive))
		return false;

	if(!dir.absolutePath().contains("vcmi", Qt::CaseInsensitive))
		return false;
#endif
	if(!dir.absolutePath().contains("Mods", Qt::CaseInsensitive))
		return false;

	return dir.removeRecursively();
}
