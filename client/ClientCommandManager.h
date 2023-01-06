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

class PlayerColor;
class CIntObject;

class ClientCommandManager
{
	static void giveTurn(const PlayerColor &color);
	static void removeGUI();
	static void printInfoAboutInterfaceObject(const CIntObject *obj, int level);
public:
#ifndef VCMI_IOS
	static void processCommand(const std::string &message);
#endif
	static void handleGoSolo();
	static void handleControlAi(const std::string &colorName);
};
