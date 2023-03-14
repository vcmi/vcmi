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
#include "adventureMap/CAdvMapInt.h"
#include "CPlayerInterface.h"
#include "CServerHandler.h"
#include "gui/CGuiHandler.h"
#include "../lib/NetPacks.h"
#include "ClientNetPackVisitors.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/StringConstants.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "windows/CCastleInterface.h"
#include "render/CAnimation.h"
#include "../CCallback.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/VCMIDirs.h"
#include "CMT.h"

#ifdef SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif

#include <SDL_surface.h>

void ClientCommandManager::handleGoSolo()
{
	Settings session = settings.write["session"];

	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
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
				auto AiToGive = CSH->client->aiNameForPlayer(*CSH->client->getPlayerSettings(elem.first), false);
				printCommandMessage("Player " + elem.first.getStr() + " will be lead by " + AiToGive, ELogLevel::INFO);
				CSH->client->installNewPlayerInterface(CDynLibHandler::getNewAI(AiToGive), elem.first);
			}
		}
		GH.totalRedraw();
		giveTurn(color);
	}
	session["aiSolo"].Bool() = !session["aiSolo"].Bool();
}

void ClientCommandManager::handleControlAi(const std::string &colorName)
{
	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
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
		if(elem.second.human || (colorName.length() &&
								 elem.first.getNum() != vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, colorName)))
		{
			continue;
		}

		CSH->client->removeGUI();
		CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
	}
	GH.totalRedraw();
	if(color != PlayerColor::NEUTRAL)
		giveTurn(color);
}

