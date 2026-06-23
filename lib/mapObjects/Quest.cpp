/*
 * Quest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Quest.h"

#include <vcmi/spells/Spell.h>

#include "../CSoundBase.h"
#include "../texts/CGeneralTextHandler.h"
#include "CGCreature.h"
#include "../IGameSettings.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../entities/artifact/CArtifact.h"
#include "../entities/hero/CHeroHandler.h"
#include "../entities/ResourceTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../spells/CSpellHandler.h"
#include "../GameConstants.h"
#include "../constants/StringConstants.h"
#include "../CPlayerState.h"
#include "../CSkillHandler.h"
#include "../mapping/CMap.h"
#include "CGHeroInstance.h"
#include "../modding/ModScope.h"
#include "../modding/ModUtility.h"
#include "../networkPacks/PacksForClient.h"

#include <vstd/RNG.h>


//TODO: Remove constructor

static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return LIBRARY->generaltexth->translate("core.genrltxt", id);
}

const std::string & Quest::missionName(EQuestMission mission)
{
	static const std::array<std::string, 16> names = {
		"empty",
		"heroLevel",
		"primarySkill",
		"killHero",
		"killCreature",
		"bringArt",
		"bringCreature",
		"bringResources",
		"bringHero",
		"bringPlayer",
		"hotaINVALID", // only used for h3m parsing
		"keymaster",
		"heroClass",
		"reachDate",
		"gameDifficulty",
		"scripted"
	};

	if(static_cast<size_t>(mission) < names.size())
		return names[static_cast<size_t>(mission)];
	return names[0];
}

const std::string & Quest::missionState(int state)
{
	static const std::array<std::string, 5> states = {
		"receive",
		"visit",
		"complete",
		"hover",
		"description",
	};

	if(state < states.size())
		return states[state];
	return states[0];
}

bool Quest::checkMissionArmy(const Quest * q, const CCreatureSet * army)
{
	return army->hasUnits(q->mission.creatures, true);
}

bool Quest::checkQuest(const CGHeroInstance * h) const
{
	return mission.heroAllowed(h);
}

void Quest::completeQuest(IGameEventCallback & gameEvents, const CGHeroInstance *h, bool allowFullArmyRemoval) const
{
	// FIXME: this should be part of 'reward', and not hacking into limiter state that should only limit access to such reward

	for(auto & elem : mission.artifacts)
	{
		// hero does not have such artifact alone, but he might have it as part of assembled artifact
		if(!h->hasArt(elem))
		{
			const auto * assembly = h->getCombinedArtWithPart(elem);
			if (assembly)
			{
				DisassembledArtifact da;
				da.al = ArtifactLocation(h->id, h->getArtPos(assembly));
				gameEvents.sendAndApply(da);
			}
		}

		if(h->hasArt(elem))
			gameEvents.removeArtifact(ArtifactLocation(h->id, h->getArtPos(elem, false)));
		else
			logGlobal->error("Failed to find artifact %s in inventory of hero %s", elem.toEntity(LIBRARY)->getJsonKey(), h->getHeroTypeID());
	}

	gameEvents.takeCreatures(h->id, mission.creatures, allowFullArmyRemoval);
	gameEvents.giveResources(h->getOwner(), -mission.resources);
}

void Quest::addTextReplacements(const IGameInfoCallback * cb, MetaString & text, std::vector<Component> & components) const
{
	if(mission.heroLevel > 0)
		text.replaceNumber(mission.heroLevel);
	
	if(mission.heroExperience > 0)
		text.replaceNumber(mission.heroExperience);
	
	{ //primary skills
		MetaString loot;
		for(int i = 0; i < 4; ++i)
		{
			if(mission.primary[i])
			{
				loot.appendRawString("%d %s");
				loot.replaceNumber(mission.primary[i]);
				loot.replaceRawString(LIBRARY->generaltexth->primarySkillNames[i]);
			}
		}
		
		for(auto & skill : mission.secondary)
		{
			loot.appendTextID(LIBRARY->skillh->getById(skill.first)->getNameTextID());
		}
		
		for(auto & spell : mission.spells)
		{
			loot.appendTextID(LIBRARY->spellh->getById(spell)->getNameTextID());
		}
		
		if(!loot.empty())
			text.replaceRawString(loot.buildList());
	}
	
	if(missionKind == EQuestMission::KILL_HERO)
	{
		components.emplace_back(ComponentType::HERO_PORTRAIT, heroPortrait);
		addKillTargetReplacements(text);
	}

	if(missionKind == EQuestMission::KILL_CREATURE)
	{
		components.emplace_back(ComponentType::CREATURE, stackToKill);
		addKillTargetReplacements(text);
	}
	
	if(!mission.heroes.empty())
		text.replaceRawString(LIBRARY->heroh->getById(mission.heroes.front())->getNameTranslated());
	
	if(!mission.artifacts.empty())
	{
		MetaString loot;
		for(const auto & elem : mission.artifacts)
		{
			loot.appendRawString("%s");
			loot.replaceName(elem);
		}
		text.replaceRawString(loot.buildList());
	}
	
	if(!mission.creatures.empty())
	{
		MetaString loot;
		for(const auto & elem : mission.creatures)
		{
			loot.appendRawString("%s");
			loot.replaceName(elem);
		}
		text.replaceRawString(loot.buildList());
	}
	
	if(mission.resources.nonZero())
	{
		MetaString loot;
		for(auto i : LIBRARY->resourceTypeHandler->getAllObjects())
		{
			if(mission.resources[i])
			{
				loot.appendRawString("%d %s");
				loot.replaceNumber(mission.resources[i]);
				loot.replaceName(i);
			}
		}
		text.replaceRawString(loot.buildList());
	}
	
	if(!mission.players.empty())
	{
		MetaString loot;
		for(auto & p : mission.players)
			loot.appendName(p);
		
		text.replaceRawString(loot.buildList());
	}
	
	if(lastDay >= 0)
		text.replaceNumber(lastDay - cb->getCalendar().getCurrentDay());
}

void Quest::getVisitText(const IGameInfoCallback * cb, MetaString &iwText, std::vector<Component> &components, bool firstVisit, const CGHeroInstance * h) const
{
	bool failRequirements = (h ? !checkQuest(h) : true);
	mission.loadComponents(components, h);

	if(firstVisit)
		iwText.appendRawString(firstVisitText.toString());
	else if(failRequirements)
		iwText.appendRawString(nextVisitText.toString());
	
	if(lastDay >= 0)
		iwText.appendTextID(TextIdentifier("core", "seerhut", "time", textOption).get());
	
	addTextReplacements(cb, iwText, components);
}

void Quest::getRolloverText(const IGameInfoCallback * cb, MetaString &ms, bool onHover) const
{
	if(onHover)
		ms.appendRawString("\n\n");

	std::string questState = missionState(onHover ? 3 : 4);

	ms.appendTextID(TextIdentifier("core", "seerhut", "quest", missionName(missionKind), questState, textOption).get());

	std::vector<Component> components;
	addTextReplacements(cb, ms, components);
}

void Quest::getCompletionText(const IGameInfoCallback * cb, MetaString &iwText) const
{
	iwText.appendRawString(completedText.toString());
	
	std::vector<Component> components;
	addTextReplacements(cb, iwText, components);
}

void Quest::defineQuestName()
{
	//standard quests
	missionKind = EQuestMission::NONE;
	if(!mission.requiredKeys.empty()) missionKind = EQuestMission::KEYMASTER;
	if(mission.heroLevel > 0) missionKind = EQuestMission::LEVEL;
	for(auto & s : mission.primary) if(s) missionKind = EQuestMission::PRIMARY_SKILL;
	if(!mission.destroyedObjects.empty() && !heroName.empty()) missionKind = EQuestMission::KILL_HERO;
	if(!mission.destroyedObjects.empty() && stackToKill != CreatureID::NONE) missionKind = EQuestMission::KILL_CREATURE;
	if(!mission.artifacts.empty()) missionKind = EQuestMission::ARTIFACT;
	if(!mission.creatures.empty()) missionKind = EQuestMission::ARMY;
	if(mission.resources.nonZero()) missionKind = EQuestMission::RESOURCES;
	if(!mission.heroes.empty()) missionKind = EQuestMission::HERO;
	if(!mission.players.empty()) missionKind = EQuestMission::PLAYER;
	if(mission.daysPassed > 0) missionKind = EQuestMission::HOTA_REACH_DATE;
	if(!mission.heroClasses.empty()) missionKind = EQuestMission::HOTA_HERO_CLASS;
	if(!mission.allowedDifficulties.allowsAll()) missionKind = EQuestMission::HOTA_GAME_DIFFICULTY;
}

void Quest::addKillTargetReplacements(MetaString &out) const
{
	if(!heroName.empty())
		out.replaceRawString(heroName);
	if(stackToKill != CreatureID::NONE)
	{
		out.replaceNamePlural(stackToKill);
		out.replaceTextID(TextIdentifier("core", "arraytxt", 147 + stackDirection).get());
	}
}

void Quest::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	auto q = handler.enterStruct(fieldName);

	handler.serializeStruct("firstVisitText", firstVisitText);
	handler.serializeStruct("nextVisitText", nextVisitText);
	handler.serializeStruct("completedText", completedText);
	handler.serializeBool("repeatedQuest", repeatedQuest, false);

	handler.serializeInt("timeLimit", lastDay, -1);
	handler.serializeStruct("limiter", mission);

	// kill quests have a single target; kept as a scalar "killTarget" key for map
	// compatibility, but stored in the limiter as mission.destroyedObjects
	ObjectInstanceID killTarget = mission.destroyedObjects.empty() ? ObjectInstanceID::NONE : mission.destroyedObjects.front();
	handler.serializeInstance("killTarget", killTarget, ObjectInstanceID::NONE);
	if(!handler.saving && killTarget.hasValue())
		mission.destroyedObjects.push_back(killTarget);

	if(!handler.saving) //compatibility with legacy vmaps
	{
		std::string missionType = "None";
		handler.serializeString("missionType", missionType);
		if(missionType == "None")
			return;
		
		if(missionType == "Level")
			handler.serializeInt("heroLevel", mission.heroLevel);
		
		if(missionType == "PrimaryStat")
		{
			auto primarySkills = handler.enterStruct("primarySkills");
			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
				handler.serializeInt(NPrimarySkill::names[i], mission.primary[i], 0);
		}
		
		if(missionType == "Artifact")
			handler.serializeIdArray<ArtifactID>("artifacts", mission.artifacts);
		
		if(missionType == "Army")
		{
			auto a = handler.enterArray("creatures");
			a.serializeStruct(mission.creatures);
		}
		
		if(missionType == "Resources")
		{
			auto r = handler.enterStruct("resources");
			
			for(auto & idx : LIBRARY->resourceTypeHandler->getAllObjects())
				handler.serializeInt(idx.toResource()->getJsonKey(), mission.resources[idx], 0);
		}
		
		if(missionType == "Hero")
		{
			HeroTypeID temp;
			handler.serializeId("hero", temp, HeroTypeID::NONE);
			mission.heroes.emplace_back(temp);
		}
		
		if(missionType == "Player")
		{
			PlayerColor temp;
			handler.serializeId("player", temp, PlayerColor::NEUTRAL);
			mission.players.emplace_back(temp);
		}
	}

}

bool QuestSource::checkQuest(const CGHeroInstance* h) const
{
	return getQuest().checkQuest(h);
}

void QuestSource::getVisitText(MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h) const
{
	activeQuest().getVisitText(cb, text, components, FirstVisit, h);
}

// Map a position into the seer-hut compass text slot (1-9), matching the
// core.arraytxt direction strings ("in the north", "in the north-east", …).
static int compassDirection(const int3 & pos, const int3 & mapSize)
{
	// thirds of the map: x/size < 1/3 and < 2/3, kept as integer comparisons
	if(3 * pos.x < mapSize.x) //north
		return 3 * pos.y < mapSize.y ? 8 : (3 * pos.y < 2 * mapSize.y ? 1 : 2);
	if(3 * pos.x < 2 * mapSize.x) //horizontal
		return 3 * pos.y < mapSize.y ? 7 : (3 * pos.y < 2 * mapSize.y ? 9 : 3);
	//south
	return 3 * pos.y < mapSize.y ? 6 : (3 * pos.y < 2 * mapSize.y ? 5 : 4);
}

void SeerHut::setObjToKill()
{
	if(getQuest().mission.destroyedObjects.empty())
		return;

	if(getCreatureToKill(true))
	{
		getQuest().stackToKill = getCreatureToKill(false)->getCreatureID();
		assert(getQuest().stackToKill != CreatureID::NONE);
		getQuest().stackDirection = compassDirection(getCreatureToKill(false)->visitablePos(), cb->getMapSize());
	}
	else if(getHeroToKill(true))
	{
		getQuest().heroName = getHeroToKill(false)->getNameTranslated();
		getQuest().heroPortrait = getHeroToKill(false)->getPortraitSource();
	}
}

void SeerHut::init(vstd::RNG & rand)
{
	auto names = LIBRARY->generaltexth->findStringsWithPrefix("core.seerhut.names");

	auto seerNameID = *RandomGeneratorUtil::nextItem(names, rand);
	seerName = LIBRARY->generaltexth->translate(seerNameID);
	getQuest().textOption = rand.nextInt(2);
	getQuest().completedOption = rand.nextInt(1, 3);
	getQuest().mission.hasExtraCreatures = !allowsFullArmyRemoval();

	configuration.canRefuse = true;
	configuration.visitMode = Rewardable::EVisitMode::VISIT_ONCE;
	configuration.selectMode = Rewardable::ESelectMode::SELECT_PLAYER;
}

void SeerHut::initObj(IGameRandomizer & gameRandomizer)
{
	init(gameRandomizer.getDefault());
	
	CRewardableObject::initObj(gameRandomizer);
	
	setObjToKill();
	getQuest().defineQuestName();
	
	if(getQuest().mission == Rewardable::Limiter{})
		getQuest().isCompleted = true;
	
	if(getQuest().missionKind == EQuestMission::NONE)
	{
		getQuest().firstVisitText.appendTextID(TextIdentifier("core", "seerhut", "empty", getQuest().completedOption).get());
	}
	else if(getQuest().missionKind == EQuestMission::KEYMASTER)
	{
		if(getQuest().firstVisitText.empty())
			getQuest().firstVisitText.appendTextID(TextIdentifier("core", "advevent", 18).get());
	}
	else
	{
		const std::string & questName = Quest::missionName(getQuest().missionKind);
		if(getQuest().firstVisitText.empty())
			getQuest().firstVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", questName, getQuest().missionState(0), getQuest().textOption).get());
		if(getQuest().nextVisitText.empty())
			getQuest().nextVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", questName, getQuest().missionState(1), getQuest().textOption).get());
		if(getQuest().completedText.empty())
			getQuest().completedText.appendTextID(TextIdentifier("core", "seerhut", "quest", questName, getQuest().missionState(2), getQuest().textOption).get());
	}
	
	getQuest().getCompletionText(cb, configuration.onSelect);
	for(auto & i : configuration.info)
		getQuest().getCompletionText(cb, i.message);
}

void SeerHut::getRolloverText(MetaString &text, bool onHover) const
{
	getQuest().getRolloverText(cb, text, onHover);
	if(!onHover)
		text.replaceRawString(seerName);
}

std::string SeerHut::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(ID == Obj::SEER_HUT && getQuest().activeForPlayers.count(player))
	{
		hoverName = LIBRARY->generaltexth->translate("core.genrltxt", 347);
		boost::algorithm::replace_first(hoverName, "%s", seerName);
	}

	if(getQuest().activeForPlayers.count(player)
	   && getQuest().mission != Rewardable::Limiter{}) //rollover when the quest is active
	{
		MetaString ms;
		getRolloverText (ms, true);
		hoverName += ms.toString();
	}
	return hoverName;
}

std::string SeerHut::getHoverText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->getOwner());
}

std::string SeerHut::getPopupText(PlayerColor player) const
{
	return getHoverText(player);
}

std::string SeerHut::getPopupText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->getOwner());
}

std::vector<Component> SeerHut::getPopupComponents(PlayerColor player) const
{
	std::vector<Component> result;
	if (getQuest().activeForPlayers.count(player))
		getQuest().mission.loadComponents(result, nullptr);
	return result;
}

std::vector<Component> SeerHut::getPopupComponents(const CGHeroInstance * hero) const
{
	std::vector<Component> result;
	if (getQuest().activeForPlayers.count(hero->getOwner()))
		getQuest().mission.loadComponents(result, hero);
	return result;
}

void SeerHut::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch(what)
	{
		case ObjProperty::SEERHUT_VISITED:
		{
			getQuest().activeForPlayers.emplace(identifier.as<PlayerColor>());
			break;
		}
		case ObjProperty::SEERHUT_COMPLETE:
		{
			getQuest().isCompleted = identifier.getNum();
			getQuest().activeForPlayers.clear();
			break;
		}
	}
}

void SeerHut::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	CRewardableObject::newTurn(gameEvents, gameRandomizer);
	if(getQuest().lastDay >= 0 && getQuest().lastDay <= cb->getCalendar().getCurrentDay() - 1) //time is up
	{
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, true);
	}
}

void SeerHut::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if(!getQuest().isCompleted)
	{
		bool firstVisit = !getQuest().activeForPlayers.count(h->getOwner());
		bool failRequirements = !checkQuest(h);

		if(firstVisit)
		{
			gameEvents.setObjPropertyID(id, ObjProperty::SEERHUT_VISITED, h->getOwner());

			AddQuest aq;
			aq.quest = QuestInfo(id);
			aq.player = h->tempOwner;
			gameEvents.sendAndApply(aq); //TODO: merge with setObjProperty?
		}

		if(firstVisit || failRequirements)
		{
			getVisitText (iw.text, iw.components, firstVisit, h);

			gameEvents.showInfoDialog(&iw);
		}
		if(!failRequirements) // propose completion, also on first visit
		{
			CRewardableObject::onHeroVisit(gameEvents, h);
			return;
		}
	}
	else
	{
		iw.text.appendTextID(TextIdentifier("core", "seerhut", "empty", getQuest().completedOption).get());
		if (ID == Obj::SEER_HUT)
			iw.text.replaceRawString(seerName);
		gameEvents.showInfoDialog(&iw);
	}
}

const CGHeroInstance * SeerHut::getHeroToKill(bool allowNull) const
{
	const CGObjectInstance *o = getQuest().mission.destroyedObjects.empty() ? nullptr : cb->getObj(getQuest().mission.destroyedObjects.front());
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGHeroInstance *>(o);
}

const CGCreature * SeerHut::getCreatureToKill(bool allowNull) const
{
	const CGObjectInstance *o = getQuest().mission.destroyedObjects.empty() ? nullptr : cb->getObj(getQuest().mission.destroyedObjects.front());
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGCreature *>(o);
}

bool SeerHut::allowsFullArmyRemoval() const
{
	bool seerGivesUnits = !configuration.info.empty() && !configuration.info.back().reward.creatures.empty();
	bool h3BugSettingEnabled = cb->getSettings().getBoolean(EGameSettings::MAP_OBJECTS_H3_BUG_QUEST_TAKES_ENTIRE_ARMY);
	return seerGivesUnits || h3BugSettingEnabled;
}

void SeerHut::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
	{
		getQuest().completeQuest(gameEvents, hero, allowsFullArmyRemoval());
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, !getQuest().repeatedQuest); //mission complete
	}
	CRewardableObject::blockingDialogAnswered(gameEvents, hero, answer);
}

void SeerHut::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest and reward
	CRewardableObject::serializeJsonOptions(handler);
	getQuest().serializeJson(handler, "quest");
	
	if(!handler.saving)
	{
		//backward compatibility for VCMI maps that use old SeerHut format
		auto s = handler.enterStruct("reward");
		const JsonNode & rewardsJson = handler.getCurrent();

		if (rewardsJson.Struct().empty())
			return;
		
		std::string fullIdentifier;
		std::string metaTypeName;
		std::string scope;
		std::string identifier;

		auto iter = rewardsJson.Struct().begin();
		fullIdentifier = iter->first;

		ModUtility::parseIdentifier(fullIdentifier, scope, metaTypeName, identifier);
		if(!std::set<std::string>{"resource", "primarySkill", "secondarySkill", "artifact", "spell", "creature", "experience", "mana", "morale", "luck"}.count(metaTypeName))
			return;

		int val = 0;
		handler.serializeInt(fullIdentifier, val);
		
		Rewardable::VisitInfo vinfo;
		auto & reward = vinfo.reward;
		if(metaTypeName == "experience")
		   reward.heroExperience = val;
		if(metaTypeName == "mana")
			reward.manaDiff = val;
		if(metaTypeName == "morale")
			reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id)));
		if(metaTypeName == "luck")
			reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id)));
		if(metaTypeName == "resource")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.resources[rawId] = val;
		}
		if(metaTypeName == "primarySkill")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.primary.at(rawId) = val;
		}
		if(metaTypeName == "secondarySkill")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.secondary[rawId] = val;
		}
		if(metaTypeName == "artifact")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.grantedArtifacts.push_back(rawId);
		}
		if(metaTypeName == "spell")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.spells.push_back(rawId);
		}
		if(metaTypeName == "creature")
		{
			auto rawId = *LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.creatures.emplace_back(rawId, val);
		}
		
		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		configuration.info.push_back(vinfo);
	}
}

void QuestGuard::init(vstd::RNG & rand)
{
	blockVisit = true;

	// border guard: synthetic "owns the matching key" quest, handled manually in onHeroVisit
	if(!getQuest().mission.requiredKeys.empty())
		return;

	getQuest().textOption = rand.nextInt(3, 5);
	getQuest().completedOption = rand.nextInt(4, 5);
	getQuest().mission.hasExtraCreatures = !allowsFullArmyRemoval();
	
	configuration.info.push_back({});
	configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	configuration.info.back().reward.removeObject = subID.getNum() == 0 ? true : false;
	configuration.canRefuse = true;
}

void QuestGuard::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	// border guard: offer demolition once the key is held, otherwise log the requirement
	if(!getQuest().mission.requiredKeys.empty())
	{
		if(checkQuest(h))
		{
			BlockingDialog bd(true, false);
			bd.player = h->getOwner();
			bd.text.appendTextID(TextIdentifier("core", "advevent", 17).get());
			gameEvents.showBlockingDialog(this, &bd);
		}
		else
		{
			h->showInfoDialog(gameEvents, 18);

			AddQuest aq;
			aq.quest = QuestInfo(id);
			aq.player = h->tempOwner;
			gameEvents.sendAndApply(aq);
		}
		return;
	}

	if(!getQuest().isCompleted)
		SeerHut::onHeroVisit(gameEvents, h);
	else
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, false);
}

void QuestGuard::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer) const
{
	// border guard demolition prompt: positive answer tears it down
	if(!getQuest().mission.requiredKeys.empty())
	{
		if(answer)
			gameEvents.removeObject(this, hero->getOwner());
		return;
	}

	SeerHut::blockingDialogAnswered(gameEvents, hero, answer);
}

bool QuestGuard::passableFor(PlayerColor color) const
{
	return getQuest().isCompleted;
}

void QuestGuard::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest only, do not call base class
	getQuest().serializeJson(handler, "quest");
}

bool QuestSource::hasVisitedKeymaster(const CGObjectInstance * keyObject, PlayerColor player)
{
	return keyObject->cb->getPlayerState(player)->visitedObjectsGlobal.count({Obj::KEYMASTER, keyObject->subID}) != 0;
}

std::string QuestSource::keymasterVisitedText(const CGObjectInstance * keyObject, PlayerColor player)
{
	return visitedTxt(hasVisitedKeymaster(keyObject, player));
}

bool KeymasterTent::wasMyColorVisited(const PlayerColor & player) const
{
	return QuestSource::hasVisitedKeymaster(this, player);
}

std::string KeymasterTent::getHoverText(PlayerColor player) const
{
	return getObjectName() + "\n" + visitedTxt(wasMyColorVisited(player));
}

std::string KeymasterTent::getObjectName() const
{
	return LIBRARY->generaltexth->translate("core.tentcolr", subID.getNum()) + " " + CGObjectInstance::getObjectName();
}

std::string KeymasterTent::getObjectDescription(PlayerColor player) const
{
	return visitedTxt(wasMyColorVisited(player));
}

bool KeymasterTent::wasVisited (PlayerColor player) const
{
	return wasMyColorVisited (player);
}

void KeymasterTent::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	int txt_id;
	if (!wasMyColorVisited (h->getOwner()) )
	{
		ChangeObjectVisitors cow;
		cow.mode = ChangeObjectVisitors::VISITOR_ADD_PLAYER;
		cow.hero = h->id;
		cow.object = id;
		gameEvents.sendAndApply(cow);
		txt_id=19;
	}
	else
		txt_id=20;
	h->showInfoDialog(gameEvents, txt_id);
}

void QuestGate::initObj(IGameRandomizer & gameRandomizer)
{
	CRewardableObject::initObj(gameRandomizer);
	getQuest().defineQuestName();
	if(getQuest().firstVisitText.empty())
		getQuest().firstVisitText.appendTextID(TextIdentifier("core", "advevent", 18).get());
}

void QuestGate::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(passableFor(h->getOwner()))
		return;

	h->showInfoDialog(gameEvents, 18);

	AddQuest aq;
	aq.quest = QuestInfo(id);
	aq.player = h->tempOwner;
	gameEvents.sendAndApply(aq);
}

bool QuestGate::passableFor(PlayerColor color) const
{
	for(const auto & key : getQuest().mission.requiredKeys)
		if(!cb->getPlayerState(color)->visitedObjectsGlobal.count({Obj::KEYMASTER, key}))
			return false;
	return true;
}
