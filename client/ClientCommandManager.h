/*
 * ClientCommandManager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class PlayerColor;
VCMI_LIB_NAMESPACE_END
class CIntObject;

class ClientCommandManager //take mantis #2292 issue about account if thinking about handling cheats from command-line
{
	bool currentCallFromIngameConsole;

	void giveTurn(const PlayerColor &color);
	void printInfoAboutInterfaceObject(const CIntObject *obj, int level);
	void printCommandMessage(const std::string &commandMessage, ELogLevel::ELogLevel messageType = ELogLevel::NOT_SET);
	void handleGoSolo();
	void handleControlAi(const std::string &colorName);

public:
	ClientCommandManager() = default;
	void processCommand(const std::string &message, bool calledFromIngameConsole);
};
