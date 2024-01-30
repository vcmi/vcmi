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

#include "../ArtifactUtils.h"
#include "../CSoundBase.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "CGCreature.h"
#include "../IGameCallback.h"
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
#include "../CRandomGenerator.h"

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
	questName(CQuest::missionName(0))
{
}

static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

const std::string & CQuest::missionName(int mission)
{
	static const std::array<std::string, 13> names = {
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
		"keymaster",
		"hota",
		"other"
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
	std::vector<CStackBasicDescriptor>::const_iterator cre;
	TSlots::const_iterator it;
	ui32 count = 0;
	ui32 slotsCount = 0;
	bool hasExtraCreatures = false;
	for(cre = q->mission.creatures.begin(); cre != q->mission.creatures.end(); ++cre)
	{
		for(count = 0, it = army->Slots().begin(); it != army->Slots().end(); ++it)
		{
			if(it->second->type == cre->type)
			{
				count += it->second->count;
				slotsCount++;
			}
		}

		if(static_cast<TQuantity>(count) < cre->count) //not enough creatures of this kind
			return false;

		hasExtraCreatures |= static_cast<TQuantity>(count) > cre->count;
	}

	return hasExtraCreatures || slotsCount < army->Slots().size();
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

void CQuest::completeQuest(IGameCallback * cb, const CGHeroInstance *h) const
{
	for(auto & elem : mission.artifacts)
	{
		if(h->hasArt(elem))
		{
			cb->removeArtifact(ArtifactLocation(h->id, h->getArtPos(elem, false)));
		}
		else
		{
			const auto * assembly = h->getAssemblyByConstituent(elem);
			assert(assembly);
			auto parts = assembly->getPartsInfo();

			// Remove the assembly
			cb->removeArtifact(ArtifactLocation(h->id, h->getArtPos(assembly)));

			// Disassemble this backpack artifact
			for(const auto & ci : parts)
			{
				if(ci.art->getTypeId() != elem)
					cb->giveHeroNewArtifact(h, ci.art->artType, ArtifactPosition::BACKPACK_START);
			}
		}
	}
			
	cb->takeCreatures(h->id, mission.creatures);
	cb->giveResources(h->getOwner(), mission.resources);
}

void CQuest::addTextReplacements(IGameCallback * cb, MetaString & text, std::vector<Component> & components) const
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
				loot.replaceRawString(VLC->generaltexth->primarySkillNames[i]);
			}
		}
		
		for(auto & skill : mission.secondary)
		{
			loot.appendTextID(VLC->skillh->getById(skill.first)->getNameTextID());
		}
		
		for(auto & spell : mission.spells)
		{
			loot.appendTextID(VLC->spellh->getById(spell)->getNameTextID());
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
		text.replaceRawString(VLC->heroh->getById(mission.heroes.front())->getNameTranslated());
	
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
		for(auto i : GameResID::ALL_RESOURCES())
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

void CQuest::getVisitText(IGameCallback * cb, MetaString &iwText, std::vector<Component> &components, bool firstVisit, const CGHeroInstance * h) const
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

void CQuest::getRolloverText(IGameCallback * cb, MetaString &ms, bool onHover) const
{
	if(onHover)
		ms.appendRawString("\n\n");

	std::string questState = missionState(onHover ? 3 : 4);

	ms.appendTextID(TextIdentifier("core", "seerhut", "quest", questName, questState, textOption).get());

	std::vector<Component> components;
	addTextReplacements(cb, ms, components);
}

void CQuest::getCompletionText(IGameCallback * cb, MetaString &iwText) const
{
	iwText.appendRawString(completedText.toString());
	
	std::vector<Component> components;
	addTextReplacements(cb, iwText, components);
}

void CQuest::defineQuestName()
{
	//standard quests
	questName = CQuest::missionName(0);
	if(mission != Rewardable::Limiter{}) questName = CQuest::missionName(12);
	if(mission.heroLevel > 0) questName = CQuest::missionName(1);
	for(auto & s : mission.primary) if(s) questName = CQuest::missionName(2);
	if(!mission.spells.empty()) questName = CQuest::missionName(2);
	if(!mission.secondary.empty()) questName = CQuest::missionName(2);
	if(killTarget != ObjectInstanceID::NONE && !heroName.empty()) questName = CQuest::missionName(3);
	if(killTarget != ObjectInstanceID::NONE && stackToKill != CreatureID::NONE) questName = CQuest::missionName(4);
	if(!mission.artifacts.empty()) questName = CQuest::missionName(5);
	if(!mission.creatures.empty()) questName = CQuest::missionName(6);
	if(mission.resources.nonZero()) questName = CQuest::missionName(7);
	if(!mission.heroes.empty()) questName = CQuest::missionName(8);
	if(!mission.players.empty()) questName = CQuest::missionName(9);
	if(mission.daysPassed > 0 || !mission.heroClasses.empty()) questName = CQuest::missionName(11);
}

void CQuest::addKillTargetReplacements(MetaString &out) const
{
	if(!heroName.empty())
		out.replaceRawString(heroName);
	if(stackToKill != CreatureID::NONE)
	{
		out.replaceNamePlural(stackToKill);
		out.replaceRawString(VLC->generaltexth->arraytxt[147+stackDirection]);
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

			for(size_t idx = 0; idx < (GameConstants::RESOURCE_QUANTITY - 1); idx++)
			{
				handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], mission.resources[idx], 0);
			}
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

bool IQuestObject::checkQuest(const CGHeroInstance* h) const
{
	return quest->checkQuest(h);
}

void CGSeerHut::getVisitText(MetaString &text, std::vector<Component> &components, bool FirstVisit, const CGHeroInstance * h) const
{
	quest->getVisitText(cb, text, components, FirstVisit, h);
}

void IQuestObject::afterAddToMapCommon(CMap * map) const
{
	map->addNewQuestInstance(quest);
}

void CGSeerHut::setObjToKill()
{
	if(quest->killTarget == ObjectInstanceID::NONE)
		return;
	
	if(getCreatureToKill(true))
	{
		quest->stackToKill = getCreatureToKill(false)->getCreature();
		assert(quest->stackToKill != CreatureID::NONE);
		quest->stackDirection = checkDirection();
	}
	else if(getHeroToKill(true))
	{
		quest->heroName = getHeroToKill(false)->getNameTranslated();
		quest->heroPortrait = getHeroToKill(false)->getPortraitSource();
	}
}

void CGSeerHut::init(CRandomGenerator & rand)
{
	auto names = VLC->generaltexth->findStringsWithPrefix("core.seerhut.names");

	auto seerNameID = *RandomGeneratorUtil::nextItem(names, rand);
	seerName = VLC->generaltexth->translate(seerNameID);
	quest->textOption = rand.nextInt(2);
	quest->completedOption = rand.nextInt(1, 3);
	
	configuration.canRefuse = true;
	configuration.visitMode = Rewardable::EVisitMode::VISIT_ONCE;
	configuration.selectMode = Rewardable::ESelectMode::SELECT_PLAYER;
}

void CGSeerHut::initObj(CRandomGenerator & rand)
{
	init(rand);
	
	CRewardableObject::initObj(rand);
	
	setObjToKill();
	quest->defineQuestName();
	
	if(quest->mission == Rewardable::Limiter{} && quest->killTarget == ObjectInstanceID::NONE)
		quest->isCompleted = true;
	
	if(quest->questName == quest->missionName(0))
	{
		quest->firstVisitText.appendTextID(TextIdentifier("core", "seehut", "empty", quest->completedOption).get());
	}
	else
	{
		if(!quest->isCustomFirst)
			quest->firstVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", quest->questName, quest->missionState(0), quest->textOption).get());
		if(!quest->isCustomNext)
			quest->nextVisitText.appendTextID(TextIdentifier("core", "seerhut", "quest", quest->questName, quest->missionState(1), quest->textOption).get());
		if(!quest->isCustomComplete)
			quest->completedText.appendTextID(TextIdentifier("core", "seerhut", "quest", quest-> questName, quest->missionState(2), quest->textOption).get());
	}
	
	quest->getCompletionText(cb, configuration.onSelect);
	for(auto & i : configuration.info)
		quest->getCompletionText(cb, i.message);
}

void CGSeerHut::getRolloverText(MetaString &text, bool onHover) const
{
	quest->getRolloverText(cb, text, onHover);//TODO: simplify?
	if(!onHover)
		text.replaceRawString(seerName);
}

std::string CGSeerHut::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(ID == Obj::SEER_HUT && quest->activeForPlayers.count(player))
	{
		hoverName = VLC->generaltexth->allTexts[347];
		boost::algorithm::replace_first(hoverName, "%s", seerName);
	}

	if(quest->activeForPlayers.count(player)
	   && (quest->mission != Rewardable::Limiter{}
		   || quest->killTarget != ObjectInstanceID::NONE)) //rollover when the quest is active
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
	if (quest->activeForPlayers.count(player))
		quest->mission.loadComponents(result, nullptr);
	return result;
}

