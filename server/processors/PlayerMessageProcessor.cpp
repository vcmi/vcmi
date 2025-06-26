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

#include "TurnOrderProcessor.h"

#include "../CGameHandler.h"
#include "../CVCMIServer.h"
#include "../TurnTimerHandler.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/constants/Enumerations.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/StackLocation.h"
#include "../../lib/serializer/Connection.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../lib/VCMIDirs.h"

PlayerMessageProcessor::PlayerMessageProcessor(CGameHandler * gameHandler)
	:gameHandler(gameHandler)
{
}

void PlayerMessageProcessor::playerMessage(PlayerColor player, const std::string & message, ObjectInstanceID currObj)
{
	if(!message.empty() && message[0] == '!')
	{
		broadcastMessage(player, message);
		handleCommand(player, message);
		return;
	}

	if(handleCheatCode(message, player, currObj))
	{
		if(!gameHandler->gameInfo().getPlayerSettings(player)->isControlledByAI())
		{
			MetaString txt;
			txt.appendLocalString(EMetaText::GENERAL_TXT, 260);
			broadcastSystemMessage(txt);
		}

		if(!player.isSpectator())
			gameHandler->checkVictoryLossConditionsForPlayer(player); //Player enter win code or got required art\creature

		return;
	}

	broadcastMessage(player, message);
}

void PlayerMessageProcessor::commandExit(PlayerColor player, const std::vector<std::string> & words)
{
	bool isHost = gameHandler->gameLobby().isPlayerHost(player);
	if(!isHost)
		return;

	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.gameTerminated"));
	gameHandler->gameLobby().setState(EServerState::SHUTDOWN);
}

void PlayerMessageProcessor::commandKick(PlayerColor player, const std::vector<std::string> & words)
{
	bool isHost = gameHandler->gameLobby().isPlayerHost(player);
	if(!isHost)
		return;

	if(words.size() == 2)
	{
		auto playername = words[1];
		PlayerColor playerToKick(PlayerColor::CANNOT_DETERMINE);
		if(std::all_of(playername.begin(), playername.end(), ::isdigit))
			playerToKick = PlayerColor(std::stoi(playername));
		else
		{
			for (PlayerColor color : PlayerColor::ALL_PLAYERS())
			{
				if(color.toString() == playername)
					playerToKick = color;
			}
		}

		if(playerToKick != PlayerColor::CANNOT_DETERMINE)
		{
			PlayerCheated pc;
			pc.player = playerToKick;
			pc.losingCheatCode = true;
			gameHandler->sendAndApply(pc);
			gameHandler->checkVictoryLossConditionsForPlayer(playerToKick);
		}
	}
}

void PlayerMessageProcessor::commandSave(PlayerColor player, const std::vector<std::string> & words)
{
	bool isHost = gameHandler->gameLobby().isPlayerHost(player);
	if(!isHost)
		return;

	if(words.size() == 2)
	{
		gameHandler->save("Saves/" + words[1]);
		MetaString str;
		str.appendTextID("vcmi.broadcast.gameSavedAs");
		str.appendRawString(" ");
		str.appendRawString(words[1]);
		broadcastSystemMessage(str);
	}
}

void PlayerMessageProcessor::commandCheaters(PlayerColor player, const std::vector<std::string> & words)
{
	int playersCheated = 0;
	for(const auto & playerState : gameHandler->gameState().players)
	{
		if(playerState.second.cheated)
		{
			auto str = MetaString::createFromTextID("vcmi.broadcast.playerCheater");
			str.replaceName(playerState.first);
			broadcastSystemMessage(str);
			playersCheated++;
		}
	}

	if(!playersCheated)
		broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.noCheater"));
}

void PlayerMessageProcessor::commandStatistic(PlayerColor player, const std::vector<std::string> & words)
{
	bool isHost = gameHandler->gameLobby().isPlayerHost(player);
	if(!isHost)
		return;

	std::string path = gameHandler->statistics->writeCsv();

	auto str = MetaString::createFromTextID("vcmi.broadcast.statisticFile");
	str.replaceRawString(path);
	broadcastSystemMessage(str);
}

void PlayerMessageProcessor::commandHelp(PlayerColor player, const std::vector<std::string> & words)
{
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.commands"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.exit"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.kick"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.save"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.statistic"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.commandsAll"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.help"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.cheaters"));
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.help.vote"));
}