void ClientCommandManager::processCommand(const std::string &message, bool calledFromIngameConsole)
{
	std::istringstream singleWordBuffer;
	singleWordBuffer.str(message);
	std::string commandName;
	singleWordBuffer >> commandName;
	currentCallFromIngameConsole = calledFromIngameConsole;

	if(message==std::string("die, fool"))
	{
		exit(EXIT_SUCCESS);
	}
	else if(commandName == "redraw")
	{
		GH.totalRedraw();
	}
	else if(commandName == "screen")
	{
		printCommandMessage("Screenbuf points to ");

		if(screenBuf == screen)
			printCommandMessage("screen", ELogLevel::ERROR);
		else if(screenBuf == screen2)
			printCommandMessage("screen2", ELogLevel::ERROR);
		else
			printCommandMessage("?!?", ELogLevel::ERROR);

		SDL_SaveBMP(screen, "Screen_c.bmp");
		SDL_SaveBMP(screen2, "Screen2_c.bmp");
	}
	else if(commandName == "save")
	{
		if(!CSH->client)
		{
			printCommandMessage("Game is not in playing state");
			return;
		}
		std::string fname;
		singleWordBuffer >> fname;
		CSH->client->save(fname);
	}
//	else if(commandName=="load")
//	{
//		// TODO: this code should end the running game and manage to call startGame instead
//		std::string fname;
//		singleWordBuffer >> fname;
//		CSH->client->loadGame(fname);
//	}
	else if(message=="convert txt")
	{
		std::unordered_set<ResourceID> mapList = CResourceHandler::get()->getFilteredFiles([&](const ResourceID & ident)
		{
			return ident.getType() == EResType::MAP;
		});

		std::unordered_set<ResourceID> campaignList = CResourceHandler::get()->getFilteredFiles([&](const ResourceID & ident)
		{
			return ident.getType() == EResType::CAMPAIGN;
		});

		CMapService mapService;

		for (auto const & mapName : mapList)
			mapService.loadMap(mapName); // load and drop loaded map - we only need loader to run over all maps

		for (auto const & campaignName : campaignList)
		{
			CCampaignState state(CCampaignHandler::getCampaign(campaignName.getName()));
			for (auto const & part : state.camp->mapPieces)
				delete state.getMap(part.first);
		}

		VLC->generaltexth->dumpAllTexts();
	}
	else if(message=="get config")
	{
		printCommandMessage("Command accepted.\t");

		const boost::filesystem::path outPath =
				VCMIDirs::get().userExtractedPath() / "configuration";

		boost::filesystem::create_directories(outPath);

		const std::vector<std::string> contentNames = {"heroClasses", "artifacts", "creatures", "factions", "objects", "heroes", "spells", "skills"};

		for(auto contentName : contentNames)
		{
			auto & content = (*VLC->modh->content)[contentName];

			auto contentOutPath = outPath / contentName;
			boost::filesystem::create_directories(contentOutPath);

			for(auto & iter : content.modData)
			{
				const JsonNode & modData = iter.second.modData;

				for(auto & nameAndObject : modData.Struct())
				{
					const JsonNode & object = nameAndObject.second;

					std::string name = CModHandler::makeFullIdentifier(object.meta, contentName, nameAndObject.first);

					boost::algorithm::replace_all(name,":","_");

					const boost::filesystem::path filePath = contentOutPath / (name + ".json");
					boost::filesystem::ofstream file(filePath);
					file << object.toJson();
				}
			}
		}

		printCommandMessage("\rExtracting done :)\n");
		printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
	}
#if SCRIPTING_ENABLED
		else if(message=="get scripts")
	{
		printCommandMessage("Command accepted.\t");

		const boost::filesystem::path outPath =
			VCMIDirs::get().userExtractedPath() / "scripts";

		boost::filesystem::create_directories(outPath);

		for(auto & kv : VLC->scriptHandler->objects)
		{
			std::string name = kv.first;
			boost::algorithm::replace_all(name,":","_");

			const scripting::ScriptImpl * script = kv.second.get();
			boost::filesystem::path filePath = outPath / (name + ".lua");
			boost::filesystem::ofstream file(filePath);
			file << script->getSource();
		}
		printCommandMessage("\rExtracting done :)\n");
		printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
	}
#endif
	else if(message=="get txt")
	{
		printCommandMessage("Command accepted.\t");

		const boost::filesystem::path outPath =
				VCMIDirs::get().userExtractedPath();

		auto list =
				CResourceHandler::get()->getFilteredFiles([](const ResourceID & ident)
				{
					return ident.getType() == EResType::TEXT && boost::algorithm::starts_with(ident.getName(), "DATA/");
				});

		for (auto & filename : list)
		{
			const boost::filesystem::path filePath = outPath / (filename.getName() + ".TXT");

			boost::filesystem::create_directories(filePath.parent_path());

			boost::filesystem::ofstream file(filePath);
			auto text = CResourceHandler::get()->load(filename)->readAll();

			file.write((char*)text.first.get(), text.second);
		}

		printCommandMessage("\rExtracting done :)\n");
		printCommandMessage("Extracted files can be found in " + outPath.string() + " directory\n");
	}
	else if(commandName == "crash")
	{
		int *ptr = nullptr;
		*ptr = 666;
		//disaster!
	}
	else if(commandName == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = adventureInt->curHero())
			printCommandMessage(std::to_string(h->movement) + "; max: " + std::to_string(h->maxMovePoints(true)) + "/" + std::to_string(h->maxMovePoints(false)) + "\n");
	}
	else if(commandName == "bonuses")
	{
		bool jsonFormat = (message == "bonuses json");
		auto format = [jsonFormat](const BonusList & b) -> std::string
		{
			if(jsonFormat)
				return b.toJsonNode().toJson(true);
			std::ostringstream ss;
			ss << b;
			return ss.str();
		};
		printCommandMessage("Bonuses of " + adventureInt->curArmy()->getObjectName() + "\n");
		printCommandMessage(format(adventureInt->curArmy()->getBonusList()) + "\n");

		printCommandMessage("\nInherited bonuses:\n");
		TCNodes parents;
		adventureInt->curArmy()->getParents(parents);
		for(const CBonusSystemNode *parent : parents)
		{
			printCommandMessage(std::string("\nBonuses from ") + typeid(*parent).name() + "\n" + format(*parent->getAllBonuses(Selector::all, Selector::all)) + "\n");
		}
	}
	else if(commandName == "not dialog")
	{
		LOCPLINT->showingDialog->setn(false);
	}
	else if(commandName == "gui")
	{
		for(auto & child : GH.listInt)
		{
			const auto childPtr = child.get();
			if(const CIntObject * obj = dynamic_cast<const CIntObject *>(childPtr))
				printInfoAboutInterfaceObject(obj, 0);
			else
				printCommandMessage(std::string(typeid(childPtr).name()) + "\n");
		}
	}
	else if(commandName == "tell")
	{
		std::string what;
		int id1, id2;
		singleWordBuffer >> what >> id1 >> id2;
		if(what == "hs")
		{
			for(const CGHeroInstance *h : LOCPLINT->cb->getHeroesInfo())
				if(h->type->getIndex() == id1)
					if(const CArtifactInstance *a = h->getArt(ArtifactPosition(id2)))
						printCommandMessage(a->nodeName());
		}
	}
	else if (commandName == "set")
	{
		std::string what, value;
		singleWordBuffer >> what;

		Settings config = settings.write["session"][what];

		singleWordBuffer >> value;

		if (value == "on")
		{
			config->Bool() = true;
			printCommandMessage("Option " + what + " enabled!", ELogLevel::INFO);
		}
		else if (value == "off")
		{
			config->Bool() = false;
			printCommandMessage("Option " + what + " disabled!", ELogLevel::INFO);
		}
	}
	else if(commandName == "unlock")
	{
		std::string mxname;
		singleWordBuffer >> mxname;
		if(mxname == "pim" && LOCPLINT)
			LOCPLINT->pim->unlock();
	}
	else if(commandName == "def2bmp")
	{
		std::string URI;
		singleWordBuffer >> URI;
		std::unique_ptr<CAnimation> anim = std::make_unique<CAnimation>(URI);
		anim->preload();
		anim->exportBitmaps(VCMIDirs::get().userExtractedPath());
	}
	else if(commandName == "extract")
	{
		std::string URI;
		singleWordBuffer >> URI;

		if (CResourceHandler::get()->existsResource(ResourceID(URI)))
		{
			const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / URI;

			auto data = CResourceHandler::get()->load(ResourceID(URI))->readAll();

			boost::filesystem::create_directories(outPath.parent_path());
			boost::filesystem::ofstream outFile(outPath, boost::filesystem::ofstream::binary);
			outFile.write((char*)data.first.get(), data.second);
		}
		else
			printCommandMessage("File not found!", ELogLevel::ERROR);
	}
	else if(commandName == "setBattleAI")
	{
		std::string fname;
		singleWordBuffer >> fname;
		printCommandMessage("Will try loading that AI to see if it is correct name...\n");
		try
		{
			if(auto ai = CDynLibHandler::getNewBattleAI(fname)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
			{
				Settings neutralAI = settings.write["server"]["neutralAI"];
				neutralAI->String() = fname;
				printCommandMessage("Setting changed, from now the battle ai will be " + fname + "!\n");
			}
		}
		catch(std::exception &e)
		{
			printCommandMessage("Failed opening " + fname + ": " + e.what(), ELogLevel::WARN);
			printCommandMessage("Setting not changed, AI not found or invalid!", ELogLevel::WARN);
		}
	}
	else if(commandName == "autoskip")
	{
		Settings session = settings.write["session"];
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
	}
	else if(commandName == "gosolo")
	{
		ClientCommandManager::handleGoSolo();
	}
	else if(commandName == "controlai")
	{
		std::string colorName;
		singleWordBuffer >> colorName;
		boost::to_lower(colorName);

		ClientCommandManager::handleControlAi(colorName);
	}
	else
	{
		printCommandMessage("Command not found :(", ELogLevel::ERROR);
	}
}