std::vector<Component> CGSeerHut::getPopupComponents(const CGHeroInstance * hero) const
{
	std::vector<Component> result;
	if (quest->activeForPlayers.count(hero->getOwner()))
		quest->mission.loadComponents(result, hero);
	return result;
}

void CGSeerHut::setPropertyDer(ObjProperty what, ObjPropertyID identifier)
{
	switch(what)
	{
		case ObjProperty::SEERHUT_VISITED:
		{
			quest->activeForPlayers.emplace(identifier.as<PlayerColor>());
			break;
		}
		case ObjProperty::SEERHUT_COMPLETE:
		{
			quest->isCompleted = identifier.getNum();
			quest->activeForPlayers.clear();
			break;
		}
	}
}

void CGSeerHut::newTurn(CRandomGenerator & rand) const
{
	CRewardableObject::newTurn(rand);
	if(quest->lastDay >= 0 && quest->lastDay <= cb->getDate() - 1) //time is up
	{
		cb->setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, true);
	}
}

void CGSeerHut::onHeroVisit(const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if(!quest->isCompleted)
	{
		bool firstVisit = !quest->activeForPlayers.count(h->getOwner());
		bool failRequirements = !checkQuest(h);

		if(firstVisit)
		{
			cb->setObjPropertyID(id, ObjProperty::SEERHUT_VISITED, h->getOwner());

			AddQuest aq;
			aq.quest = QuestInfo (quest, this, visitablePos());
			aq.player = h->tempOwner;
			cb->sendAndApply(&aq); //TODO: merge with setObjProperty?
		}

		if(firstVisit || failRequirements)
		{
			getVisitText (iw.text, iw.components, firstVisit, h);

			cb->showInfoDialog(&iw);
		}
		if(!failRequirements) // propose completion, also on first visit
		{
			CRewardableObject::onHeroVisit(h);
			return;
		}
	}
	else
	{
		iw.text.appendRawString(VLC->generaltexth->seerEmpty[quest->completedOption]);
		if (ID == Obj::SEER_HUT)
			iw.text.replaceRawString(seerName);
		cb->showInfoDialog(&iw);
	}
}

