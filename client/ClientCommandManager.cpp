/*
 * ClientCommandManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ClientCommandManager.h"

#include "Client.h"
#include "adventureMap/CInGameConsole.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "CServerHandler.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/WindowHandler.h"
#include "render/IRenderHandler.h"
#include "ClientNetPackVisitors.h"
#include "../lib/callback/CCallback.h"
#include "../lib/callback/CGlobalAI.h"
#include "../lib/callback/CDynLibHandler.h"
#include "../lib/CConfigHandler.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/constants/StringConstants.h"
#include "../lib/campaign/CampaignHandler.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "windows/CCastleInterface.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "render/CAnimation.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ContentTypeHandler.h"
#include "../lib/modding/ModUtility.h"
#include "../lib/serializer/GameConnection.h"
#include "../lib/VCMIDirs.h"
#include "../lib/logging/VisualLogger.h"

#ifdef SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif

void ClientCommandManager::handleQuitCommand()
{
		exit(EXIT_SUCCESS);
}

void ClientCommandManager::handleSaveCommand(std::istringstream & singleWordBuffer)
{
	if(!GAME->server().client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}

	std::string saveFilename;
	singleWordBuffer >> saveFilename;
	GAME->interface()->cb->save(saveFilename);
	printCommandMessage("Game saved as: " + saveFilename);
}

void ClientCommandManager::handleLoadCommand(std::istringstream& singleWordBuffer)
{
	// TODO: this code should end the running game and manage to call startGame instead
	//std::string fname;
	//singleWordBuffer >> fname;
	//GAME->server().client->loadGame(fname);
}

void ClientCommandManager::handleGoSoloCommand()
{
	Settings session = settings.write["session"];

	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if(!GAME->server().client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}

	if(session["aiSolo"].Bool())
	{
		// unlikely it will work but just in case to be consistent
		for(auto & color : GAME->server().getAllClientPlayers(GAME->server().logicConnection->connectionID))
		{
			if(color.isValidPlayer() && GAME->server().client->gameInfo().getStartInfo()->playerInfos.at(color).isControlledByHuman())
			{
				GAME->server().client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(color), color);
			}
		}
	}
	else
	{
		PlayerColor currentColor = GAME->interface()->playerID;
		GAME->server().client->removeGUI();
		
		for(auto & color : GAME->server().getAllClientPlayers(GAME->server().logicConnection->connectionID))
		{
			if(color.isValidPlayer() && GAME->server().client->gameInfo().getStartInfo()->playerInfos.at(color).isControlledByHuman())
			{
				auto AiToGive = GAME->server().client->aiNameForPlayer(*GAME->server().client->gameInfo().getPlayerSettings(color), false, false);
				printCommandMessage("Player " + color.toString() + " will be lead by " + AiToGive, ELogLevel::INFO);
				GAME->server().client->installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), color);
			}
		}

		ENGINE->windows().totalRedraw();
		giveTurn(currentColor);
	}

	session["aiSolo"].Bool() = !session["aiSolo"].Bool();
}

void ClientCommandManager::handleAutoskipCommand()
{
		Settings session = settings.write["session"];
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
}

void ClientCommandManager::handleControlaiCommand(std::istringstream& singleWordBuffer)
{
	std::string colorName;
	singleWordBuffer >> colorName;
	boost::to_lower(colorName);

	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if(!GAME->server().client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}

	PlayerColor color;
	if(GAME->interface())
		color = GAME->interface()->playerID;

	for(auto & elem : GAME->server().client->gameState().players)
	{
		if(!elem.first.isValidPlayer()
			|| elem.second.human
			|| (colorName.length() && elem.first.getNum() != vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, colorName)))
		{
			continue;
		}

		GAME->server().client->removeGUI();
		GAME->server().client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
	}

	ENGINE->windows().totalRedraw();
	if(color != PlayerColor::NEUTRAL)
		giveTurn(color);
}

void ClientCommandManager::handleSetBattleAICommand(std::istringstream& singleWordBuffer)
{
	std::string aiName;
	singleWordBuffer >> aiName;

	printCommandMessage("Will try loading that AI to see if it is correct name...\n");
	try
	{
		if(auto ai = CDynLibHandler::getNewBattleAI(aiName)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
		{
			Settings neutralAI = settings.write["ai"]["combatNeutralAI"];
			neutralAI->String() = aiName;
			printCommandMessage("Setting changed, from now the battle ai will be " + aiName + "!\n");
		}
	}
	catch(std::exception &e)
	{
		printCommandMessage("Failed opening " + aiName + ": " + e.what(), ELogLevel::WARN);
		printCommandMessage("Setting not changed, AI not found or invalid!", ELogLevel::WARN);
	}
}

void ClientCommandManager::handleRedrawCommand()
{
	ENGINE->windows().totalRedraw();
}

void ClientCommandManager::handleTranslateGameCommand(bool onlyMissing)
{
	std::map<std::string, std::map<std::string, std::string>> textsByMod;
	LIBRARY->generaltexth->exportAllTexts(textsByMod, onlyMissing);

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / ( onlyMissing ? "translationMissing" : "translation");
	boost::filesystem::create_directories(outPath);

	for(const auto & modEntry : textsByMod)
	{
		JsonNode output;

		for(const auto & stringEntry : modEntry.second)
		{
			if(boost::algorithm::starts_with(stringEntry.first, "map."))
				continue;
			if(boost::algorithm::starts_with(stringEntry.first, "campaign."))
				continue;

			output[stringEntry.first].String() = stringEntry.second;
		}

		if (!output.isNull())
		{
			const boost::filesystem::path filePath = outPath / (modEntry.first + ".json");
			std::ofstream file(filePath.c_str());
			file << output.toString();
		}
	}

	printCommandMessage("Translation export complete");
}

void ClientCommandManager::handleTranslateMapsCommand()
{
	CMapService mapService;

	printCommandMessage("Searching for available maps");
	std::unordered_set<ResourcePath> mapList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == EResType::MAP;
	});

	std::vector<std::unique_ptr<CMap>> loadedMaps;
	std::vector<std::shared_ptr<CampaignState>> loadedCampaigns;

	printCommandMessage("Loading maps for export");
	for (auto const & mapName : mapList)
	{
		try
		{
			// load and drop loaded map - we only need loader to run over all maps
			loadedMaps.push_back(mapService.loadMap(mapName, GAME->interface()->cb.get()));
		}
		catch(std::exception & e)
		{
			logGlobal->warn("Map %s is invalid. Message: %s", mapName.getName(), e.what());
		}
	}

	printCommandMessage("Searching for available campaigns");
	std::unordered_set<ResourcePath> campaignList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == EResType::CAMPAIGN;
	});

	logGlobal->info("Loading campaigns for export");
	for (auto const & campaignName : campaignList)
	{
		try
		{
			loadedCampaigns.push_back(CampaignHandler::getCampaign(campaignName.getName()));
			for (auto const & part : loadedCampaigns.back()->allScenarios())
				loadedCampaigns.back()->getMap(part, GAME->interface()->cb.get());
		}
		catch(std::exception & e)
		{
			logGlobal->warn("Campaign %s is invalid. Message: %s", campaignName.getName(), e.what());
		}
	}

	std::map<std::string, std::map<std::string, std::string>> textsByMod;
	LIBRARY->generaltexth->exportAllTexts(textsByMod, false);

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "translation";
	boost::filesystem::create_directories(outPath);

	for(const auto & modEntry : textsByMod)
	{
		JsonNode output;

		for(const auto & stringEntry : modEntry.second)
		{
			if(boost::algorithm::starts_with(stringEntry.first, "map."))
				output[stringEntry.first].String() = stringEntry.second;

			if(boost::algorithm::starts_with(stringEntry.first, "campaign."))
				output[stringEntry.first].String() = stringEntry.second;
		}

		if (!output.isNull())
		{
			const boost::filesystem::path filePath = outPath / (modEntry.first + ".json");
			std::ofstream file(filePath.c_str());
			file << output.toString();
		}
	}

	printCommandMessage("Translation export complete");
}

void ClientCommandManager::handleGetConfigCommand()
{
	printCommandMessage("Command accepted.\t");

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "configuration";

	boost::filesystem::create_directories(outPath);

	const std::vector<std::string> contentNames = { "heroClasses", "artifacts", "creatures", "factions", "objects", "heroes", "spells", "skills" };

	for(auto contentName : contentNames)
	{
		auto const & handler = *LIBRARY->modh->content;
		auto const & content = handler[contentName];

		auto contentOutPath = outPath / contentName;
		boost::filesystem::create_directories(contentOutPath);

		for(auto& iter : content.modData)
		{
			const JsonNode& modData = iter.second.modData;

			for(auto& nameAndObject : modData.Struct())
			{
				const JsonNode& object = nameAndObject.second;

				std::string name = ModUtility::makeFullIdentifier(object.getModScope(), contentName, nameAndObject.first);

				boost::algorithm::replace_all(name, ":", "_");

				const boost::filesystem::path filePath = contentOutPath / (name + ".json");
				std::ofstream file(filePath.c_str());
				file << object.toString();
			}
		}
	}

	printCommandMessage("\rExtracting done :)\n");
	printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
}

void ClientCommandManager::handleAntilagCommand(std::istringstream& singleWordBuffer)
{
	std::string commandName;
	singleWordBuffer >> commandName;

	if (commandName == "on")
	{
		GAME->server().enableLagCompensation(true);
		printCommandMessage("Network lag compensation is now enabled.\n");
	}
	else if (commandName == "off")
	{
		GAME->server().enableLagCompensation(true);
		printCommandMessage("Network lag compensation is now disabled.\n");
	}
	else
	{
		printCommandMessage("Unexpected syntax. Supported forms:\n");
		printCommandMessage("'antilag on'\n");
		printCommandMessage("'antilag off'\n");
	}
}

void ClientCommandManager::handleGetScriptsCommand()
{
#if SCRIPTING_ENABLED
	printCommandMessage("Command accepted.\t");

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "scripts";

	boost::filesystem::create_directories(outPath);

	for(const auto & kv : LIBRARY->scriptHandler->objects)
	{
		std::string name = kv.first;
		boost::algorithm::replace_all(name,":","_");

		const scripting::ScriptImpl * script = kv.second.get();
		boost::filesystem::path filePath = outPath / (name + ".lua");
		std::ofstream file(filePath.c_str());
		file << script->getSource();
	}
	printCommandMessage("\rExtracting done :)\n");
	printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
#endif
}

void ClientCommandManager::handleGetTextCommand()
{
	printCommandMessage("Command accepted.\t");

	const boost::filesystem::path outPath =
			VCMIDirs::get().userExtractedPath();

	auto list =
			CResourceHandler::get()->getFilteredFiles([](const ResourcePath & ident)
			{
				return ident.getType() == EResType::TEXT && boost::algorithm::starts_with(ident.getName(), "DATA/");
			});

	for (auto & filename : list)
	{
		const boost::filesystem::path filePath = outPath / (filename.getName() + ".TXT");

		boost::filesystem::create_directories(filePath.parent_path());

		std::ofstream file(filePath.c_str());
		auto text = CResourceHandler::get()->load(filename)->readAll();

		file.write((char*)text.first.get(), text.second);
	}

	printCommandMessage("\rExtracting done :)\n");
	printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
}

void ClientCommandManager::handleDef2bmpCommand(std::istringstream& singleWordBuffer)
{
	std::string URI;
	singleWordBuffer >> URI;
	auto anim = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin(URI), EImageBlitMode::SIMPLE);
	anim->exportBitmaps(VCMIDirs::get().userExtractedPath());
}

void ClientCommandManager::handleExtractCommand(std::istringstream& singleWordBuffer)
{
	std::string URI;
	singleWordBuffer >> URI;

	if(CResourceHandler::get()->existsResource(ResourcePath(URI)))
	{
		const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / URI;

		auto data = CResourceHandler::get()->load(ResourcePath(URI))->readAll();

		boost::filesystem::create_directories(outPath.parent_path());
		std::ofstream outFile(outPath.c_str(), std::ofstream::binary);
		outFile.write((char*)data.first.get(), data.second);
	}
	else
		printCommandMessage("File not found!", ELogLevel::ERROR);
}

void ClientCommandManager::handleBonusesCommand(std::istringstream & singleWordBuffer)
{
	if(currentCallFromIngameConsole)
	{
		printCommandMessage("Output for this command is too large for ingame chat! Please run it from client console.\n");
		return;
	}

	std::string outputFormat;
	singleWordBuffer >> outputFormat;

	auto format = [outputFormat](const BonusList & b) -> std::string
	{
		if(outputFormat == "json")
			return b.toJsonNode().toCompactString();

		std::ostringstream ss;
		ss << b;
		return ss.str();
	};
		printCommandMessage("Bonuses of " + GAME->interface()->localState->getCurrentArmy()->getObjectName() + "\n");
		printCommandMessage(format(*GAME->interface()->localState->getCurrentArmy()->getAllBonuses(Selector::all)) + "\n");

	printCommandMessage("\nInherited bonuses:\n");
	TCNodes parents;
		GAME->interface()->localState->getCurrentArmy()->getDirectParents(parents);
	for(const CBonusSystemNode *parent : parents)
	{
		printCommandMessage(std::string("\nBonuses from ") + typeid(*parent).name() + "\n" + format(*parent->getAllBonuses(Selector::all)) + "\n");
	}
}

void ClientCommandManager::handleTellCommand(std::istringstream& singleWordBuffer)
{
	std::string what;
	int id1;
	int id2;
	singleWordBuffer >> what >> id1 >> id2;

	if(what == "hs")
	{
		for(const CGHeroInstance* h : GAME->interface()->cb->getHeroesInfo())
			if(h->getHeroTypeID().getNum() == id1)
				if(const CArtifactInstance* a = h->getArt(ArtifactPosition(id2)))
					printCommandMessage(a->nodeName());
	}
}

void ClientCommandManager::handleMpCommand()
{
	if(const CGHeroInstance* h = GAME->interface()->localState->getCurrentHero())
		printCommandMessage(std::to_string(h->movementPointsRemaining()) + "; max: " + std::to_string(h->movementPointsLimit(true)) + "/" + std::to_string(h->movementPointsLimit(false)) + "\n");
}

void ClientCommandManager::handleSetCommand(std::istringstream& singleWordBuffer)
{
	std::string what;
	std::string value;
	singleWordBuffer >> what;

	Settings config = settings.write["session"][what];

	singleWordBuffer >> value;

	if(value == "on")
	{
		config->Bool() = true;
		printCommandMessage("Option " + what + " enabled!", ELogLevel::INFO);
	}
	else if(value == "off")
	{
		config->Bool() = false;
		printCommandMessage("Option " + what + " disabled!", ELogLevel::INFO);
	}
}

void ClientCommandManager::handleCrashCommand()
{
	int* ptr = nullptr;
	*ptr = 666;
	//disaster!
}

void ClientCommandManager::handleVsLog(std::istringstream & singleWordBuffer)
{
	std::string key;
	singleWordBuffer >> key;

	logVisual->setKey(key);
}

void ClientCommandManager::handleGenerateAssets()
{
	ENGINE->renderHandler().exportGeneratedAssets();
	printCommandMessage("All assets generated");
}

void ClientCommandManager::printCommandMessage(const std::string &commandMessage, ELogLevel::ELogLevel messageType)
{
	switch(messageType)
	{
		case ELogLevel::NOT_SET:
			std::cout << commandMessage;
			break;
		case ELogLevel::TRACE:
			logGlobal->trace(commandMessage);
			break;
		case ELogLevel::DEBUG:
			logGlobal->debug(commandMessage);
			break;
		case ELogLevel::INFO:
			logGlobal->info(commandMessage);
			break;
		case ELogLevel::WARN:
			logGlobal->warn(commandMessage);
			break;
		case ELogLevel::ERROR:
			logGlobal->error(commandMessage);
			break;
		default:
			std::cout << commandMessage;
			break;
	}

	if(currentCallFromIngameConsole)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);
		if(GAME->interface() && GAME->interface()->cingconsole)
		{
			GAME->interface()->cingconsole->addMessage("", "System", commandMessage);
		}
	}
}

void ClientCommandManager::giveTurn(const PlayerColor &colorIdentifier)
{
	PlayerStartsTurn yt;
	yt.player = colorIdentifier;
	yt.queryID = QueryID::NONE;

	ApplyClientNetPackVisitor visitor(*GAME->server().client, GAME->server().client->gameState());
	yt.visit(visitor);
}

void ClientCommandManager::processCommand(const std::string & message, bool calledFromIngameConsole)
{
	// split the message into individual words
	std::istringstream singleWordBuffer;
	singleWordBuffer.str(message);

	// get command name, to be used for single word commands
	std::string commandName;
	singleWordBuffer >> commandName;

	currentCallFromIngameConsole = calledFromIngameConsole;

	if(message == std::string("die, fool"))
		handleQuitCommand();

	else if(commandName == "save")
		handleSaveCommand(singleWordBuffer);

	else if(commandName=="load")
		handleLoadCommand(singleWordBuffer); // not implemented

	else if(commandName == "gosolo")
		handleGoSoloCommand();

	else if(commandName == "autoskip")
		handleAutoskipCommand();

	else if(commandName == "controlai")
		handleControlaiCommand(singleWordBuffer);

	else if(commandName == "setBattleAI")
		handleSetBattleAICommand(singleWordBuffer);

	else if(commandName == "antilag")
		handleAntilagCommand(singleWordBuffer);

	else if(commandName == "redraw")
		handleRedrawCommand();

	else if(message=="translate" || message=="translate game")
		handleTranslateGameCommand(false);

	else if(message=="translate missing")
		handleTranslateGameCommand(true);

	else if(message=="translate maps")
		handleTranslateMapsCommand();

	else if(message=="get config")
		handleGetConfigCommand();

	else if(message=="get scripts")
		handleGetScriptsCommand();

	else if(message=="get txt")
		handleGetTextCommand();

	else if(commandName == "def2bmp")
		handleDef2bmpCommand(singleWordBuffer);

	else if(commandName == "extract")
		handleExtractCommand(singleWordBuffer);

	else if(commandName == "bonuses")
		handleBonusesCommand(singleWordBuffer);

	else if(commandName == "tell")
		handleTellCommand(singleWordBuffer);

	else if(commandName == "mp" && GAME->interface())
		handleMpCommand();

	else if (commandName == "set")
		handleSetCommand(singleWordBuffer);

	else if(commandName == "crash")
		handleCrashCommand();

	else if(commandName == "vslog")
		handleVsLog(singleWordBuffer);

	else if(message=="generate assets")
		handleGenerateAssets();

	else
	{
		if (!commandName.empty() && !vstd::iswithin(commandName[0], 0, ' ')) // filter-out debugger/IDE noise
			printCommandMessage("Command not found :(", ELogLevel::ERROR);
	}
}
