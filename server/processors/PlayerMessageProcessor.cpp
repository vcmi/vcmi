/*
 * CGameHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PlayerMessageProcessor.h"

#include "../CGameHandler.h"
#include "../CVCMIServer.h"

#include "../../lib/serializer/Connection.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/GameConstants.h"
#include "../../lib/NetPacks.h"
#include "../../lib/StartInfo.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../lib/modding/IdentifierStorage.h"
#include "../lib/modding/ModScope.h"

PlayerMessageProcessor::PlayerMessageProcessor()
	:gameHandler(nullptr)
{
}

PlayerMessageProcessor::PlayerMessageProcessor(CGameHandler * gameHandler)
	:gameHandler(gameHandler)
{
}

void PlayerMessageProcessor::playerMessage(PlayerColor player, const std::string &message, ObjectInstanceID currObj)
{
	if (handleHostCommand(player, message))
		return;

	if (handleCheatCode(message, player, currObj))
	{
		if(!gameHandler->getPlayerSettings(player)->isControlledByAI())
			broadcastSystemMessage(VLC->generaltexth->allTexts[260]);

		if(!player.isSpectator())
			gameHandler->checkVictoryLossConditionsForPlayer(player);//Player enter win code or got required art\creature

		return;
	}

	broadcastMessage(player, message);
}

bool PlayerMessageProcessor::handleHostCommand(PlayerColor player, const std::string &message)
{
	std::vector<std::string> words;
	boost::split(words, message, boost::is_any_of(" "));

	bool isHost = false;
	for(auto & c : gameHandler->connections[player])
		if(gameHandler->gameLobby()->isClientHost(c->connectionID))
			isHost = true;

	if(!isHost || words.size() < 2 || words[0] != "game")
		return false;

	if(words[1] == "exit" || words[1] == "quit" || words[1] == "end")
	{
		broadcastSystemMessage("game was terminated");
		gameHandler->gameLobby()->setState(EServerState::SHUTDOWN);

		return true;
	}
	if(words.size() == 3 && words[1] == "save")
	{
		gameHandler->save("Saves/" + words[2]);
		broadcastSystemMessage("game saved as " + words[2]);

		return true;
	}
	if(words.size() == 3 && words[1] == "kick")
	{
		auto playername = words[2];
		PlayerColor playerToKick(PlayerColor::CANNOT_DETERMINE);
		if(std::all_of(playername.begin(), playername.end(), ::isdigit))
			playerToKick = PlayerColor(std::stoi(playername));
		else
		{
			for(auto & c : gameHandler->connections)
			{
				if(c.first.toString() == playername)
					playerToKick = c.first;
			}
		}

		if(playerToKick != PlayerColor::CANNOT_DETERMINE)
		{
			PlayerCheated pc;
			pc.player = playerToKick;
			pc.losingCheatCode = true;
			gameHandler->sendAndApply(&pc);
			gameHandler->checkVictoryLossConditionsForPlayer(playerToKick);
		}
		return true;
	}
	if(words.size() == 2 && words[1] == "cheaters")
	{
		if (cheaters.empty())
			broadcastSystemMessage("No cheaters registered!");

		for (auto const & entry : cheaters)
			broadcastSystemMessage("Player " + entry.toString() + " is cheater!");

		return true;
	}

	return false;
}

void PlayerMessageProcessor::cheatGiveSpells(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;

	///Give hero spellbook
	if (!hero->hasSpellbook())
		gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::SPELLBOOK], ArtifactPosition::SPELLBOOK);

	///Give all spells with bonus (to allow banned spells)
	GiveBonus giveBonus(GiveBonus::ETarget::HERO);
	giveBonus.id = hero->id.getNum();
	giveBonus.bonus = Bonus(BonusDuration::PERMANENT, BonusType::SPELLS_OF_LEVEL, BonusSource::OTHER, 0, 0);
	//start with level 0 to skip abilities
	for (int level = 1; level <= GameConstants::SPELL_LEVELS; level++)
	{
		giveBonus.bonus.subtype = level;
		gameHandler->sendAndApply(&giveBonus);
	}

	///Give mana
	SetMana sm;
	sm.hid = hero->id;
	sm.val = 999;
	sm.absolute = true;
	gameHandler->sendAndApply(&sm);
}

void PlayerMessageProcessor::cheatBuildTown(PlayerColor player, const CGTownInstance * town)
{
	if (!town)
		return;

	for (auto & build : town->town->buildings)
	{
		if (!town->hasBuilt(build.first)
			&& !build.second->getNameTranslated().empty()
			&& build.first != BuildingID::SHIP)
		{
			gameHandler->buildStructure(town->id, build.first, true);
		}
	}
}

void PlayerMessageProcessor::cheatGiveArmy(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	std::string creatureIdentifier = words.empty() ? "archangel" : words[0];
	std::optional<int> amountPerSlot;

	try
	{
		amountPerSlot = std::stol(words.at(1));
	}
	catch(std::logic_error&)
	{
	}

	std::optional<int32_t> creatureId = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "creature", creatureIdentifier, false);

	if(creatureId.has_value())
	{
		const auto * creature = CreatureID(creatureId.value()).toCreature();

		for (int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			if (!hero->hasStackAtSlot(SlotID(i)))
			{
				if (amountPerSlot.has_value())
					gameHandler->insertNewStack(StackLocation(hero, SlotID(i)), creature, *amountPerSlot);
				else
					gameHandler->insertNewStack(StackLocation(hero, SlotID(i)), creature, 5 * std::pow(10, i));
			}
		}
	}
}

void PlayerMessageProcessor::cheatGiveMachines(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;

	if (!hero->getArt(ArtifactPosition::MACH1))
		gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::BALLISTA], ArtifactPosition::MACH1);
	if (!hero->getArt(ArtifactPosition::MACH2))
		gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::AMMO_CART], ArtifactPosition::MACH2);
	if (!hero->getArt(ArtifactPosition::MACH3))
		gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[ArtifactID::FIRST_AID_TENT], ArtifactPosition::MACH3);
}

void PlayerMessageProcessor::cheatGiveArtifacts(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	if (!words.empty())
	{
		for (auto const & word : words)
		{
			auto artID = VLC->identifiers()->getIdentifier(ModScope::scopeGame(), "artifact", word, false);
			if(artID &&  VLC->arth->objects[*artID])
				gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[*artID], ArtifactPosition::FIRST_AVAILABLE);
		}
	}
	else
	{
		for(int g = 7; g < VLC->arth->objects.size(); ++g) //including artifacts from mods
		{
			if(VLC->arth->objects[g]->canBePutAt(hero))
				gameHandler->giveHeroNewArtifact(hero, VLC->arth->objects[g], ArtifactPosition::FIRST_AVAILABLE);
		}
	}
}

void PlayerMessageProcessor::cheatLevelup(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	int levelsToGain;
	try
	{
		levelsToGain = std::stol(words.at(0));
	}
	catch(std::logic_error&)
	{
		levelsToGain = 1;
	}

	gameHandler->changePrimSkill(hero, PrimarySkill::EXPERIENCE, VLC->heroh->reqExp(hero->level + levelsToGain) - VLC->heroh->reqExp(hero->level));
}

void PlayerMessageProcessor::cheatExperience(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	int expAmountProcessed;

	try
	{
		expAmountProcessed = std::stol(words.at(0));
	}
	catch(std::logic_error&)
	{
		expAmountProcessed = 10000;
	}

	gameHandler->changePrimSkill(hero, PrimarySkill::EXPERIENCE, expAmountProcessed);
}

void PlayerMessageProcessor::cheatMovement(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	SetMovePoints smp;
	smp.hid = hero->id;
	try
	{
		smp.val = std::stol(words.at(0));;
	}
	catch(std::logic_error&)
	{
		smp.val = 1000000;
	}

	gameHandler->sendAndApply(&smp);

	GiveBonus gb(GiveBonus::ETarget::HERO);
	gb.bonus.type = BonusType::FREE_SHIP_BOARDING;
	gb.bonus.duration = BonusDuration::ONE_DAY;
	gb.bonus.source = BonusSource::OTHER;
	gb.id = hero->id.getNum();
	gameHandler->giveHeroBonus(&gb);
}

void PlayerMessageProcessor::cheatResources(PlayerColor player, std::vector<std::string> words)
{
	int baseResourceAmount;
	try
	{
		baseResourceAmount = std::stol(words.at(0));;
	}
	catch(std::logic_error&)
	{
		baseResourceAmount = 100;
	}

	TResources resources;
	resources[EGameResID::GOLD] = baseResourceAmount * 1000;
	for (GameResID i = EGameResID::WOOD; i < EGameResID::GOLD; ++i)
		resources[i] = baseResourceAmount;

	gameHandler->giveResources(player, resources);
}

void PlayerMessageProcessor::cheatVictory(PlayerColor player)
{
	PlayerCheated pc;
	pc.player = player;
	pc.winningCheatCode = true;
	gameHandler->sendAndApply(&pc);
}

void PlayerMessageProcessor::cheatDefeat(PlayerColor player)
{
	PlayerCheated pc;
	pc.player = player;
	pc.losingCheatCode = true;
	gameHandler->sendAndApply(&pc);
}

void PlayerMessageProcessor::cheatMapReveal(PlayerColor player, bool reveal)
{
	FoWChange fc;
	fc.mode = reveal;
	fc.player = player;
	const auto & fowMap = gameHandler->gameState()->getPlayerTeam(player)->fogOfWarMap;
	const auto & mapSize = gameHandler->gameState()->getMapSize();
	auto hlp_tab = new int3[mapSize.x * mapSize.y * mapSize.z];
	int lastUnc = 0;

	for(int z = 0; z < mapSize.z; z++)
		for(int x = 0; x < mapSize.x; x++)
			for(int y = 0; y < mapSize.y; y++)
				if(!(*fowMap)[z][x][y] || !fc.mode)
					hlp_tab[lastUnc++] = int3(x, y, z);

	fc.tiles.insert(hlp_tab, hlp_tab + lastUnc);
	delete [] hlp_tab;
	gameHandler->sendAndApply(&fc);
}

bool PlayerMessageProcessor::handleCheatCode(const std::string & cheat, PlayerColor player, ObjectInstanceID currObj)
{
	std::vector<std::string> words;
	boost::split(words, cheat, boost::is_any_of("\t\r\n "));

	if (words.empty())
		return false;

	//Make cheat name case-insensitive, but keep words/parameters (e.g. creature name) as it
	std::string cheatName = boost::to_lower_copy(words[0]);
	words.erase(words.begin());

	std::vector<std::string> townTargetedCheats = { "vcmiarmenelos", "vcmibuild", "nwczion" };
	std::vector<std::string> playerTargetedCheats = {
		"vcmiformenos",  "vcmiresources", "nwctheconstruct",
		"vcmimelkor",    "vcmilose",      "nwcbluepill",
		"vcmisilmaril",  "vcmiwin",       "nwcredpill",
		"vcmieagles",    "vcmimap",       "nwcwhatisthematrix",
		"vcmiungoliant", "vcmihidemap",   "nwcignoranceisbliss"
	};
	std::vector<std::string> heroTargetedCheats = {
		"vcmiainur",             "vcmiarchangel",   "nwctrinity",
		"vcmiangband",           "vcmiblackknight", "nwcagents",
		"vcmiglaurung",          "vcmicrystal",     "vcmiazure",
		"vcmifaerie",            "vcmiarmy",        "vcminissi",
		"vcmiistari",            "vcmispells",      "nwcthereisnospoon",
		"vcminoldor",            "vcmimachines",     "nwclotsofguns",
		"vcmiglorfindel",        "vcmilevel",       "nwcneo",
		"vcminahar",             "vcmimove",        "nwcnebuchadnezzar",
		"vcmiforgeofnoldorking", "vcmiartifacts",
		"vcmiolorin",            "vcmiexp",
	};

	if (!vstd::contains(townTargetedCheats, cheatName) && !vstd::contains(playerTargetedCheats, cheatName) && !vstd::contains(heroTargetedCheats, cheatName))
		return false;

	bool playerTargetedCheat = false;

	for (const auto & i : gameHandler->gameState()->players)
	{
		if (words.empty())
			break;

		if (i.first == PlayerColor::NEUTRAL)
			continue;

		if (words.front() == "ai" && i.second.human)
			continue;

		if (words.front() != "all" && words.front() != i.first.toString())
			continue;

		std::vector<std::string> parameters = words;

		cheaters.insert(i.first);
		playerTargetedCheat = true;
		parameters.erase(parameters.begin());

		if (vstd::contains(playerTargetedCheats, cheatName))
			executeCheatCode(cheatName, i.first, ObjectInstanceID::NONE, parameters);

		if (vstd::contains(townTargetedCheats, cheatName))
			for (const auto & t : i.second.towns)
				executeCheatCode(cheatName, i.first, t->id, parameters);

		if (vstd::contains(heroTargetedCheats, cheatName))
			for (const auto & h : i.second.heroes)
				executeCheatCode(cheatName, i.first, h->id, parameters);
	}

	if (!playerTargetedCheat)
		executeCheatCode(cheatName, player, currObj, words);

	cheaters.insert(player);
	return true;
}

void PlayerMessageProcessor::executeCheatCode(const std::string & cheatName, PlayerColor player, ObjectInstanceID currObj, const std::vector<std::string> & words)
{
	const CGHeroInstance * hero = gameHandler->getHero(currObj);
	const CGTownInstance * town = gameHandler->getTown(currObj);
	if (!town && hero)
		town = hero->visitedTown;

	const auto & doCheatGiveSpells = [&]() { cheatGiveSpells(player, hero); };
	const auto & doCheatBuildTown = [&]() { cheatBuildTown(player, town); };
	const auto & doCheatGiveArmyCustom = [&]() { cheatGiveArmy(player, hero, words); };
	const auto & doCheatGiveArmyFixed = [&](std::vector<std::string> customWords) { cheatGiveArmy(player, hero, customWords); };
	const auto & doCheatGiveMachines = [&]() { cheatGiveMachines(player, hero); };
	const auto & doCheatGiveArtifacts = [&]() { cheatGiveArtifacts(player, hero, words); };
	const auto & doCheatLevelup = [&]() { cheatLevelup(player, hero, words); };
	const auto & doCheatExperience = [&]() { cheatExperience(player, hero, words); };
	const auto & doCheatMovement = [&]() { cheatMovement(player, hero, words); };
	const auto & doCheatResources = [&]() { cheatResources(player, words); };
	const auto & doCheatVictory = [&]() { cheatVictory(player); };
	const auto & doCheatDefeat = [&]() { cheatDefeat(player); };
	const auto & doCheatMapReveal = [&]() { cheatMapReveal(player, true); };
	const auto & doCheatMapHide = [&]() { cheatMapReveal(player, false); };

	// Unimplemented H3 cheats:
	// nwcfollowthewhiterabbit - The currently selected hero permanently gains maximum luck.
	// nwcmorpheus - The currently selected hero permanently gains maximum morale.
	// nwcoracle - The puzzle map is permanently revealed.
	// nwcphisherprice - Changes and brightens the game colors.

	std::map<std::string, std::function<void()>> callbacks = {
		{"vcmiainur",            [&] () {doCheatGiveArmyFixed({ "archangel", "5" });} },
		{"nwctrinity",           [&] () {doCheatGiveArmyFixed({ "archangel", "5" });} },
		{"vcmiangband",          [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });} },
		{"vcmiglaurung",         [&] () {doCheatGiveArmyFixed({ "crystalDragon", "5000" });} },
		{"vcmiarchangel",        [&] () {doCheatGiveArmyFixed({ "archangel", "5" });} },
		{"nwcagents",            [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });} },
		{"vcmiblackknight",      [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });} },
		{"vcmicrystal",          [&] () {doCheatGiveArmyFixed({ "crystalDragon", "5000" });} },
		{"vcmiazure",            [&] () {doCheatGiveArmyFixed({ "azureDragon", "5000" });} },
		{"vcmifaerie",           [&] () {doCheatGiveArmyFixed({ "fairieDragon", "5000" });} },
		{"vcmiarmy",              doCheatGiveArmyCustom },
		{"vcminissi",             doCheatGiveArmyCustom },
		{"vcmiistari",            doCheatGiveSpells     },
		{"vcmispells",            doCheatGiveSpells     },
		{"nwcthereisnospoon",     doCheatGiveSpells     },
		{"vcmiarmenelos",         doCheatBuildTown      },
		{"vcmibuild",             doCheatBuildTown      },
		{"nwczion",               doCheatBuildTown      },
		{"vcminoldor",            doCheatGiveMachines   },
		{"vcmimachines",          doCheatGiveMachines   },
		{"nwclotsofguns",         doCheatGiveMachines   },
		{"vcmiforgeofnoldorking", doCheatGiveArtifacts  },
		{"vcmiartifacts",         doCheatGiveArtifacts  },
		{"vcmiglorfindel",        doCheatLevelup        },
		{"vcmilevel",             doCheatLevelup        },
		{"nwcneo",                doCheatLevelup        },
		{"vcmiolorin",            doCheatExperience     },
		{"vcmiexp",               doCheatExperience     },
		{"vcminahar",             doCheatMovement       },
		{"vcmimove",              doCheatMovement       },
		{"nwcnebuchadnezzar",     doCheatMovement       },
		{"vcmiformenos",          doCheatResources      },
		{"vcmiresources",         doCheatResources      },
		{"nwctheconstruct",       doCheatResources      },
		{"nwcbluepill",           doCheatDefeat         },
		{"vcmimelkor",            doCheatDefeat         },
		{"vcmilose",              doCheatDefeat         },
		{"nwcredpill",            doCheatVictory        },
		{"vcmisilmaril",          doCheatVictory        },
		{"vcmiwin",               doCheatVictory        },
		{"nwcwhatisthematrix",    doCheatMapReveal      },
		{"vcmieagles",            doCheatMapReveal      },
		{"vcmimap",               doCheatMapReveal      },
		{"vcmiungoliant",         doCheatMapHide        },
		{"vcmihidemap",           doCheatMapHide        },
		{"nwcignoranceisbliss",   doCheatMapHide        },
	};

	assert(callbacks.count(cheatName));
	if (callbacks.count(cheatName))
		callbacks.at(cheatName)();
}

void PlayerMessageProcessor::sendSystemMessage(std::shared_ptr<CConnection> connection, const std::string & message)
{
	SystemMessage sm;
	sm.text = message;
	connection->sendPack(&sm);
}

void PlayerMessageProcessor::broadcastSystemMessage(const std::string & message)
{
	SystemMessage sm;
	sm.text = message;
	gameHandler->sendToAllClients(&sm);
}

void PlayerMessageProcessor::broadcastMessage(PlayerColor playerSender, const std::string & message)
{
	PlayerMessageClient temp_message(playerSender, message);
	gameHandler->sendAndApply(&temp_message);
}