int CGSeerHut::checkDirection() const
{
	int3 cord = getCreatureToKill(false)->pos;
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
	const CGObjectInstance *o = cb->getObj(quest->killTarget);
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGHeroInstance *>(o);
}

const CGCreature * CGSeerHut::getCreatureToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObj(quest->killTarget);
	if(allowNull && !o)
		return nullptr;
	return dynamic_cast<const CGCreature *>(o);
}

void CGSeerHut::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	CRewardableObject::blockingDialogAnswered(hero, answer);
	if(answer)
	{
		quest->completeQuest(cb, hero);
		cb->setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, !quest->repeatedQuest); //mission complete
	}
}

void CGSeerHut::afterAddToMap(CMap* map)
{
	IQuestObject::afterAddToMapCommon(map);
}

void CGSeerHut::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest and reward
	CRewardableObject::serializeJsonOptions(handler);
	quest->serializeJson(handler, "quest");
	
	if(!handler.saving)
	{
		//backward compatibility for VCMI maps that use old SeerHut format
		auto s = handler.enterStruct("reward");
		const JsonNode & rewardsJson = handler.getCurrent();
		
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
			reward.bonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id));
		if(metaTypeName == "luck")
			reward.bonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id));
		if(metaTypeName == "resource")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.resources[rawId] = val;
		}
		if(metaTypeName == "primarySkill")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.primary.at(rawId) = val;
		}
		if(metaTypeName == "secondarySkill")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.secondary[rawId] = val;
		}
		if(metaTypeName == "artifact")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.artifacts.push_back(rawId);
		}
		if(metaTypeName == "spell")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.spells.push_back(rawId);
		}
		if(metaTypeName == "creature")
		{
			auto rawId = *VLC->identifiers()->getIdentifier(ModScope::scopeMap(), fullIdentifier, false);
			reward.creatures.emplace_back(rawId, val);
		}
		
		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		configuration.info.push_back(vinfo);
	}
}