void PlayerMessageProcessor::commandVote(PlayerColor player, const std::vector<std::string> & words)
{
	if(words.size() < 2)
	{
		broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.allow"));
		broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.force"));
		broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.abort"));
		broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.timer"));
		return;
	}

	if(words[1] == "yes" || words[1] == "no" || words[1] == MetaString::createFromTextID("vcmi.broadcast.vote.yes").toString() || words[1] == MetaString::createFromTextID("vcmi.broadcast.vote.no").toString())
	{
		if(currentVote == ECurrentChatVote::NONE)
		{
			broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.noActive"));
			return;
		}

		if(words[1] == "yes" || words[1] == MetaString::createFromTextID("vcmi.broadcast.vote.yes").toString())
		{
			awaitingPlayers.erase(player);
			if(awaitingPlayers.empty())
				finishVoting();
			return;
		}
		if(words[1] == "no" || words[1] == MetaString::createFromTextID("vcmi.broadcast.vote.no").toString())
		{
			abortVoting();
			return;
		}
	}

	const auto & parseNumber = [](const std::string & input) -> std::optional<int>
	{
		try
		{
			return std::stol(input);
		}
		catch(std::logic_error &)
		{
			return std::nullopt;
		}
	};

	if(words[1] == "simturns" && words.size() > 2)
	{
		if(words[2] == "allow" && words.size() > 3)
		{
			auto daysCount = parseNumber(words[3]);
			if(daysCount && daysCount.value() > 0)
				startVoting(player, ECurrentChatVote::SIMTURNS_ALLOW, daysCount.value());
			return;
		}

		if(words[2] == "force" && words.size() > 3)
		{
			auto daysCount = parseNumber(words[3]);
			if(daysCount && daysCount.value() > 0)
				startVoting(player, ECurrentChatVote::SIMTURNS_FORCE, daysCount.value());
			return;
		}

		if(words[2] == "abort")
		{
			startVoting(player, ECurrentChatVote::SIMTURNS_ABORT, 0);
			return;
		}
	}

	if(words[1] == "timer" && words.size() > 2)
	{
		if(words[2] == "prolong" && words.size() > 3)
		{
			auto secondsCount = parseNumber(words[3]);
			if(secondsCount && secondsCount.value() > 0)
				startVoting(player, ECurrentChatVote::TIMER_PROLONG, secondsCount.value());
			return;
		}
	}

	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.notRecognized"));
}

void PlayerMessageProcessor::finishVoting()
{
	MetaString msg;
	switch(currentVote)
	{
		case ECurrentChatVote::SIMTURNS_ALLOW:
			msg.appendTextID("vcmi.broadcast.vote.success.untilContacts");
			msg.replaceRawString(std::to_string(currentVoteParameter));
			broadcastSystemMessage(msg);
			gameHandler->turnOrder->setMaxSimturnsDuration(currentVoteParameter);
			break;
		case ECurrentChatVote::SIMTURNS_FORCE:
			msg.appendTextID("vcmi.broadcast.vote.success.contactsBlocked");
			msg.replaceRawString(std::to_string(currentVoteParameter));
			broadcastSystemMessage(msg);
			gameHandler->turnOrder->setMinSimturnsDuration(currentVoteParameter);
			break;
		case ECurrentChatVote::SIMTURNS_ABORT:
			msg.appendTextID("vcmi.broadcast.vote.success.nextDay");
			broadcastSystemMessage(msg);
			gameHandler->turnOrder->setMinSimturnsDuration(0);
			gameHandler->turnOrder->setMaxSimturnsDuration(0);
			break;
		case ECurrentChatVote::TIMER_PROLONG:
			msg.appendTextID("vcmi.broadcast.vote.success.timer");
			msg.replaceRawString(std::to_string(currentVoteParameter));
			broadcastSystemMessage(msg);
			gameHandler->turnTimerHandler->prolongTimers(currentVoteParameter * 1000);
			break;
	}

	currentVote = ECurrentChatVote::NONE;
	currentVoteParameter = -1;
}

void PlayerMessageProcessor::abortVoting()
{
	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.aborted"));
	currentVote = ECurrentChatVote::NONE;
}

