/*
 * CQuest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CQuest.h"

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
#include "../GameConstants.h"
#include "../constants/StringConstants.h"
#include "../CPlayerState.h"
#include "../CSkillHandler.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../modding/ModScope.h"
#include "../modding/ModUtility.h"
#include "../networkPacks/PacksForClient.h"
#include "../spells/CSpellHandler.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN


//TODO: Remove constructor
CQuest::CQuest():
	qid(-1),
	isCompleted(false),
	lastDay(-1),
	killTarget(ObjectInstanceID::NONE),
	textOption(0),
	completedOption(0),
	stackDirection(0),
	isCustomFirst(false),
	isCustomNext(false),
	isCustomComplete(false),
	repeatedQuest(false),
	questName(CQuest::missionName(EQuestMission::NONE))
{
}

static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return LIBRARY->generaltexth->allTexts[id];
}

const std::string & CQuest::missionName(EQuestMission mission)
{
	static const std::array<std::string, 14> names = {
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
		"reachDate"
	};

	if(static_cast<size_t>(mission) < names.size())
		return names[static_cast<size_t>(mission)];
	return names[0];
}

const std::string & CQuest::missionState(int state)
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

bool CQuest::checkMissionArmy(const CQuest * q, const CCreatureSet * army)
{
	return army->hasUnits(q->mission.creatures, true);
}

bool CQuest::checkQuest(const CGHeroInstance * h) const
{
	if(!mission.heroAllowed(h))
		return false;
	
	if(killTarget.hasValue())
	{
		PlayerColor owner = h->getOwner();
		if (!h->cb->getPlayerState(owner)->destroyedObjects.count(killTarget))
			return false;
	}
	return true;
}

void CQuest::completeQuest(IGameEventCallback & gameEvents, const CGHeroInstance *h, bool allowFullArmyRemoval) const
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

void CQuest::addTextReplacements(const IGameInfoCallback * cb, MetaString & text, std::vector<Component> & components) const
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
	
	if(killTarget != ObjectInstanceID::NONE && !heroName.empty())
	{
		components.emplace_back(ComponentType::HERO_PORTRAIT, heroPortrait);
		addKillTargetReplacements(text);
	}
	
	if(killTarget != ObjectInstanceID::NONE && stackToKill != CreatureID::NONE)
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
		text.replaceNumber(lastDay - cb->getDate(Date::DAY));
}

void CQuest::getVisitText(const IGameInfoCallback * cb, MetaString &iwText, std::vector<Component> &components, bool firstVisit, const CGHeroInstance * h) const
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

void CQuest::getRolloverText(const IGameInfoCallback * cb, MetaString &ms, bool onHover) const
{
	if(onHover)
		ms.appendRawString("\n\n");

	std::string questState = missionState(onHover ? 3 : 4);

	ms.appendTextID(TextIdentifier("core", "seerhut", "quest", questName, questState, textOption).get());

	std::vector<Component> components;
	addTextReplacements(cb, ms, components);
}

void CQuest::getCompletionText(const IGameInfoCallback * cb, MetaString &iwText) const
{
	iwText.appendRawString(completedText.toString());
	
	std::vector<Component> components;
	addTextReplacements(cb, iwText, components);
}

void CQuest::defineQuestName()
{
	//standard quests
	questName = CQuest::missionName(EQuestMission::NONE);
	if(mission.heroLevel > 0) questName = CQuest::missionName(EQuestMission::LEVEL);
	for(auto & s : mission.primary) if(s) questName = CQuest::missionName(EQuestMission::PRIMARY_SKILL);
	if(killTarget != ObjectInstanceID::NONE && !heroName.empty()) questName = CQuest::missionName(EQuestMission::KILL_HERO);
	if(killTarget != ObjectInstanceID::NONE && stackToKill != CreatureID::NONE) questName = CQuest::missionName(EQuestMission::KILL_CREATURE);
	if(!mission.artifacts.empty()) questName = CQuest::missionName(EQuestMission::ARTIFACT);
	if(!mission.creatures.empty()) questName = CQuest::missionName(EQuestMission::ARMY);
	if(mission.resources.nonZero()) questName = CQuest::missionName(EQuestMission::RESOURCES);
	if(!mission.heroes.empty()) questName = CQuest::missionName(EQuestMission::HERO);
	if(!mission.players.empty()) questName = CQuest::missionName(EQuestMission::PLAYER);
	if(mission.daysPassed > 0) questName = CQuest::missionName(EQuestMission::HOTA_REACH_DATE);
	if(!mission.heroClasses.empty()) questName = CQuest::missionName(EQuestMission::HOTA_HERO_CLASS);
}

void CQuest::addKillTargetReplacements(MetaString &out) const
{
	if(!heroName.empty())
		out.replaceRawString(heroName);
	if(stackToKill != CreatureID::NONE)
	{
		out.replaceNamePlural(stackToKill);
		out.replaceRawString(LIBRARY->generaltexth->arraytxt[147+stackDirection]);
	}
}

void CQuest::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	auto q = handler.enterStruct(fieldName);

	handler.serializeStruct("firstVisitText", firstVisitText);
	handler.serializeStruct("nextVisitText", nextVisitText);
	handler.serializeStruct("completedText", completedText);
	handler.serializeBool("repeatedQuest", repeatedQuest, false);

	if(!handler.saving)
	{
		isCustomFirst = !firstVisitText.empty();
		isCustomNext = !nextVisitText.empty();
		isCustomComplete = !completedText.empty();
	}
	
	handler.serializeInt("timeLimit", lastDay, -1);
	handler.serializeStruct("limiter", mission);
	handler.serializeInstance("killTarget", killTarget, ObjectInstanceID::NONE);

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

IQuestObject::IQuestObject()
	:quest(std::make_shared<CQuest>())
{}

IQuestObject::~IQuestObject() = default;

bool IQuestObject::checkQuest(const CGHeroInstance* h) const
{
	return getQuest().checkQuest(h);
}

void CGSeerHut::getVisitText(MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h) const
{
	getQuest().getVisitText(cb, text, components, FirstVisit, h);
}

void CGSeerHut::setObjToKill()
{
	if(getQuest().killTarget == ObjectInstanceID::NONE)
		return;
	
	if(getCreatureToKill(true))
	{
		getQuest().stackToKill = getCreatureToKill(false)->getCreatureID();
		assert(getQuest().stackToKill != CreatureID::NONE);
		getQuest().stackDirection = checkDirection();
	}
	else if(getHeroToKill(true))
	{
		getQuest().heroName = getHeroToKill(false)->getNameTranslated();
		getQuest().heroPortrait = getHeroToKill(false)->getPortraitSource();
	}
}

void CGSeerHut::init(vstd::RNG & rand)
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

void CGSeerHut::initObj(IGameRandomizer & gameRandomizer)
{
	init(gameRandomizer.getDefault());
	
	CRewardableObject::initObj(gameRandomizer);
	
	setObjToKill();
	getQuest().defineQuestName();
	
	if(getQuest().mission == Rewardable::Limiter{} && getQuest().killTarget == ObjectInstanceID::NONE)
		getQuest().isCompleted = true;
	
	if(getQuest().questName == getQuest().missionName(EQuestMission::NONE))
	{
		getQuest().firstVisitText.appendTextID(TextIdentifier("core", "seerhut", "empty", getQuest().completedOption).get());
	}
	else
	{
		if(!getQuest().isCustomFirst)
			getQuest().firstVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", getQuest().questName, getQuest().missionState(0), getQuest().textOption).get());
		if(!getQuest().isCustomNext)
			getQuest().nextVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", getQuest().questName, getQuest().missionState(1), getQuest().textOption).get());
		if(!getQuest().isCustomComplete)
			getQuest().completedText.appendTextID(TextIdentifier("core", "seerhut", "quest", getQuest(). questName, getQuest().missionState(2), getQuest().textOption).get());
	}
	
	getQuest().getCompletionText(cb, configuration.onSelect);
	for(auto & i : configuration.info)
		getQuest().getCompletionText(cb, i.message);
}

void CGSeerHut::getRolloverText(MetaString &text, bool onHover) const
{
	getQuest().getRolloverText(cb, text, onHover);
	if(!onHover)
		text.replaceRawString(seerName);
}

std::string CGSeerHut::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(ID == Obj::SEER_HUT && getQuest().activeForPlayers.count(player))
	{
		hoverName = LIBRARY->generaltexth->allTexts[347];
		boost::algorithm::replace_first(hoverName, "%s", seerName);
	}

	if(getQuest().activeForPlayers.count(player)
	   && (getQuest().mission != Rewardable::Limiter{}
		   || getQuest().killTarget != ObjectInstanceID::NONE)) //rollover when the quest is active
	{
		MetaString ms;
		getRolloverText (ms, true);
		hoverName += ms.toString();
	}
	return hoverName;
}

std::string CGSeerHut::getHoverText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->getOwner());
}

std::string CGSeerHut::getPopupText(PlayerColor player) const
{
	return getHoverText(player);
}

std::string CGSeerHut::getPopupText(const CGHeroInstance * hero) const
{
	return getHoverText(hero->getOwner());
}

std::vector<Component> CGSeerHut::getPopupComponents(PlayerColor player) const
{
	std::vector<Component> result;
	if (getQuest().activeForPlayers.count(player))
		getQuest().mission.loadComponents(result, nullptr);
	return result;
}

std::vector<Component> CGSeerHut::getPopupComponents(const CGHeroInstance * hero) const
{
	std::vector<Component> result;
	if (getQuest().activeForPlayers.count(hero->getOwner()))
		getQuest().mission.loadComponents(result, hero);
	return result;
}

void CGSeerHut::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
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

void CGSeerHut::newTurn(IGameEventCallback & gameEvents, IGameRandomizer & gameRandomizer) const
{
	CRewardableObject::newTurn(gameEvents, gameRandomizer);
	if(getQuest().lastDay >= 0 && getQuest().lastDay <= cb->getDate() - 1) //time is up
	{
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, true);
	}
}

void CGSeerHut::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
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
		iw.text.appendRawString(LIBRARY->generaltexth->seerEmpty[getQuest().completedOption]);
		if (ID == Obj::SEER_HUT)
			iw.text.replaceRawString(seerName);
		gameEvents.showInfoDialog(&iw);
	}
}

int CGSeerHut::checkDirection() const
{
	int3 cord = getCreatureToKill(false)->visitablePos();
	if(static_cast<double>(cord.x) / static_cast<double>(cb->getMapSize().x) < 0.34) //north
	{
		if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.34) //northwest
			return 8;
		else if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.67) //north
			return 1;
		else //northeast
			return 2;
	}
	else if(static_cast<double>(cord.x) / static_cast<double>(cb->getMapSize().x) < 0.67) //horizontal
	{
		if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.34) //west
			return 7;
		else if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.67) //central
			return 9;
		else //east
			return 3;
	}
	else //south
	{
		if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.34) //southwest
			return 6;
		else if(static_cast<double>(cord.y) / static_cast<double>(cb->getMapSize().y) < 0.67) //south
			return 5;
		else //southeast
			return 4;
	}
}

const CGHeroInstance * CGSeerHut::getHeroToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObj(getQuest().killTarget);
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGHeroInstance *>(o);
}

const CGCreature * CGSeerHut::getCreatureToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObj(getQuest().killTarget);
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGCreature *>(o);
}

bool CGSeerHut::allowsFullArmyRemoval() const
{
	bool seerGivesUnits = !configuration.info.empty() && !configuration.info.back().reward.creatures.empty();
	bool h3BugSettingEnabled = cb->getSettings().getBoolean(EGameSettings::MAP_OBJECTS_H3_BUG_QUEST_TAKES_ENTIRE_ARMY);
	return seerGivesUnits || h3BugSettingEnabled;
}

void CGSeerHut::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
	{
		getQuest().completeQuest(gameEvents, hero, allowsFullArmyRemoval());
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, !getQuest().repeatedQuest); //mission complete
	}
	CRewardableObject::blockingDialogAnswered(gameEvents, hero, answer);
}

void CGSeerHut::serializeJsonOptions(JsonSerializeFormat & handler)
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

void CGQuestGuard::init(vstd::RNG & rand)
{
	blockVisit = true;
	getQuest().textOption = rand.nextInt(3, 5);
	getQuest().completedOption = rand.nextInt(4, 5);
	getQuest().mission.hasExtraCreatures = !allowsFullArmyRemoval();
	
	configuration.info.push_back({});
	configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	configuration.info.back().reward.removeObject = subID.getNum() == 0 ? true : false;
	configuration.canRefuse = true;
}

void CGQuestGuard::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if(!getQuest().isCompleted)
		CGSeerHut::onHeroVisit(gameEvents, h);
	else
		gameEvents.setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, false);
}

bool CGQuestGuard::passableFor(PlayerColor color) const
{
	return getQuest().isCompleted;
}

void CGQuestGuard::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest only, do not call base class
	getQuest().serializeJson(handler, "quest");
}

bool CGKeys::wasMyColorVisited(const PlayerColor & player) const
{
	return cb->getPlayerState(player)->visitedObjectsGlobal.count({Obj::KEYMASTER, subID}) != 0;
}

std::string CGKeys::getHoverText(PlayerColor player) const
{
	return getObjectName() + "\n" + visitedTxt(wasMyColorVisited(player));
}

std::string CGKeys::getObjectName() const
{
	return LIBRARY->generaltexth->tentColors[subID.getNum()] + " " + CGObjectInstance::getObjectName();
}

std::string CGKeys::getObjectDescription(PlayerColor player) const
{
	return visitedTxt(wasMyColorVisited(player));
}

bool CGKeymasterTent::wasVisited (PlayerColor player) const
{
	return wasMyColorVisited (player);
}

void CGKeymasterTent::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
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

void CGBorderGuard::initObj(IGameRandomizer & gameRandomizer)
{
	blockVisit = true;
}

void CGBorderGuard::getVisitText(MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h) const
{
	text.appendLocalString(EMetaText::ADVOB_TXT, 18);
}

void CGBorderGuard::getRolloverText(MetaString &text, bool onHover) const
{
	if (!onHover)
	{
		text.appendRawString(LIBRARY->generaltexth->tentColors[subID.getNum()]);
		text.appendRawString(" ");
		text.appendRawString(LIBRARY->objtypeh->getObjectName(Obj::KEYMASTER, subID));
	}
}

bool CGBorderGuard::checkQuest(const CGHeroInstance * h) const
{
	return wasMyColorVisited (h->tempOwner);
}

void CGBorderGuard::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const
{
	if (wasMyColorVisited (h->getOwner()) )
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.text.appendLocalString (EMetaText::ADVOB_TXT, 17);
		gameEvents.showBlockingDialog (this, &bd);
	}
	else
	{
		h->showInfoDialog(gameEvents, 18);

		AddQuest aq;
		aq.quest = QuestInfo(id);
		aq.player = h->tempOwner;
		gameEvents.sendAndApply(aq);
		//TODO: add this quest only once OR check for multiple instances later
	}
}

void CGBorderGuard::blockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance *hero, int32_t answer) const
{
	if (answer)
		gameEvents.removeObject(this, hero->getOwner());
}

void CGBorderGate::onHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance * h) const //TODO: passability
{
	if (!wasMyColorVisited (h->getOwner()) )
	{
		h->showInfoDialog(gameEvents, 18);

		AddQuest aq;
		aq.quest = QuestInfo(id);
		aq.player = h->tempOwner;
		gameEvents.sendAndApply(aq);
	}
}

bool CGBorderGate::passableFor(PlayerColor color) const
{
	return wasMyColorVisited(color);
}

VCMI_LIB_NAMESPACE_END
