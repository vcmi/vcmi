/*
 * ServerLibraryReloader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IServerLibraryReloader.h"

/// Reloader of a server process that owns its game library
class ServerLibraryReloader final : public IServerLibraryReloader
{
	std::unique_ptr<ModManager> modManager;

public:
	explicit ServerLibraryReloader(std::unique_ptr<ModManager> modManager);
	~ServerLibraryReloader() override;

	const ModManager & mods() const override;
	bool reload(const ModSnapshot & mods, std::string & error) override;
};