void PlayerMessageProcessor::startVoting(PlayerColor initiator, ECurrentChatVote what, int parameter)
{
	currentVote = what;
	currentVoteParameter = parameter;

	MetaString msg;
	switch(currentVote)
	{
		case ECurrentChatVote::SIMTURNS_ALLOW:
			msg.appendTextID("vcmi.broadcast.vote.start.untilContacts");
			msg.replaceRawString(std::to_string(parameter));
			broadcastSystemMessage(msg);
			break;
		case ECurrentChatVote::SIMTURNS_FORCE:
			msg.appendTextID("vcmi.broadcast.vote.start.contactsBlocked");
			msg.replaceRawString(std::to_string(parameter));
			broadcastSystemMessage(msg);
			break;
		case ECurrentChatVote::SIMTURNS_ABORT:
			msg.appendTextID("vcmi.broadcast.vote.start.nextDay");
			broadcastSystemMessage(msg);
			break;
		case ECurrentChatVote::TIMER_PROLONG:
			msg.appendTextID("vcmi.broadcast.vote.start.timer");
			msg.replaceRawString(std::to_string(parameter));
			broadcastSystemMessage(msg);
			break;
		default:
			return;
	}

	broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.vote.hint"));
	awaitingPlayers.clear();

	for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
	{
		auto state = gameHandler->gameInfo().getPlayerState(player, false);
		if(state && state->isHuman() && initiator != player)
			awaitingPlayers.insert(player);
	}

	if(awaitingPlayers.empty())
		finishVoting();
}

void PlayerMessageProcessor::handleCommand(PlayerColor player, const std::string & message)
{
	if(message.empty() || message[0] != '!')
		return;

	std::vector<std::string> words;
	boost::split(words, message, boost::is_any_of(" "));

	if(words[0] == "!exit" || words[0] == "!quit")
		commandExit(player, words);
	if(words[0] == "!help")
		commandHelp(player, words);
	if(words[0] == "!vote")
		commandVote(player, words);
	if(words[0] == "!kick")
		commandKick(player, words);
	if(words[0] == "!save")
		commandSave(player, words);
	if(words[0] == "!cheaters")
		commandCheaters(player, words);
	if(words[0] == "!statistic")
		commandStatistic(player, words);
}

void PlayerMessageProcessor::cheatGiveSpells(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;

	///Give hero spellbook
	if (!hero->hasSpellbook())
		gameHandler->giveHeroNewArtifact(hero, ArtifactID::SPELLBOOK, ArtifactPosition::SPELLBOOK);

	///Give all spells with bonus (to allow banned spells)
	GiveBonus giveBonus(GiveBonus::ETarget::OBJECT);
	giveBonus.id = hero->id;
	giveBonus.bonus = Bonus(BonusDuration::PERMANENT, BonusType::SPELLS_OF_LEVEL, BonusSource::OTHER, 0, BonusSourceID());
	//start with level 0 to skip abilities
	for (int level = 1; level <= GameConstants::SPELL_LEVELS; level++)
	{
		giveBonus.bonus.subtype = BonusCustomSubtype::spellLevel(level);
		gameHandler->sendAndApply(giveBonus);
	}

	giveBonus.bonus = Bonus(BonusDuration::PERMANENT, BonusType::HERO_SPELL_CASTS_PER_COMBAT_TURN, BonusSource::OTHER, 99, BonusSourceID());
	gameHandler->sendAndApply(giveBonus);

	///Give mana
	SetMana sm;
	sm.hid = hero->id;
	sm.val = 999;
	sm.mode = ChangeValueMode::ABSOLUTE;
	gameHandler->sendAndApply(sm);
}

void PlayerMessageProcessor::cheatBuildTown(PlayerColor player, const CGTownInstance * town)
{
	if (!town)
		return;

	for (auto & build : town->getTown()->buildings)
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

	std::optional<int32_t> creatureId = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "creature", creatureIdentifier, false);

	if(creatureId.has_value())
	{
		const auto * creature = CreatureID(creatureId.value()).toCreature();

		for (int i = 0; i < GameConstants::ARMY_SIZE; i++)
		{
			if (!hero->hasStackAtSlot(SlotID(i)))
			{
				if (amountPerSlot.has_value())
					gameHandler->insertNewStack(StackLocation(hero->id, SlotID(i)), creature, *amountPerSlot);
				else
					gameHandler->insertNewStack(StackLocation(hero->id, SlotID(i)), creature, 5 * std::pow(10, i));
			}
		}
	}
}