void ClientCommandManager::giveTurn(const PlayerColor &colorIdentifier)
{
	YourTurn yt;
	yt.player = colorIdentifier;
	yt.daysWithoutCastle = CSH->client->getPlayerState(colorIdentifier)->daysWithoutCastle;

	ApplyClientNetPackVisitor visitor(*CSH->client, *CSH->client->gameState());
	yt.visit(visitor);
}

void ClientCommandManager::printInfoAboutInterfaceObject(const CIntObject *obj, int level)
{
	std::stringstream sbuffer;
	sbuffer << std::string(level, '\t');

	sbuffer << typeid(*obj).name() << " *** ";
	if (obj->active)
	{
#define PRINT(check, text) if (obj->active & CIntObject::check) sbuffer << text
		PRINT(LCLICK, 'L');
		PRINT(RCLICK, 'R');
		PRINT(HOVER, 'H');
		PRINT(MOVE, 'M');
		PRINT(KEYBOARD, 'K');
		PRINT(TIME, 'T');
		PRINT(GENERAL, 'A');
		PRINT(WHEEL, 'W');
		PRINT(DOUBLECLICK, 'D');
#undef  PRINT
	}
	else
		sbuffer << "inactive";
	sbuffer << " at " << obj->pos.x <<"x"<< obj->pos.y;
	sbuffer << " (" << obj->pos.w <<"x"<< obj->pos.h << ")";
	printCommandMessage(sbuffer.str(), ELogLevel::INFO);

	for(const CIntObject *child : obj->children)
		printInfoAboutInterfaceObject(child, level+1);
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
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		if(LOCPLINT && LOCPLINT->cingconsole)
		{
			LOCPLINT->cingconsole->print(commandMessage);
		}
	}
}
