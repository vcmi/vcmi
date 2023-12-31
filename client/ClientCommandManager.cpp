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
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "render/IRenderHandler.h"
#include "ClientNetPackVisitors.h"
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
#include "../CCallback.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ContentTypeHandler.h"
#include "../lib/modding/ModUtility.h"
#include "../lib/CHeroHandler.h"
#include "../lib/VCMIDirs.h"
#include "CMT.h"

#ifdef SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif

void ClientCommandManager::handleQuitCommand()
{
		exit(EXIT_SUCCESS);
}

void ClientCommandManager::handleSaveCommand(std::istringstream & singleWordBuffer)
{
	if(!CSH->client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}

	std::string saveFilename;
	singleWordBuffer >> saveFilename;
	CSH->client->save(saveFilename);
	printCommandMessage("Game saved as: " + saveFilename);
}

void ClientCommandManager::handleLoadCommand(std::istringstream& singleWordBuffer)
{
	// TODO: this code should end the running game and manage to call startGame instead
	//std::string fname;
	//singleWordBuffer >> fname;
	//CSH->client->loadGame(fname);
}

void ClientCommandManager::handleGoSoloCommand()
{
	Settings session = settings.write["session"];

	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	if(!CSH->client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}
	PlayerColor color;
	if(session["aiSolo"].Bool())
	{
		for(auto & elem : CSH->client->gameState()->players)
		{
			if(elem.second.human)
				CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
		}
	}
	else
	{
		color = LOCPLINT->playerID;
		CSH->client->removeGUI();
		for(auto & elem : CSH->client->gameState()->players)
		{
			if(elem.second.human)
			{
				auto AiToGive = CSH->client->aiNameForPlayer(*CSH->client->getPlayerSettings(elem.first), false, false);
				printCommandMessage("Player " + elem.first.toString() + " will be lead by " + AiToGive, ELogLevel::INFO);
				CSH->client->installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), elem.first);
			}
		}
		GH.windows().totalRedraw();
		giveTurn(color);
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

	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

	if(!CSH->client)
	{
		printCommandMessage("Game is not in playing state");
		return;
	}

	PlayerColor color;
	if(LOCPLINT)
		color = LOCPLINT->playerID;

	for(auto & elem : CSH->client->gameState()->players)
	{
		if(elem.second.human || 
			(colorName.length() && elem.first.getNum() != vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, colorName)))
		{
			continue;
		}

		CSH->client->removeGUI();
		CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
	}

	GH.windows().totalRedraw();
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
			Settings neutralAI = settings.write["server"]["neutralAI"];
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
	GH.windows().totalRedraw();
}

void ClientCommandManager::handleNotDialogCommand()
{
	LOCPLINT->showingDialog->setn(false);
}

void ClientCommandManager::handleConvertTextCommand()
{
	logGlobal->info("Searching for available maps");
	std::unordered_set<ResourcePath> mapList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == EResType::MAP;
	});

	std::unordered_set<ResourcePath> campaignList = CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == EResType::CAMPAIGN;
	});

	CMapService mapService;

	logGlobal->info("Loading maps for export");
	for (auto const & mapName : mapList)
	{
		try
		{
			// load and drop loaded map - we only need loader to run over all maps
			mapService.loadMap(mapName);
		}
		catch(std::exception & e)
		{
			logGlobal->error("Map %s is invalid. Message: %s", mapName.getName(), e.what());
		}
	}

	logGlobal->info("Loading campaigns for export");
	for (auto const & campaignName : campaignList)
	{
		auto state = CampaignHandler::getCampaign(campaignName.getName());
		for (auto const & part : state->allScenarios())
			state->getMap(part);
	}

	VLC->generaltexth->dumpAllTexts();
}