void PlayerMessageProcessor::cheatGiveMachines(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;

	if (!hero->getArt(ArtifactPosition::MACH1))
		gameHandler->giveHeroNewArtifact(hero, ArtifactID::BALLISTA, ArtifactPosition::MACH1);
	if (!hero->getArt(ArtifactPosition::MACH2))
		gameHandler->giveHeroNewArtifact(hero, ArtifactID::AMMO_CART, ArtifactPosition::MACH2);
	if (!hero->getArt(ArtifactPosition::MACH3))
		gameHandler->giveHeroNewArtifact(hero, ArtifactID::FIRST_AID_TENT, ArtifactPosition::MACH3);
}

void PlayerMessageProcessor::cheatGiveArtifacts(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	if (!words.empty())
	{
		for (auto const & word : words)
		{
			auto artID = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "artifact", word, false);
			if(artID &&  LIBRARY->arth->objects[*artID])
				gameHandler->giveHeroNewArtifact(hero, ArtifactID(*artID), ArtifactPosition::FIRST_AVAILABLE);
		}
	}
	else
	{
		for(int g = 7; g < LIBRARY->arth->objects.size(); ++g) //including artifacts from mods
		{
			if(LIBRARY->arth->objects[g]->canBePutAt(hero))
				gameHandler->giveHeroNewArtifact(hero, ArtifactID(g), ArtifactPosition::FIRST_AVAILABLE);
		}
	}
}

void PlayerMessageProcessor::cheatGiveScrolls(PlayerColor player, const CGHeroInstance * hero)
{
	if(!hero)
		return;

	for(const auto & spell : LIBRARY->spellh->objects)
		if(gameHandler->gameState().isAllowed(spell->getId()) && !spell->isSpecial())
		{
			gameHandler->giveHeroNewScroll(hero, spell->getId(), ArtifactPosition::FIRST_AVAILABLE);
		}
}

void PlayerMessageProcessor::cheatColorSchemeChange(PlayerColor player, ColorScheme scheme)
{
	PlayerCheated pc;
	pc.player = player;
	pc.colorScheme = scheme;
	pc.localOnlyCheat = true;
	gameHandler->sendAndApply(pc);
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

	gameHandler->giveExperience(hero, LIBRARY->heroh->reqExp(hero->level + levelsToGain) - LIBRARY->heroh->reqExp(hero->level));
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

	gameHandler->giveExperience(hero, expAmountProcessed);
}

void PlayerMessageProcessor::cheatMovement(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	SetMovePoints smp;
	smp.hid = hero->id;
	bool unlimited = false;
	try
	{
		smp.val = std::stol(words.at(0));
	}
	catch(std::logic_error&)
	{
		smp.val = 1000000;
		unlimited = true;
	}

	gameHandler->sendAndApply(smp);

	GiveBonus gb(GiveBonus::ETarget::OBJECT);
	gb.bonus.type = BonusType::FREE_SHIP_BOARDING;
	gb.bonus.duration = unlimited ? BonusDuration::PERMANENT : BonusDuration::ONE_DAY;
	gb.bonus.source = BonusSource::OTHER;
	gb.id = hero->id;
	gameHandler->giveHeroBonus(&gb);

	if(unlimited)
	{
		GiveBonus gb(GiveBonus::ETarget::OBJECT);
		gb.bonus.type = BonusType::UNLIMITED_MOVEMENT;
		gb.bonus.duration = BonusDuration::PERMANENT;
		gb.bonus.source = BonusSource::OTHER;
		gb.id = hero->id;
		gameHandler->giveHeroBonus(&gb);
	}
}

