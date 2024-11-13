/*
 * modstatemodel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "modstate.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
class ModManager;
VCMI_LIB_NAMESPACE_END

/// Class that represent current state of available mods
/// Provides Qt-based interface to library class ModManager
class ModStateModel
{
	std::unique_ptr<JsonNode> repositoryData;
	std::unique_ptr<ModManager> modManager;

public:
	ModStateModel();
	~ModStateModel();

	void appendRepositories(const JsonNode & repositoriesList);
	const JsonNode & getRepositoryData() const;

	ModState getMod(QString modName) const;
	QStringList getAllMods() const;
	QStringList getSubmods(QString modName) const;

	bool isModExists(QString modName) const;
	bool isModInstalled(QString modName) const;
	bool isModEnabled(QString modName) const;
	bool isModUpdateAvailable(QString modName) const;
	bool isModVisible(QString modName) const;
};