void ClientCommandManager::handleGetConfigCommand()
{
	printCommandMessage("Command accepted.\t");

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "configuration";

	boost::filesystem::create_directories(outPath);

	const std::vector<std::string> contentNames = { "heroClasses", "artifacts", "creatures", "factions", "objects", "heroes", "spells", "skills" };

	for(auto contentName : contentNames)
	{
		auto const & handler = *VLC->modh->content;
		auto const & content = handler[contentName];

		auto contentOutPath = outPath / contentName;
		boost::filesystem::create_directories(contentOutPath);

		for(auto& iter : content.modData)
		{
			const JsonNode& modData = iter.second.modData;

			for(auto& nameAndObject : modData.Struct())
			{
				const JsonNode& object = nameAndObject.second;

				std::string name = ModUtility::makeFullIdentifier(object.meta, contentName, nameAndObject.first);

				boost::algorithm::replace_all(name, ":", "_");

				const boost::filesystem::path filePath = contentOutPath / (name + ".json");
				std::ofstream file(filePath.c_str());
				file << object.toJson();
			}
		}
	}

	printCommandMessage("\rExtracting done :)\n");
	printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
}

void ClientCommandManager::handleGetScriptsCommand()
{
#if SCRIPTING_ENABLED
	printCommandMessage("Command accepted.\t");

	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "scripts";

	boost::filesystem::create_directories(outPath);

	for(auto & kv : VLC->scriptHandler->objects)
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
	auto anim = GH.renderHandler().loadAnimation(AnimationPath::builtin(URI));
	anim->preload();
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
			return b.toJsonNode().toJson(true);

		std::ostringstream ss;
		ss << b;
		return ss.str();
	};
		printCommandMessage("Bonuses of " + LOCPLINT->localState->getCurrentArmy()->getObjectName() + "\n");
		printCommandMessage(format(*LOCPLINT->localState->getCurrentArmy()->getAllBonuses(Selector::all, Selector::all)) + "\n");

	printCommandMessage("\nInherited bonuses:\n");
	TCNodes parents;
		LOCPLINT->localState->getCurrentArmy()->getParents(parents);
	for(const CBonusSystemNode *parent : parents)
	{
		printCommandMessage(std::string("\nBonuses from ") + typeid(*parent).name() + "\n" + format(*parent->getAllBonuses(Selector::all, Selector::all)) + "\n");
	}
}

void ClientCommandManager::handleTellCommand(std::istringstream& singleWordBuffer)
{
	std::string what;
	int id1, id2;
	singleWordBuffer >> what >> id1 >> id2;

	if(what == "hs")
	{
		for(const CGHeroInstance* h : LOCPLINT->cb->getHeroesInfo())
			if(h->type->getIndex() == id1)
				if(const CArtifactInstance* a = h->getArt(ArtifactPosition(id2)))
					printCommandMessage(a->nodeName());
	}
}

void ClientCommandManager::handleMpCommand()
{
	if(const CGHeroInstance* h = LOCPLINT->localState->getCurrentHero())
		printCommandMessage(std::to_string(h->movementPointsRemaining()) + "; max: " + std::to_string(h->movementPointsLimit(true)) + "/" + std::to_string(h->movementPointsLimit(false)) + "\n");
}

void ClientCommandManager::handleSetCommand(std::istringstream& singleWordBuffer)
{
	std::string what, value;
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
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
		if(LOCPLINT && LOCPLINT->cingconsole)
		{
			LOCPLINT->cingconsole->print(commandMessage);
		}
	}
}

void ClientCommandManager::giveTurn(const PlayerColor &colorIdentifier)
{
	PlayerStartsTurn yt;
	yt.player = colorIdentifier;
	yt.queryID = QueryID::NONE;

	ApplyClientNetPackVisitor visitor(*CSH->client, *CSH->client->gameState());
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

	else if(commandName == "redraw")
		handleRedrawCommand();

	else if(commandName == "not dialog")
		handleNotDialogCommand();

	else if(message=="convert txt")
		handleConvertTextCommand();

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

	else if(commandName == "mp" && LOCPLINT)
		handleMpCommand();

	else if (commandName == "set")
		handleSetCommand(singleWordBuffer);

	else if(commandName == "crash")
		handleCrashCommand();

	else
	{
		if (!commandName.empty() && !vstd::iswithin(commandName[0], 0, ' ')) // filter-out debugger/IDE noise
			printCommandMessage("Command not found :(", ELogLevel::ERROR);
	}
}