void PlayerMessageProcessor::cheatResources(PlayerColor player, std::vector<std::string> words)
{
	int baseResourceAmount;
	try
	{
		baseResourceAmount = std::stol(words.at(0));
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
	gameHandler->sendAndApply(pc);
}

void PlayerMessageProcessor::cheatDefeat(PlayerColor player)
{
	PlayerCheated pc;
	pc.player = player;
	pc.losingCheatCode = true;
	gameHandler->sendAndApply(pc);
}

void PlayerMessageProcessor::cheatMapReveal(PlayerColor player, bool reveal)
{
	FoWChange fc;
	fc.mode = reveal ? ETileVisibility::REVEALED : ETileVisibility::HIDDEN;
	fc.player = player;
	const auto & fowMap = gameHandler->gameState().getPlayerTeam(player)->fogOfWarMap;
	const auto & mapSize = gameHandler->gameState().getMapSize();
	auto hlp_tab = new int3[mapSize.x * mapSize.y * mapSize.z];
	int lastUnc = 0;

	for(int z = 0; z < mapSize.z; z++)
		for(int x = 0; x < mapSize.x; x++)
			for(int y = 0; y < mapSize.y; y++)
				if(!fowMap[z][x][y] || fc.mode == ETileVisibility::HIDDEN)
					hlp_tab[lastUnc++] = int3(x, y, z);

	fc.tiles.insert(hlp_tab, hlp_tab + lastUnc);
	delete [] hlp_tab;

	if (!fc.tiles.empty())
		gameHandler->sendAndApply(fc);
}

void PlayerMessageProcessor::cheatPuzzleReveal(PlayerColor player)
{
	const TeamState * t = gameHandler->gameState().getPlayerTeam(player);

	for(auto & obj : gameHandler->gameState().getMap().getObjects<CGObelisk>())
	{
		if(!obj->wasVisited(player))
		{
			gameHandler->setObjPropertyID(obj->id, ObjProperty::OBELISK_VISITED, t->id);
			for(const auto & color : t->players)
			{
				gameHandler->setObjPropertyID(obj->id, ObjProperty::VISITED, color);

				PlayerCheated pc;
				pc.player = color;
				gameHandler->sendAndApply(pc);
			}
		}
	}
}

void PlayerMessageProcessor::cheatMaxLuck(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;

	GiveBonus gb;
	gb.bonus = Bonus(BonusDuration::PERMANENT, BonusType::MAX_LUCK, BonusSource::OTHER, 0, BonusSourceID(Obj(Obj::NO_OBJ)));
	gb.id = hero->id;

	gameHandler->giveHeroBonus(&gb);
}

void PlayerMessageProcessor::cheatFly(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;
		
	GiveBonus gb;
	gb.bonus = Bonus(BonusDuration::PERMANENT, BonusType::FLYING_MOVEMENT, BonusSource::OTHER, 0, BonusSourceID(Obj(Obj::NO_OBJ)));
	gb.id = hero->id;

	gameHandler->giveHeroBonus(&gb);
}

void PlayerMessageProcessor::cheatMaxMorale(PlayerColor player, const CGHeroInstance * hero)
{
	if (!hero)
		return;
		
	GiveBonus gb;
	gb.bonus = Bonus(BonusDuration::PERMANENT, BonusType::MAX_MORALE, BonusSource::OTHER, 0, BonusSourceID(Obj(Obj::NO_OBJ)));
	gb.id = hero->id;

	gameHandler->giveHeroBonus(&gb);
}

void PlayerMessageProcessor::cheatSkill(PlayerColor player, const CGHeroInstance * hero, std::vector<std::string> words)
{
	if (!hero)
		return;

	std::string identifier;
	try
	{
		identifier = words.at(0);
	}
	catch(std::logic_error&)
	{
		return;
	}

	MasteryLevel::Type mastery;
	try
	{
		mastery = static_cast<MasteryLevel::Type>(std::stoi(words.at(1)));
	}
	catch(std::logic_error&)
	{
		mastery = MasteryLevel::Type::EXPERT;
	}

	if(identifier == "every")
	{
		for(const auto & skill : LIBRARY->skillh->objects)
			gameHandler->changeSecSkill(hero, SecondarySkill(skill->getId()), mastery, ChangeValueMode::ABSOLUTE);
		return;
	}

	std::optional<int32_t> skillId = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "skill", identifier, false);
	if(!skillId.has_value())
		return;
	
	auto skill = SecondarySkill(skillId.value());
	gameHandler->changeSecSkill(hero, skill, mastery, ChangeValueMode::ABSOLUTE);
}

