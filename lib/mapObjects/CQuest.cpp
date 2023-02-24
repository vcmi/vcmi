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

#include "../NetPacks.h"
#include "../CSoundBase.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "CObjectClassesHandler.h"
#include "MiscObjects.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../CModHandler.h"
#include "../GameConstants.h"
#include "../StringConstants.h"
#include "../CSkillHandler.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN


std::map <PlayerColor, std::set <ui8> > CGKeys::playerKeyMap;

//TODO: Remove constructor
CQuest::CQuest():
	qid(-1),
	missionType(MISSION_NONE),
	progress(NOT_ACTIVE),
	lastDay(-1),
	m13489val(0),
	textOption(0),
	completedOption(0),
	stackDirection(0),
	heroPortrait(-1),
	isCustomFirst(false), 
	isCustomNext(false),
	isCustomComplete(false)
{
}

///helpers
static void showInfoDialog(const PlayerColor & playerID, const ui32 txtID, const ui16 soundID = 0)
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = playerID;
	iw.text.addTxt(MetaString::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID = 0)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

const std::string & CQuest::missionName(CQuest::Emission mission)
{
	static const std::array<std::string, 11> names = {
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
		"keymaster"
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
	for(cre = q->m6creatures.begin(); cre != q->m6creatures.end(); ++cre)
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
	switch (missionType)
	{
		case MISSION_NONE:
			return true;
		case MISSION_LEVEL:
			return m13489val <= h->level;
		case MISSION_PRIMARY_STAT:
			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
			{
				if(h->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i)) < static_cast<int>(m2stats[i]))
					return false;
			}
			return true;
		case MISSION_KILL_HERO:
		case MISSION_KILL_CREATURE:
			if(!CGHeroInstance::cb->getObjByQuestIdentifier(m13489val))
				return true;
			return false;
		case MISSION_ART:
			// if the object was deserialized
			if(artifactsRequirements.empty())
				for(const auto & id : m5arts)
					++artifactsRequirements[id];

			for(const auto & elem : artifactsRequirements)
			{
				// check required amount of artifacts
				if(h->getArtPosCount(elem.first, false, true, true) < elem.second)
					return false;
			}
			return true;
		case MISSION_ARMY:
			return checkMissionArmy(this, h);
		case MISSION_RESOURCES:
			for(Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, +1)) //including Mithril ?
			{	//Quest has no direct access to callback
				if(CGHeroInstance::cb->getResource(h->tempOwner, i) < static_cast<int>(m7resources[i]))
					return false;
			}
			return true;
		case MISSION_HERO:
			return m13489val == h->type->getIndex();
		case MISSION_PLAYER:
			return m13489val == h->getOwner().getNum();
		default:
			return false;
	}
}

