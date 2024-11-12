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

#include "../../lib/modding/ModManager.h"

ModStateModel::ModStateModel()
    :modManager(std::make_unique<ModManager>())
{}

ModStateModel::~ModStateModel() = default;

void ModStateModel::setRepositories(QVector<JsonNode> repositoriesList)
{
	//TODO
}

ModState ModStateModel::getMod(QString modName) const
{
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
	return stringListStdToQt(modManager->getActiveMods());
}

QStringList ModStateModel::getSubmods(QString modName) const
{
	return {}; //TODO
}

bool ModStateModel::isModExists(QString modName) const
{
	return vstd::contains(modManager->getAllMods(), modName.toStdString());
}

bool ModStateModel::isModInstalled(QString modName) const
{
	return getMod(modName).isInstalled();
}

bool ModStateModel::isModEnabled(QString modName) const
{
	return getMod(modName).isEnabled(); // TODO
}

bool ModStateModel::isModUpdateAvailable(QString modName) const
{
	return getMod(modName).isUpdateAvailable();
}

bool ModStateModel::isModVisible(QString modName) const
{
	return getMod(modName).isVisible();
}
