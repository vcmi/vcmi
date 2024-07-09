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
	bool currentCallFromIngameConsole = false; // commands can come from 2 sources: ingame console (chat) and client console
	
	// Quits the game (die, fool command)
	void handleQuitCommand();

	// Saves current game under the given filename
	void handleSaveCommand(std::istringstream & singleWordBuffer);

	// Loads a game with the given filename
	void handleLoadCommand(std::istringstream & singleWordBuffer);

	// AI takes over until the end of turn (unlike original H3 currently causes AI to take over until typed again)
	void handleGoSoloCommand();

	// Toggles autoskip mode on and off. In this mode, player turns are automatically skipped and only AI moves.
	// However, GUI is still present and allows to observe AI moves. After this option is activated, you need to end first turn manually.
	// Press [Shift] before your turn starts to not skip it.
	void handleAutoskipCommand();

	// Gives you control over specified AI player. If none is specified gives you control over all AI players
	void handleControlaiCommand(std::istringstream& singleWordBuffer);

	// Change battle AI used by neutral creatures to the one specified. Persists through game quit
	void handleSetBattleAICommand(std::istringstream& singleWordBuffer);

	// Redraw the current screen
	void handleRedrawCommand();

	// Extracts all translateable game texts into Translation directory, separating files on per-mod basis
	void handleTranslateGameCommand();

	// Extracts all translateable texts from maps and campaigns into Translation directory, separating files on per-mod basis
	void handleTranslateMapsCommand();

	// Saves current game configuration into extracted/configuration folder
	void handleGetConfigCommand();

	// Dumps all scripts in Extracted/Scripts
	void handleGetScriptsCommand();

	// Dumps all .txt files from DATA into Extracted/DATA
	void handleGetTextCommand();

	// Extract .def animation as BMP files
	void handleDef2bmpCommand(std::istringstream& singleWordBuffer);

	// Export file into Extracted directory
	void handleExtractCommand(std::istringstream& singleWordBuffer);

	// Print in console the current bonuses for current army
	void handleBonusesCommand(std::istringstream & singleWordBuffer);

	// Get what artifact is present on artifact slot with specified ID for hero with specified ID
	void handleTellCommand(std::istringstream& singleWordBuffer);

	// Show current movement points, max movement points on land / max movement points on water.
	void handleMpCommand();

	// set <command> <on/off> - sets special temporary settings that reset on game quit.
	void handleSetCommand(std::istringstream& singleWordBuffer);

	// Crashes the game forcing an exception
	void handleCrashCommand();

	// shows object graph
	void handleVsLog(std::istringstream & singleWordBuffer);

	// Prints in Chat the given message
	void printCommandMessage(const std::string &commandMessage, ELogLevel::ELogLevel messageType = ELogLevel::NOT_SET);
	void giveTurn(const PlayerColor &color);

public:
	ClientCommandManager() = default;
	void processCommand(const std::string & message, bool calledFromIngameConsole);
};