void CQuest::getVisitText(MetaString &iwText, std::vector<Component> &components, bool isCustom, bool firstVisit, const CGHeroInstance * h) const
{
	std::string text;
	bool failRequirements = (h ? !checkQuest(h) : true);

	if(firstVisit)
	{
		isCustom = isCustomFirst;
		iwText << (text = firstVisitText);
	}
	else if(failRequirements)
	{
		isCustom = isCustomNext;
		iwText << (text = nextVisitText);
	}
	switch (missionType)
	{
		case MISSION_LEVEL:
			components.emplace_back(Component::EXPERIENCE, 0, m13489val, 0);
			if(!isCustom)
				iwText.addReplacement(m13489val);
			break;
		case MISSION_PRIMARY_STAT:
		{
			MetaString loot;
			for(int i = 0; i < 4; ++i)
			{
				if(m2stats[i])
				{
					components.emplace_back(Component::PRIM_SKILL, i, m2stats[i], 0);
					loot << "%d %s";
					loot.addReplacement(m2stats[i]);
					loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
				}
			}
			if (!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_KILL_HERO:
			components.emplace_back(Component::HERO_PORTRAIT, heroPortrait, 0, 0);
			if(!isCustom)
				addReplacements(iwText, text);
			break;
		case MISSION_HERO:
			//FIXME: portrait may not match hero, if custom portrait was set in map editor
			components.emplace_back(Component::HERO_PORTRAIT, VLC->heroh->objects[m13489val]->imageIndex, 0, 0);
			if(!isCustom)
				iwText.addReplacement(VLC->heroh->objects[m13489val]->getNameTranslated());
			break;
		case MISSION_KILL_CREATURE:
			{
				components.emplace_back(stackToKill);
				if(!isCustom)
				{
					addReplacements(iwText, text);
				}
			}
			break;
		case MISSION_ART:
		{
			MetaString loot;
			for(const auto & elem : m5arts)
			{
				components.emplace_back(Component::ARTIFACT, elem, 0, 0);
				loot << "%s";
				loot.addReplacement(MetaString::ART_NAMES, elem);
			}
			if(!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_ARMY:
		{
			MetaString loot;
			for(const auto & elem : m6creatures)
			{
				components.emplace_back(elem);
				loot << "%s";
				loot.addReplacement(elem);
			}
			if(!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_RESOURCES:
		{
			MetaString loot;
			for(int i = 0; i < 7; ++i)
			{
				if(m7resources[i])
				{
					components.emplace_back(Component::RESOURCE, i, m7resources[i], 0);
					loot << "%d %s";
					loot.addReplacement(m7resources[i]);
					loot.addReplacement(MetaString::RES_NAMES, i);
				}
			}
			if(!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_PLAYER:
			components.emplace_back(Component::FLAG, m13489val, 0, 0);
			if(!isCustom)
				iwText.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
	}
}

void CQuest::getRolloverText(MetaString &ms, bool onHover) const
{
	// Quests with MISSION_NONE type don't have a text for them
	assert(missionType != MISSION_NONE);

	if(onHover)
		ms << "\n\n";

	std::string questName = missionName(static_cast<Emission>(missionType - 1));
	std::string questState = missionState(onHover ? 3 : 4);

	ms << VLC->generaltexth->translate("core.seerhut.quest", questName, questState,textOption);

	switch(missionType)
	{
		case MISSION_LEVEL:
			ms.addReplacement(m13489val);
			break;
		case MISSION_PRIMARY_STAT:
			{
				MetaString loot;
				for (int i = 0; i < 4; ++i)
				{
					if (m2stats[i])
					{
						loot << "%d %s";
						loot.addReplacement(m2stats[i]);
						loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
					}
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_KILL_HERO:
			ms.addReplacement(heroName);
			break;
		case MISSION_KILL_CREATURE:
			ms.addReplacement(stackToKill);
			break;
		case MISSION_ART:
			{
				MetaString loot;
				for(const auto & elem : m5arts)
				{
					loot << "%s";
					loot.addReplacement(MetaString::ART_NAMES, elem);
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_ARMY:
			{
				MetaString loot;
				for(const auto & elem : m6creatures)
				{
					loot << "%s";
					loot.addReplacement(elem);
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_RESOURCES:
			{
				MetaString loot;
				for (int i = 0; i < 7; ++i)
				{
					if (m7resources[i])
					{
						loot << "%d %s";
						loot.addReplacement(m7resources[i]);
						loot.addReplacement(MetaString::RES_NAMES, i);
					}
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_HERO:
			ms.addReplacement(VLC->heroh->objects[m13489val]->getNameTranslated());
			break;
		case MISSION_PLAYER:
			ms.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
		default:
			break;
	}
}

void CQuest::getCompletionText(MetaString &iwText, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h) const
{
	iwText << completedText;
	switch(missionType)
	{
		case CQuest::MISSION_LEVEL:
			if (!isCustomComplete)
				iwText.addReplacement(m13489val);
			break;
		case CQuest::MISSION_PRIMARY_STAT:
			if (vstd::contains (completedText,'%')) //there's one case when there's nothing to replace
			{
				MetaString loot;
				for (int i = 0; i < 4; ++i)
				{
					if (m2stats[i])
					{
						loot << "%d %s";
						loot.addReplacement(m2stats[i]);
						loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
					}
				}
				if (!isCustomComplete)
					iwText.addReplacement(loot.buildList());
			}
			break;
		case CQuest::MISSION_ART:
		{
			MetaString loot;
			for(const auto & elem : m5arts)
			{
				loot << "%s";
				loot.addReplacement(MetaString::ART_NAMES, elem);
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case CQuest::MISSION_ARMY:
		{
			MetaString loot;
			for(const auto & elem : m6creatures)
			{
				loot << "%s";
				loot.addReplacement(elem);
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case CQuest::MISSION_RESOURCES:
		{
			MetaString loot;
			for (int i = 0; i < 7; ++i)
			{
				if (m7resources[i])
				{
					loot << "%d %s";
					loot.addReplacement(m7resources[i]);
					loot.addReplacement(MetaString::RES_NAMES, i);
				}
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_KILL_HERO:
		case MISSION_KILL_CREATURE:
			if (!isCustomComplete)
				addReplacements(iwText, completedText);
			break;
		case MISSION_HERO:
			if (!isCustomComplete)
				iwText.addReplacement(VLC->heroh->objects[m13489val]->getNameTranslated());
			break;
		case MISSION_PLAYER:
			if (!isCustomComplete)
				iwText.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
	}
}

void CQuest::addArtifactID(const ArtifactID & id)
{
	m5arts.push_back(id);
	++artifactsRequirements[id];
}

void CQuest::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	auto q = handler.enterStruct(fieldName);

	handler.serializeString("firstVisitText", firstVisitText);
	handler.serializeString("nextVisitText", nextVisitText);
	handler.serializeString("completedText", completedText);

	if(!handler.saving)
	{
		isCustomFirst = !firstVisitText.empty();
		isCustomNext = !nextVisitText.empty();
		isCustomComplete = !completedText.empty();
	}

	static const std::vector<std::string> MISSION_TYPE_JSON =
	{
		"None", "Level", "PrimaryStat", "KillHero", "KillCreature", "Artifact", "Army", "Resources", "Hero", "Player"
	};

	handler.serializeEnum("missionType", missionType, Emission::MISSION_NONE, MISSION_TYPE_JSON);
	handler.serializeInt("timeLimit", lastDay, -1);

	switch (missionType)
	{
	case MISSION_NONE:
		break;
	case MISSION_LEVEL:
		handler.serializeInt("heroLevel", m13489val, -1);
		break;
	case MISSION_PRIMARY_STAT:
		{
			auto primarySkills = handler.enterStruct("primarySkills");
			if(!handler.saving)
				m2stats.resize(GameConstants::PRIMARY_SKILLS);

			for(int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
				handler.serializeInt(PrimarySkill::names[i], m2stats[i], 0);
		}
		break;
	case MISSION_KILL_HERO:
	case MISSION_KILL_CREATURE:
		handler.serializeInstance<ui32>("killTarget", m13489val, static_cast<ui32>(-1));
		break;
	case MISSION_ART:
		//todo: ban artifacts
		handler.serializeIdArray<ArtifactID>("artifacts", m5arts);
		break;
	case MISSION_ARMY:
        {
			auto a = handler.enterArray("creatures");
			a.serializeStruct(m6creatures);
        }
		break;
	case MISSION_RESOURCES:
        {
        	auto r = handler.enterStruct("resources");

        	if(!handler.saving)
				m7resources.resize(GameConstants::RESOURCE_QUANTITY-1);

			for(size_t idx = 0; idx < (GameConstants::RESOURCE_QUANTITY - 1); idx++)
			{
				handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], m7resources[idx], 0);
			}
        }
		break;
	case MISSION_HERO:
		handler.serializeId<ui32, ui32, HeroTypeID>("hero", m13489val, 0);
		break;
	case MISSION_PLAYER:
		handler.serializeEnum("player",  m13489val, PlayerColor::CANNOT_DETERMINE.getNum(), GameConstants::PLAYER_COLOR_NAMES);
		break;
	default:
		logGlobal->error("Invalid quest mission type");
		break;
	}

}

void CGSeerHut::setObjToKill()
{
	if(quest->missionType == CQuest::MISSION_KILL_CREATURE)
	{
		quest->stackToKill = getCreatureToKill(false)->getStack(SlotID(0)); //FIXME: stacks tend to disappear (desync?) on server :?
		assert(quest->stackToKill.type);
		quest->stackToKill.count = 0; //no count in info window
		quest->stackDirection = checkDirection();
	}
	else if(quest->missionType == CQuest::MISSION_KILL_HERO)
	{
		quest->heroName = getHeroToKill(false)->getNameTranslated();
		quest->heroPortrait = getHeroToKill(false)->portrait;
	}
}

void CGSeerHut::init(CRandomGenerator & rand)
{
	auto names = VLC->generaltexth->findStringsWithPrefix("core.seerhut.names");

	seerName = *RandomGeneratorUtil::nextItem(names, rand);
	quest->textOption = rand.nextInt(2);
	quest->completedOption = rand.nextInt(1, 3);
}

void CGSeerHut::initObj(CRandomGenerator & rand)
{
	init(rand);

	quest->progress = CQuest::NOT_ACTIVE;
	if(quest->missionType)
	{
		std::string questName  = quest->missionName(quest->missionType);

		if(!quest->isCustomFirst)
			quest->firstVisitText = VLC->generaltexth->translate("core.seerhut.quest." + questName + "." + quest->missionState(0), quest->textOption);
		if(!quest->isCustomNext)
			quest->nextVisitText = VLC->generaltexth->translate("core.seerhut.quest." + questName + "." + quest->missionState(1), quest->textOption);
		if(!quest->isCustomComplete)
			quest->completedText = VLC->generaltexth->translate("core.seerhut.quest." + questName + "." + quest->missionState(2), quest->textOption);
	}
	else
	{
		quest->progress = CQuest::COMPLETE;
		quest->firstVisitText = VLC->generaltexth->seerEmpty[quest->completedOption];
	}
}

void CGSeerHut::getRolloverText(MetaString &text, bool onHover) const
{
	quest->getRolloverText (text, onHover);//TODO: simplify?
	if(!onHover)
		text.addReplacement(seerName);
}

std::string CGSeerHut::getHoverText(PlayerColor player) const
{
	std::string hoverName = getObjectName();
	if(ID == Obj::SEER_HUT && quest->progress != CQuest::NOT_ACTIVE)
	{
		hoverName = VLC->generaltexth->allTexts[347];
		boost::algorithm::replace_first(hoverName, "%s", seerName);
	}

	if(quest->progress & quest->missionType) //rollover when the quest is active
	{
		MetaString ms;
		getRolloverText (ms, true);
		hoverName += ms.toString();
	}
	return hoverName;
}

void CQuest::addReplacements(MetaString &out, const std::string &base) const
{
	switch(missionType)
	{
	case MISSION_KILL_CREATURE:
		out.addReplacement(stackToKill);
		if (std::count(base.begin(), base.end(), '%') == 2) //say where is placed monster
		{
			out.addReplacement(VLC->generaltexth->arraytxt[147+stackDirection]);
		}
		break;
	case MISSION_KILL_HERO:
		out.addReplacement(heroName);
		break;
	}
}

bool IQuestObject::checkQuest(const CGHeroInstance* h) const
{
	return quest->checkQuest(h);
}

void IQuestObject::getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h) const
{
	quest->getVisitText (text,components, isCustom, FirstVisit, h);
}

void IQuestObject::afterAddToMapCommon(CMap * map) const
{
	map->addNewQuestInstance(quest);
}

void CGSeerHut::getCompletionText(MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h) const
{
	quest->getCompletionText (text, components, isCustom, h);
	switch(rewardType)
	{
	case EXPERIENCE:
		components.emplace_back(Component::EXPERIENCE, 0, static_cast<si32>(h->calculateXp(rVal)), 0);
		break;
	case MANA_POINTS:
		components.emplace_back(Component::PRIM_SKILL, 5, rVal, 0);
		break;
	case MORALE_BONUS:
		components.emplace_back(Component::MORALE, 0, rVal, 0);
		break;
	case LUCK_BONUS:
		components.emplace_back(Component::LUCK, 0, rVal, 0);
		break;
	case RESOURCES:
		components.emplace_back(Component::RESOURCE, rID, rVal, 0);
		break;
	case PRIMARY_SKILL:
		components.emplace_back(Component::PRIM_SKILL, rID, rVal, 0);
		break;
	case SECONDARY_SKILL:
		components.emplace_back(Component::SEC_SKILL, rID, rVal, 0);
		break;
	case ARTIFACT:
		components.emplace_back(Component::ARTIFACT, rID, 0, 0);
		break;
	case SPELL:
		components.emplace_back(Component::SPELL, rID, 0, 0);
		break;
	case CREATURE:
		components.emplace_back(Component::CREATURE, rID, rVal, 0);
		break;
	}
}

void CGSeerHut::setPropertyDer (ui8 what, ui32 val)
{
	switch(what)
	{
		case 10:
			quest->progress = static_cast<CQuest::Eprogress>(val);
			break;
	}
}

void CGSeerHut::newTurn(CRandomGenerator & rand) const
{
	if(quest->lastDay >= 0 && quest->lastDay <= cb->getDate() - 1) //time is up
	{
		cb->setObjProperty (id, CGSeerHut::OBJPROP_VISITED, CQuest::COMPLETE);
	}
}

void CGSeerHut::onHeroVisit(const CGHeroInstance * h) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if(quest->progress < CQuest::COMPLETE)
	{
		bool firstVisit = !quest->progress;
		bool failRequirements = !checkQuest(h);
		bool isCustom = false;

		if(firstVisit)
		{
			isCustom = quest->isCustomFirst;
			cb->setObjProperty(id, CGSeerHut::OBJPROP_VISITED, CQuest::IN_PROGRESS);

			AddQuest aq;
			aq.quest = QuestInfo (quest, this, visitablePos());
			aq.player = h->tempOwner;
			cb->sendAndApply(&aq); //TODO: merge with setObjProperty?
		}
		else if(failRequirements)
		{
			isCustom = quest->isCustomNext;
		}

		if(firstVisit || failRequirements)
		{
			getVisitText (iw.text, iw.components, isCustom, firstVisit, h);

			cb->showInfoDialog(&iw);
		}
		if(!failRequirements) // propose completion, also on first visit
		{
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();

			getCompletionText (bd.text, bd.components, isCustom, h);

			cb->showBlockingDialog (&bd);
			return;
		}
	}
	else
	{
		iw.text << VLC->generaltexth->seerEmpty[quest->completedOption];
		if (ID == Obj::SEER_HUT)
			iw.text.addReplacement(seerName);
		cb->showInfoDialog(&iw);
	}
}

int CGSeerHut::checkDirection() const
{
	int3 cord = getCreatureToKill()->pos;
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

void CGSeerHut::finishQuest(const CGHeroInstance * h, ui32 accept) const
{
	if (accept)
	{
		switch (quest->missionType)
		{
			case CQuest::MISSION_ART:
				for (auto & elem : quest->m5arts)
				{
					if(!h->hasArt(elem))
					{
						// first we need to disassemble this backpack artifact
						const auto * assembly = h->getAssemblyByConstituent(elem);
						assert(assembly);
						for(const auto & ci : assembly->constituentsInfo)
						{
							cb->giveHeroNewArtifact(h, ci.art->artType, ArtifactPosition::PRE_FIRST);
						}
						// remove the assembly
						cb->removeArtifact(ArtifactLocation(h, h->getArtPos(assembly)));
					}
					cb->removeArtifact(ArtifactLocation(h, h->getArtPos(elem, false)));
				}
				break;
			case CQuest::MISSION_ARMY:
					cb->takeCreatures(h->id, quest->m6creatures);
				break;
			case CQuest::MISSION_RESOURCES:
				for (int i = 0; i < 7; ++i)
				{
					cb->giveResource(h->getOwner(), static_cast<Res::ERes>(i), -static_cast<int>(quest->m7resources[i]));
				}
				break;
			default:
				break;
		}
		cb->setObjProperty (id, CGSeerHut::OBJPROP_VISITED, CQuest::COMPLETE); //mission complete
		completeQuest(h); //make sure to remove QuestGuard at the very end
	}
}

void CGSeerHut::completeQuest (const CGHeroInstance * h) const //reward
{
	switch (rewardType)
	{
		case EXPERIENCE:
		{
			TExpType expVal = h->calculateXp(rVal);
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, expVal, false);
			break;
		}
		case MANA_POINTS:
		{
			cb->setManaPoints(h->id, h->mana+rVal);
			break;
		}
		case MORALE_BONUS: case LUCK_BONUS:
		{
			Bonus hb(Bonus::ONE_WEEK, (rewardType == 3 ? Bonus::MORALE : Bonus::LUCK),
				Bonus::OBJECT, rVal, h->id.getNum(), "", -1);
			GiveBonus gb;
			gb.id = h->id.getNum();
			gb.bonus = hb;
			cb->giveHeroBonus(&gb);
		}
			break;
		case RESOURCES:
			cb->giveResource(h->getOwner(), static_cast<Res::ERes>(rID), rVal);
			break;
		case PRIMARY_SKILL:
			cb->changePrimSkill(h, static_cast<PrimarySkill::PrimarySkill>(rID), rVal, false);
			break;
		case SECONDARY_SKILL:
			cb->changeSecSkill(h, SecondarySkill(rID), rVal, false);
			break;
		case ARTIFACT:
			cb->giveHeroNewArtifact(h, VLC->arth->objects[rID],ArtifactPosition::FIRST_AVAILABLE);
			break;
		case SPELL:
		{
			std::set<SpellID> spell;
			spell.insert (SpellID(rID));
			cb->changeSpells(h, true, spell);
		}
			break;
		case CREATURE:
			{
				CCreatureSet creatures;
				creatures.setCreature(SlotID(0), CreatureID(rID), rVal);
				cb->giveCreatures(this, h, creatures, false);
			}
			break;
		default:
			break;
	}
}

const CGHeroInstance * CGSeerHut::getHeroToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObjByQuestIdentifier(quest->m13489val);
	if(allowNull && !o)
		return nullptr;
	assert(o && (o->ID == Obj::HERO  ||  o->ID == Obj::PRISON));
	return dynamic_cast<const CGHeroInstance *>(o);
}

const CGCreature * CGSeerHut::getCreatureToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObjByQuestIdentifier(quest->m13489val);
	if(allowNull && !o)
		return nullptr;
	assert(o && o->ID == Obj::MONSTER);
	return dynamic_cast<const CGCreature *>(o);
}

void CGSeerHut::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	finishQuest(hero, answer);
}

void CGSeerHut::afterAddToMap(CMap* map)
{
	IQuestObject::afterAddToMapCommon(map);
}

void CGSeerHut::serializeJsonOptions(JsonSerializeFormat & handler)
{
	static const std::map<ERewardType, std::string> REWARD_MAP =
	{
		{NOTHING,		""},
		{EXPERIENCE,	"experience"},
		{MANA_POINTS,	"mana"},
		{MORALE_BONUS,	"morale"},
		{LUCK_BONUS,	"luck"},
		{RESOURCES,		"resource"},
		{PRIMARY_SKILL,	"primarySkill"},
		{SECONDARY_SKILL,"secondarySkill"},
		{ARTIFACT,		"artifact"},
		{SPELL,			"spell"},
		{CREATURE,		"creature"}
	};

	static const std::map<std::string, ERewardType> REWARD_RMAP =
	{
		{"experience",    EXPERIENCE},
		{"mana",          MANA_POINTS},
		{"morale",        MORALE_BONUS},
		{"luck",          LUCK_BONUS},
		{"resource",      RESOURCES},
		{"primarySkill",  PRIMARY_SKILL},
		{"secondarySkill",SECONDARY_SKILL},
		{"artifact",      ARTIFACT},
		{"spell",         SPELL},
		{"creature",      CREATURE}
	};

	//quest and reward
	quest->serializeJson(handler, "quest");

	//only one reward is supported
	//todo: full reward format support after CRewardInfo integration

	auto s = handler.enterStruct("reward");
	std::string fullIdentifier;
	std::string metaTypeName;
	std::string scope;
	std::string identifier;

	if(handler.saving)
	{
		si32 amount = rVal;

		metaTypeName = REWARD_MAP.at(rewardType);
		switch (rewardType)
		{
		case NOTHING:
			break;
		case EXPERIENCE:
		case MANA_POINTS:
		case MORALE_BONUS:
		case LUCK_BONUS:
			identifier.clear();
			break;
		case RESOURCES:
			identifier = GameConstants::RESOURCE_NAMES[rID];
			break;
		case PRIMARY_SKILL:
			identifier = PrimarySkill::names[rID];
			break;
		case SECONDARY_SKILL:
			identifier = CSkillHandler::encodeSkill(rID);
			break;
		case ARTIFACT:
			identifier = ArtifactID(rID).toArtifact(VLC->artifacts())->getJsonKey();
			amount = 1;
			break;
		case SPELL:
			identifier = SpellID(rID).toSpell(VLC->spells())->getJsonKey();
			amount = 1;
			break;
		case CREATURE:
			identifier = CreatureID(rID).toCreature(VLC->creatures())->getJsonKey();
			break;
		default:
			assert(false);
			break;
		}
		if(rewardType != NOTHING)
		{
			fullIdentifier = CModHandler::makeFullIdentifier(scope, metaTypeName, identifier);
			handler.serializeInt(fullIdentifier, amount);
		}
	}
	else
	{
		rewardType = NOTHING;

		const JsonNode & rewardsJson = handler.getCurrent();

		fullIdentifier.clear();

		if(rewardsJson.Struct().empty())
			return;
		else
		{
			auto iter = rewardsJson.Struct().begin();
			fullIdentifier = iter->first;
		}

		CModHandler::parseIdentifier(fullIdentifier, scope, metaTypeName, identifier);

		auto it = REWARD_RMAP.find(metaTypeName);

		if(it == REWARD_RMAP.end())
		{
			logGlobal->error("%s: invalid metatype in reward item %s", instanceName, fullIdentifier);
			return;
		}
		else
		{
			rewardType = it->second;
		}

		bool doRequest = false;

		switch (rewardType)
		{
		case NOTHING:
			return;
		case EXPERIENCE:
		case MANA_POINTS:
		case MORALE_BONUS:
		case LUCK_BONUS:
			break;
		case PRIMARY_SKILL:
			doRequest = true;
			break;
		case RESOURCES:
		case SECONDARY_SKILL:
		case ARTIFACT:
		case SPELL:
		case CREATURE:
			doRequest = true;
			break;
		default:
			assert(false);
			break;
		}

		if(doRequest)
		{
			auto rawId = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), fullIdentifier, false);

			if(rawId)
			{
				rID = rawId.get();
			}
			else
			{
				rewardType = NOTHING;//fallback in case of error
				return;
			}
		}
		handler.serializeInt(fullIdentifier, rVal);
	}
}

void CGQuestGuard::init(CRandomGenerator & rand)
{
	blockVisit = true;
	quest->textOption = rand.nextInt(3, 5);
	quest->completedOption = rand.nextInt(4, 5);
}

void CGQuestGuard::completeQuest(const CGHeroInstance *h) const
{
	cb->removeObject(this);
}

void CGQuestGuard::serializeJsonOptions(JsonSerializeFormat & handler)
{
	//quest only, do not call base class
	quest->serializeJson(handler, "quest");
}

void CGKeys::reset()
{
	playerKeyMap.clear();
}

void CGKeys::setPropertyDer (ui8 what, ui32 val) //101-108 - enable key for player 1-8
{
	if (what >= 101 && what <= (100 + PlayerColor::PLAYER_LIMIT_I))
	{
		PlayerColor player(what-101);
		playerKeyMap[player].insert(static_cast<ui8>(val));
	}
	else
		logGlobal->error("Unexpected properties requested to set: what=%d, val=%d", static_cast<int>(what), val);
}

bool CGKeys::wasMyColorVisited(const PlayerColor & player) const
{
	return playerKeyMap.count(player) && vstd::contains(playerKeyMap[player], subID);
}

std::string CGKeys::getHoverText(PlayerColor player) const
{
	return getObjectName() + "\n" + visitedTxt(wasMyColorVisited(player));
}

std::string CGKeys::getObjectName() const
{
	return VLC->generaltexth->tentColors[subID] + " " + CGObjectInstance::getObjectName();
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
		cb->setObjProperty(id, h->tempOwner.getNum()+101, subID);
		txt_id=19;
	}
	else
		txt_id=20;
	showInfoDialog(h, txt_id);
}

void CGBorderGuard::initObj(CRandomGenerator & rand)
{
	//ui32 m13489val = subID; //store color as quest info
	blockVisit = true;
}

void CGBorderGuard::getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h) const
{
	text << std::pair<ui8,ui32>(11,18);
}

void CGBorderGuard::getRolloverText (MetaString &text, bool onHover) const
{
	if (!onHover)
		text << VLC->generaltexth->tentColors[subID] << " " << VLC->objtypeh->getObjectName(Obj::KEYMASTER, subID);
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
		bd.text.addTxt (MetaString::ADVOB_TXT, 17);
		cb->showBlockingDialog (&bd);
	}
	else
	{
		showInfoDialog(h, 18);

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
		cb->removeObject(this);
}

void CGBorderGuard::afterAddToMap(CMap * map)
{
	IQuestObject::afterAddToMapCommon(map);
}

void CGBorderGate::onHeroVisit(const CGHeroInstance * h) const //TODO: passability
{
	if (!wasMyColorVisited (h->getOwner()) )
	{
		showInfoDialog(h,18,0);

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