void CGQuestGuard::init(CRandomGenerator & rand)
{
	blockVisit = true;
	quest->textOption = rand.nextInt(3, 5);
	quest->completedOption = rand.nextInt(4, 5);
	
	configuration.info.push_back({});
	configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	configuration.info.back().reward.removeObject = subID.getNum() == 0 ? true : false;
	configuration.canRefuse = true;
}

void CGQuestGuard::onHeroVisit(const CGHeroInstance * h) const
{
	if(!quest->isCompleted)
		CGSeerHut::onHeroVisit(h);
	else
		cb->setObjPropertyValue(id, ObjProperty::SEERHUT_COMPLETE, false);
}

bool CGQuestGuard::passableFor(PlayerColor color) const
{
	return quest->isCompleted;
}

void CGQuestGuard::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest only, do not call base class
	quest->serializeJson(handler, "quest");
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
	return VLC->generaltexth->tentColors[subID.getNum()] + " " + CGObjectInstance::getObjectName();
}

bool CGKeymasterTent::wasVisited (PlayerColor player) const
{
	return wasMyColorVisited (player);
}

void CGKeymasterTent::onHeroVisit( const CGHeroInstance * h ) const
{
	int txt_id;
	if (!wasMyColorVisited (h->getOwner()) )
	{
		ChangeObjectVisitors cow;
		cow.mode = ChangeObjectVisitors::VISITOR_GLOBAL;
		cow.hero = h->id;
		cow.object = id;
		cb->sendAndApply(&cow);
		txt_id=19;
	}
	else
		txt_id=20;
	h->showInfoDialog(txt_id);
}

void CGBorderGuard::initObj(CRandomGenerator & rand)
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
		text.appendRawString(VLC->generaltexth->tentColors[subID.getNum()]);
		text.appendRawString(" ");
		text.appendRawString(VLC->objtypeh->getObjectName(Obj::KEYMASTER, subID));
	}
}

bool CGBorderGuard::checkQuest(const CGHeroInstance * h) const
{
	return wasMyColorVisited (h->tempOwner);
}

void CGBorderGuard::onHeroVisit(const CGHeroInstance * h) const
{
	if (wasMyColorVisited (h->getOwner()) )
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.text.appendLocalString (EMetaText::ADVOB_TXT, 17);
		cb->showBlockingDialog (&bd);
	}
	else
	{
		h->showInfoDialog(18);

		AddQuest aq;
		aq.quest = QuestInfo (quest, this, visitablePos());
		aq.player = h->tempOwner;
		cb->sendAndApply (&aq);
		//TODO: add this quest only once OR check for multiple instances later
	}
}

void CGBorderGuard::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if (answer)
		cb->removeObject(this, hero->getOwner());
}

void CGBorderGuard::afterAddToMap(CMap * map)
{
	IQuestObject::afterAddToMapCommon(map);
}

void CGBorderGate::onHeroVisit(const CGHeroInstance * h) const //TODO: passability
{
	if (!wasMyColorVisited (h->getOwner()) )
	{
		h->showInfoDialog(18);

		AddQuest aq;
		aq.quest = QuestInfo (quest, this, visitablePos());
		aq.player = h->tempOwner;
		cb->sendAndApply (&aq);
	}
}

bool CGBorderGate::passableFor(PlayerColor color) const
{
	return wasMyColorVisited(color);
}

VCMI_LIB_NAMESPACE_END
