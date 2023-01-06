/*
 * ClientCommandManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ClientCommandManager.h"

#include "Client.h"
#include "CPlayerInterface.h"
#include "CServerHandler.h"
#include "gui/CGuiHandler.h"
#include "../lib/NetPacks.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/StringConstants.h"
#include "gui/CAnimation.h"
#include "windows/CAdvmapInterface.h"
#include "windows/CCastleInterface.h"
#include "../CCallback.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/VCMIDirs.h"

#ifdef SCRIPTING_ENABLED
#include "../lib/ScriptHandler.h"
#endif


void ClientCommandManager::handleGoSolo()
{
	Settings session = settings.write["session"];

	boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
	if(!CSH->client)
	{
		std::cout << "Game is not in playing state";
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
		removeGUI();
		for(auto & elem : CSH->client->gameState()->players)
		{
			if(elem.second.human)
			{
				auto AiToGive = CSH->client->aiNameForPlayer(*CSH->client->getPlayerSettings(elem.first), false);
				logNetwork->info("Player %s will be lead by %s", elem.first, AiToGive);
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
		std::cout << "Game is not in playing state";
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

		removeGUI();
		CSH->client->installNewPlayerInterface(std::make_shared<CPlayerInterface>(elem.first), elem.first);
	}
	GH.totalRedraw();
	if(color != PlayerColor::NEUTRAL)
		giveTurn(color);
}

#ifndef VCMI_IOS
void ClientCommandManager::processCommand(const std::string &message)
{
	std::istringstream readed;
	readed.str(message);
	std::string commandName;
	readed >> commandName;

// Check mantis issue 2292 for details
//	if(LOCPLINT && LOCPLINT->cingconsole)
//		LOCPLINT->cingconsole->print(message);

	if(message==std::string("die, fool"))
	{
		exit(EXIT_SUCCESS);
	}
	else if(commandName == std::string("activate"))
	{
		int what;
		readed >> what;
		switch (what)
		{
			case 0:
				GH.topInt()->activate();
				break;
			case 1:
				adventureInt->activate();
				break;
			case 2:
				LOCPLINT->castleInt->activate();
				break;
			default:
				logGlobal->error("Wrong argument specified!");
		}
	}
	else if(commandName == "redraw")
	{
		GH.totalRedraw();
	}
	else if(commandName == "screen")
	{
		std::cout << "Screenbuf points to ";

		if(screenBuf == screen)
			logGlobal->error("screen");
		else if(screenBuf == screen2)
			logGlobal->error("screen2");
		else
			logGlobal->error("?!?");

		SDL_SaveBMP(screen, "Screen_c.bmp");
		SDL_SaveBMP(screen2, "Screen2_c.bmp");
	}
	else if(commandName == "save")
	{
		if(!CSH->client)
		{
			std::cout << "Game is not in playing state";
			return;
		}
		std::string fname;
		readed >> fname;
		CSH->client->save(fname);
	}
//	else if(commandName=="load")
//	{
//		// TODO: this code should end the running game and manage to call startGame instead
//		std::string fname;
//		readed >> fname;
//		CSH->client->loadGame(fname);
//	}
	else if(message=="convert txt")
	{
		//TODO: to be replaced with "VLC->generaltexth->dumpAllTexts();" due to https://github.com/vcmi/vcmi/pull/1329 merge:

		std::cout << "Command accepted.\t";

		const boost::filesystem::path outPath =
				VCMIDirs::get().userExtractedPath();

		boost::filesystem::create_directories(outPath);

		auto extractVector = [=](const std::vector<std::string> & source, const std::string & name)
		{
			JsonNode data(JsonNode::JsonType::DATA_VECTOR);
			int64_t index = 0;
			for(auto & line : source)
			{
				JsonNode lineNode(JsonNode::JsonType::DATA_STRUCT);
				lineNode["text"].String() = line;
				lineNode["index"].Integer() = index++;
				data.Vector().push_back(lineNode);
			}

			const boost::filesystem::path filePath = outPath / (name + ".json");
			boost::filesystem::ofstream file(filePath);
			file << data.toJson();
		};

		extractVector(VLC->generaltexth->allTexts, "generalTexts");
		extractVector(VLC->generaltexth->jktexts, "jkTexts");
		extractVector(VLC->generaltexth->arraytxt, "arrayTexts");

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
	else if(message=="get config")
	{
		std::cout << "Command accepted.\t";

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

					std::string name = CModHandler::normalizeIdentifier(object.meta, CModHandler::scopeBuiltin(), nameAndObject.first);

					boost::algorithm::replace_all(name,":","_");

					const boost::filesystem::path filePath = contentOutPath / (name + ".json");
					boost::filesystem::ofstream file(filePath);
					file << object.toJson();
				}
			}
		}

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
#if SCRIPTING_ENABLED
		else if(message=="get scripts")
	{
		std::cout << "Command accepted.\t";

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
		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
#endif
	else if(message=="get txt")
	{
		std::cout << "Command accepted.\t";

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

		std::cout << "\rExtracting done :)\n";
		std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
	else if(commandName == "crash")
	{
		int *ptr = nullptr;
		*ptr = 666;
		//disaster!
	}
	else if(commandName == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
			std::cout << h->movement << "; max: " << h->maxMovePoints(true) << "/" << h->maxMovePoints(false) << std::endl;
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
		std::cout << "Bonuses of " << adventureInt->selection->getObjectName() << std::endl
				  << format(adventureInt->selection->getBonusList()) << std::endl;

		std::cout << "\nInherited bonuses:\n";
		TCNodes parents;
		adventureInt->selection->getParents(parents);
		for(const CBonusSystemNode *parent : parents)
		{
			std::cout << "\nBonuses from " << typeid(*parent).name() << std::endl << format(*parent->getAllBonuses(Selector::all, Selector::all)) << std::endl;
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
				std::cout << typeid(childPtr).name() << std::endl;
		}
	}
	else if(commandName == "tell")
	{
		std::string what;
		int id1, id2;
		readed >> what >> id1 >> id2;
		if(what == "hs")
		{
			for(const CGHeroInstance *h : LOCPLINT->cb->getHeroesInfo())
				if(h->type->ID.getNum() == id1)
					if(const CArtifactInstance *a = h->getArt(ArtifactPosition(id2)))
						std::cout << a->nodeName();
		}
	}
	else if (commandName == "set")
	{
		std::string what, value;
		readed >> what;

		Settings config = settings.write["session"][what];

		readed >> value;

		if (value == "on")
		{
			config->Bool() = true;
			logGlobal->info("Option %s enabled!", what);
		}
		else if (value == "off")
		{
			config->Bool() = false;
			logGlobal->info("Option %s disabled!", what);
		}
	}
	else if(commandName == "unlock")
	{
		std::string mxname;
		readed >> mxname;
		if(mxname == "pim" && LOCPLINT)
			LOCPLINT->pim->unlock();
	}
	else if(commandName == "def2bmp")
	{
		std::string URI;
		readed >> URI;
		std::unique_ptr<CAnimation> anim = std::make_unique<CAnimation>(URI);
		anim->preload();
		anim->exportBitmaps(VCMIDirs::get().userExtractedPath());
	}
	else if(commandName == "extract")
	{
		std::string URI;
		readed >> URI;

		if (CResourceHandler::get()->existsResource(ResourceID(URI)))
		{
			const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / URI;

			auto data = CResourceHandler::get()->load(ResourceID(URI))->readAll();

			boost::filesystem::create_directories(outPath.parent_path());
			boost::filesystem::ofstream outFile(outPath, boost::filesystem::ofstream::binary);
			outFile.write((char*)data.first.get(), data.second);
		}
		else
			logGlobal->error("File not found!");
	}
	else if(commandName == "setBattleAI")
	{
		std::string fname;
		readed >> fname;
		std::cout << "Will try loading that AI to see if it is correct name...\n";
		try
		{
			if(auto ai = CDynLibHandler::getNewBattleAI(fname)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
			{
				Settings neutralAI = settings.write["server"]["neutralAI"];
				neutralAI->String() = fname;
				std::cout << "Setting changed, from now the battle ai will be " << fname << "!\n";
			}
		}
		catch(std::exception &e)
		{
			logGlobal->warn("Failed opening %s: %s", fname, e.what());
			logGlobal->warn("Setting not changed, AI not found or invalid!");
		}
	}

	Settings session = settings.write["session"];
	if(commandName == "autoskip")
	{
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
	}
	else if(commandName == "gosolo")
	{
		ClientCommandManager::handleGoSolo();
	}
	else if(commandName == "controlai")
	{
		std::string colorName;
		readed >> colorName;
		boost::to_lower(colorName);

		ClientCommandManager::handleControlAi(colorName);
	}
	// Check mantis issue 2292 for details
/* 	else if(client && client->serv && client->serv->connected && LOCPLINT) //send to server
	{
		boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
		LOCPLINT->cb->sendMessage(message);
	}*/
}
#endif

void ClientCommandManager::giveTurn(const PlayerColor &colorIdentifier)
{
	YourTurn yt;
	yt.player = colorIdentifier;
	yt.daysWithoutCastle = CSH->client->getPlayerState(colorIdentifier)->daysWithoutCastle;
	yt.applyCl(CSH->client);
}

void ClientCommandManager::removeGUI()
{
	// CClient::endGame
	GH.curInt = nullptr;
	if(GH.topInt())
		GH.topInt()->deactivate();
	GH.listInt.clear();
	GH.objsToBlit.clear();
	GH.statusbar = nullptr;
	logGlobal->info("Removed GUI.");

	LOCPLINT = nullptr;
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
	logGlobal->info(sbuffer.str());

	for(const CIntObject *child : obj->children)
		printInfoAboutInterfaceObject(child, level+1);
}