bool PlayerMessageProcessor::handleCheatCode(const std::string & cheat, PlayerColor player, ObjectInstanceID currObj)
{
	std::vector<std::string> words;
	boost::split(words, boost::trim_copy(cheat), boost::is_any_of("\t\r\n "));

	if (words.empty() || !gameHandler->gameInfo().getStartInfo()->extraOptionsInfo.cheatsAllowed)
		return false;

	//Make cheat name case-insensitive, but keep words/parameters (e.g. creature name) as it
	std::string cheatName = boost::to_lower_copy(words[0]);
	words.erase(words.begin());

	//	VCMI                       VCMI simple        SoD/HotA                   AB                    RoE
	std::vector<std::string> localCheats = {
		                           "vcmicolor",       "nwcphisherprice",
		                           "vcmigray"
	};
	std::vector<std::string> townTargetedCheats = {
		"vcmiarmenelos",           "vcmibuild",       "nwczion",                 "nwccoruscant",       "nwconlyamodel"
	};
	std::vector<std::string> playerTargetedCheats = {
		"vcmiformenos",            "vcmiresources",   "nwctheconstruct",         "nwcwatto",           "nwcshrubbery",
		"vcmimelkor",              "vcmilose",        "nwcbluepill",                                   "nwcsirrobin",
		"vcmisilmaril",            "vcmiwin",         "nwcredpill",                                    "nwctrojanrabbit",
		"vcmieagles",              "vcmimap",         "nwcwhatisthematrix",      "nwcrevealourselves", "nwcgeneraldirection",
		"vcmiungoliant",           "vcmihidemap",     "nwcignoranceisbliss",
		                           "vcmiobelisk",     "nwcoracle",               "nwcprophecy",        "nwcalreadygotone"
	};
	std::vector<std::string> heroTargetedCheats = {
		"vcmiainur",               "vcmiarchangel",   "nwctrinity",              "nwcpadme",           "nwcavertingoureyes",
		"vcmiangband",             "vcmiblackknight", "nwcagents",               "nwcdarthmaul",       "nwcfleshwound"
		"vcmiglaurung",            "vcmicrystal",     "vcmiazure",
		"vcmifaerie",              "vcmiarmy",        "vcminissi",
		"vcmiistari",              "vcmispells",      "nwcthereisnospoon",       "nwcmidichlorians",   "nwctim",
		"vcminoldor",              "vcmimachines",    "nwclotsofguns",           "nwcr2d2",            "nwcantioch",
		"vcmiglorfindel",          "vcmilevel",       "nwcneo",                  "nwcquigon",          "nwcigotbetter",
		"vcminahar",               "vcmimove",        "nwcnebuchadnezzar",       "nwcpodracer",        "nwccoconuts",
		"vcmiforgeofnoldorking",   "vcmiartifacts",
		"vcmiolorin",              "vcmiexp",
		                           "vcmiluck",        "nwcfollowthewhiterabbit",                       "nwccastleanthrax",
		                           "vcmimorale",      "nwcmorpheus",                                   "nwcmuchrejoicing",
		                           "vcmigod",         "nwctheone",
		                           "vcmiscrolls",
		                           "vcmiskill"
	};

	if(vstd::contains(localCheats, cheatName))
	{
		executeCheatCode(cheatName, player, currObj, words);
		return true;
	}

	if (!vstd::contains(townTargetedCheats, cheatName) && !vstd::contains(playerTargetedCheats, cheatName) && !vstd::contains(heroTargetedCheats, cheatName))
		return false;

	bool playerTargetedCheat = false;

	for (const auto & i : gameHandler->gameState().players)
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

		PlayerCheated pc;
		pc.player = i.first;
		gameHandler->sendAndApply(pc);

		playerTargetedCheat = true;
		parameters.erase(parameters.begin());

		if (vstd::contains(playerTargetedCheats, cheatName))
			executeCheatCode(cheatName, i.first, ObjectInstanceID::NONE, parameters);

		if (vstd::contains(townTargetedCheats, cheatName))
			for (const auto & t : i.second.getTowns())
				executeCheatCode(cheatName, i.first, t->id, parameters);

		if (vstd::contains(heroTargetedCheats, cheatName))
			for (const auto & h : i.second.getHeroes())
				executeCheatCode(cheatName, i.first, h->id, parameters);
	}

	PlayerCheated pc;
	pc.player = player;
	gameHandler->sendAndApply(pc);
	
	if (!playerTargetedCheat)
		executeCheatCode(cheatName, player, currObj, words);
	
	return true;
}

