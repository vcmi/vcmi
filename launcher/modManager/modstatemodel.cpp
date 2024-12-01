/*
 * modstatemodel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modstatemodel.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/modding/ModManager.h"

ModStateModel::ModStateModel()
	: repositoryData(std::make_unique<JsonNode>())
	, modManager(std::make_unique<ModManager>())
{
}

ModStateModel::~ModStateModel() = default;

void ModStateModel::appendRepositories(const JsonNode & repositoriesList)
{
	JsonUtils::mergeCopy(*repositoryData, repositoriesList);

	modManager = std::make_unique<ModManager>(*repositoryData);
}

void ModStateModel::reloadLocalState()
{
	CResourceHandler::get("initial")->updateFilteredFiles([](const std::string &){ return true; });
	modManager = std::make_unique<ModManager>(*repositoryData);
}

const JsonNode & ModStateModel::getRepositoryData() const
{
	return *repositoryData;
}

ModState ModStateModel::getMod(QString modName) const
{
	assert(modName.toLower() == modName);
	return ModState(modManager->getModDescription(modName.toStdString()));
}

template<typename Container>
QStringList stringListStdToQt(const Container & container)
{
	QStringList result;
	for (const auto & str : container)
		result.push_back(QString::fromStdString(str));
	return result;
}

QStringList ModStateModel::getAllMods() const
{
	return stringListStdToQt(modManager->getAllMods());
}

bool ModStateModel::isModExists(QString modName) const
{
	return vstd::contains(modManager->getAllMods(), modName.toStdString());
}

bool ModStateModel::isModInstalled(QString modName) const
{
	return getMod(modName).isInstalled();
}

bool ModStateModel::isModSettingEnabled(QString rootModName, QString modSettingName) const
{
	return modManager->isModSettingActive(rootModName.toStdString(), modSettingName.toStdString());
}

bool ModStateModel::isModEnabled(QString modName) const
{
	return modManager->isModActive(modName.toStdString());
}

bool ModStateModel::isModUpdateAvailable(QString modName) const
{
	return getMod(modName).isUpdateAvailable();
}

bool ModStateModel::isModVisible(QString modName) const
{
	return getMod(modName).isVisible();
}

QString ModStateModel::getInstalledModSizeFormatted(QString modName) const
{
	return QCoreApplication::translate("File size", "%1 MiB").arg(QString::number(getInstalledModSizeMegabytes(modName), 'f', 1));
}

double ModStateModel::getInstalledModSizeMegabytes(QString modName) const
{
	return modManager->getInstalledModSizeMegabytes(modName.toStdString());
}

void ModStateModel::doEnableMods(QStringList modList)
{
	std::vector<std::string> stdList;

	for (const auto & entry : modList)
		stdList.push_back(entry.toStdString());

	modManager->tryEnableMods(stdList);
}

void ModStateModel::doDisableMod(QString modname)
{
	modManager->tryDisableMod(modname.toStdString());
}

bool ModStateModel::isSubmod(QString modname)
{
	return modname.contains('.');
}

QString ModStateModel::getTopParent(QString modname) const
{
	QStringList components = modname.split('.');
	if (components.size() > 1)
		return components.front();
	else
		return "";
}