void PlayerMessageProcessor::executeCheatCode(const std::string & cheatName, PlayerColor player, ObjectInstanceID currObj, const std::vector<std::string> & words)
{
	const CGHeroInstance * hero = gameHandler->gameInfo().getHero(currObj);
	const CGTownInstance * town = gameHandler->gameInfo().getTown(currObj);
	if (!town && hero)
		town = hero->getVisitedTown();

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
	const auto & doCheatRevealPuzzle = [&]() { cheatPuzzleReveal(player); };
	const auto & doCheatMaxLuck = [&]() { cheatMaxLuck(player, hero); };
	const auto & doCheatMaxMorale = [&]() { cheatMaxMorale(player, hero); };
	const auto & doCheatGiveScrolls = [&]() { cheatGiveScrolls(player, hero); };
	const auto & doCheatTheOne = [&]()
	{
		if(!hero)
			return;
		cheatMapReveal(player, true);
		cheatGiveArmy(player, hero, { "archangel", "5" });
		cheatMovement(player, hero, { });
		cheatFly(player, hero);
	};
	const auto & doCheatColorSchemeChange = [&](ColorScheme filter) { cheatColorSchemeChange(player, filter); };
	const auto & doCheatSkill = [&]() { cheatSkill(player, hero, words); };

	std::map<std::string, std::function<void()>> callbacks = {
		{"vcmiainur",               [&] () {doCheatGiveArmyFixed({ "archangel", "5" });}        },
		{"nwctrinity",              [&] () {doCheatGiveArmyFixed({ "archangel", "5" });}        },
		{"nwcpadme",                [&] () {doCheatGiveArmyFixed({ "archangel", "5" });}        },
		{"nwcavertingoureyes",      [&] () {doCheatGiveArmyFixed({ "archangel", "5" });}        },
		{"vcmiangband",             [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });}     },
		{"vcmiglaurung",            [&] () {doCheatGiveArmyFixed({ "crystalDragon", "5000" });} },
		{"vcmiarchangel",           [&] () {doCheatGiveArmyFixed({ "archangel", "5" });}        },
		{"nwcagents",               [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });}     },
		{"nwcdarthmaul",            [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });}     },
		{"nwcfleshwound",           [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });}     },
		{"vcmiblackknight",         [&] () {doCheatGiveArmyFixed({ "blackKnight", "10" });}     },
		{"vcmicrystal",             [&] () {doCheatGiveArmyFixed({ "crystalDragon", "5000" });} },
		{"vcmiazure",               [&] () {doCheatGiveArmyFixed({ "azureDragon", "5000" });}   },
		{"vcmifaerie",              [&] () {doCheatGiveArmyFixed({ "fairieDragon", "5000" });}  },
		{"vcmiarmy",                doCheatGiveArmyCustom                                       },
		{"vcminissi",               doCheatGiveArmyCustom                                       },
		{"vcmiistari",              doCheatGiveSpells                                           },
		{"vcmispells",              doCheatGiveSpells                                           },
		{"nwcthereisnospoon",       doCheatGiveSpells                                           },
		{"nwcmidichlorians",        doCheatGiveSpells                                           },
		{"nwctim",                  doCheatGiveSpells                                           },
		{"vcmiarmenelos",           doCheatBuildTown                                            },
		{"vcmibuild",               doCheatBuildTown                                            },
		{"nwczion",                 doCheatBuildTown                                            },
		{"nwccoruscant",            doCheatBuildTown                                            },
		{"nwconlyamodel",           doCheatBuildTown                                            },
		{"vcminoldor",              doCheatGiveMachines                                         },
		{"vcmimachines",            doCheatGiveMachines                                         },
		{"nwclotsofguns",           doCheatGiveMachines                                         },
		{"nwcr2d2",                 doCheatGiveMachines                                         },
		{"nwcantioch",              doCheatGiveMachines                                         },
		{"vcmiforgeofnoldorking",   doCheatGiveArtifacts                                        },
		{"vcmiartifacts",           doCheatGiveArtifacts                                        },
		{"vcmiglorfindel",          doCheatLevelup                                              },
		{"vcmilevel",               doCheatLevelup                                              },
		{"nwcneo",                  doCheatLevelup                                              },
		{"vcmiolorin",              doCheatExperience                                           },
		{"vcmiexp",                 doCheatExperience                                           },
		{"vcminahar",               doCheatMovement                                             },
		{"vcmimove",                doCheatMovement                                             },
		{"nwcnebuchadnezzar",       doCheatMovement                                             },
		{"nwcpodracer",             doCheatMovement                                             },
		{"nwccoconuts",             doCheatMovement                                             },
		{"vcmiformenos",            doCheatResources                                            },
		{"vcmiresources",           doCheatResources                                            },
		{"nwctheconstruct",         doCheatResources                                            },
		{"nwcwatto",                doCheatResources                                            },
		{"nwcshrubbery",            doCheatResources                                            },
		{"nwcbluepill",             doCheatDefeat                                               },
		{"nwcsirrobin",             doCheatDefeat                                               },
		{"vcmimelkor",              doCheatDefeat                                               },
		{"vcmilose",                doCheatDefeat                                               },
		{"nwcredpill",              doCheatVictory                                              },
		{"nwctrojanrabbit",         doCheatVictory                                              },
		{"vcmisilmaril",            doCheatVictory                                              },
		{"vcmiwin",                 doCheatVictory                                              },
		{"nwcwhatisthematrix",      doCheatMapReveal                                            },
		{"nwcrevealourselves",      doCheatMapReveal                                            },
		{"nwcgeneraldirection",     doCheatMapReveal                                            },
		{"vcmieagles",              doCheatMapReveal                                            },
		{"vcmimap",                 doCheatMapReveal                                            },
		{"vcmiungoliant",           doCheatMapHide                                              },
		{"vcmihidemap",             doCheatMapHide                                              },
		{"nwcignoranceisbliss",     doCheatMapHide                                              },
		{"vcmiobelisk",             doCheatRevealPuzzle                                         },
		{"nwcoracle",               doCheatRevealPuzzle                                         },
		{"nwcprophecy",             doCheatRevealPuzzle                                         },
		{"nwcalreadygotone",        doCheatRevealPuzzle                                         },
		{"vcmiluck",                doCheatMaxLuck                                              },
		{"nwcfollowthewhiterabbit", doCheatMaxLuck                                              },
		{"nwccastleanthrax",        doCheatMaxLuck                                              },
		{"vcmimorale",              doCheatMaxMorale                                            },
		{"nwcmorpheus",             doCheatMaxMorale                                            },
		{"nwcmuchrejoicing",        doCheatMaxMorale                                            },
		{"vcmigod",                 doCheatTheOne                                               },
		{"nwctheone",               doCheatTheOne                                               },
		{"vcmiscrolls",             doCheatGiveScrolls                                          },
		{"vcmicolor",               [&] () {doCheatColorSchemeChange(ColorScheme::H2_SCHEME);}  },
		{"nwcphisherprice",         [&] () {doCheatColorSchemeChange(ColorScheme::H2_SCHEME);}  },
		{"vcmigray",                [&] () {doCheatColorSchemeChange(ColorScheme::GRAYSCALE);}  },
		{"vcmiskill",               doCheatSkill                                                },
	};

	assert(callbacks.count(cheatName));
	if (callbacks.count(cheatName))
		callbacks.at(cheatName)();
}

void PlayerMessageProcessor::sendSystemMessage(std::shared_ptr<CConnection> connection, const MetaString & message)
{
	SystemMessage sm;
	sm.text = message;
	connection->sendPack(sm);
}

void PlayerMessageProcessor::sendSystemMessage(std::shared_ptr<CConnection> connection, const std::string & message)
{
	MetaString str;
	str.appendRawString(message);
	sendSystemMessage(connection, str);
}

void PlayerMessageProcessor::broadcastSystemMessage(MetaString message)
{
	SystemMessage sm;
	sm.text = message;
	gameHandler->sendToAllClients(sm);
}

void PlayerMessageProcessor::broadcastSystemMessage(const std::string & message)
{
	MetaString str;
	str.appendRawString(message);
	broadcastSystemMessage(str);
}

void PlayerMessageProcessor::broadcastMessage(PlayerColor playerSender, const std::string & message)
{
	PlayerMessageClient temp_message(playerSender, message);
	gameHandler->sendAndApply(temp_message);
}
