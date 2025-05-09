/*
 * CGPandoraBox.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGPandoraBox.h"

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "../CSoundBase.h"

#include "../CSkillHandler.h"
#include "../StartInfo.h"
#include "../IGameCallback.h"
#include "../constants/StringConstants.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

void CGPandoraBox::init()
{
	blockVisit = true;
	configuration.info.emplace_back();
	configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	
	for(auto & i : configuration.info)
	{
		i.reward.removeObject = true;
		if(!message.empty() && i.message.empty())
			i.message = message;
	}
}

void CGPandoraBox::initObj(vstd::RNG & rand)
{
	init();
	
	CRewardableObject::initObj(rand);
}

void CGPandoraBox::grantRewardWithMessage(const CGHeroInstance * h, int index, bool markAsVisit) const
{
	auto vi = configuration.info.at(index);
	if(!vi.message.empty())
	{
		CRewardableObject::grantRewardWithMessage(h, index, markAsVisit);
		return;
	}
	
	//split reward message for pandora box
	auto setText = [](bool cond, int posId, int negId, const CGHeroInstance * h)
	{
		MetaString text;
		text.appendLocalString(EMetaText::ADVOB_TXT, cond ? posId : negId);
		text.replaceRawString(h->getNameTranslated());
		return text;
	};
	
	auto sendInfoWindow = [&](const MetaString & text, const Rewardable::Reward & reward)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text = text;
		reward.loadComponents(iw.components, h);
		iw.type = EInfoWindowMode::MODAL;
		if(!iw.components.empty())
			cb->showInfoDialog(&iw);
	};

	Rewardable::Reward temp;
	temp.spells = vi.reward.spells;
	temp.heroExperience = vi.reward.heroExperience;
	temp.heroLevel = vi.reward.heroLevel;
	temp.primary = vi.reward.primary;
	temp.secondary = vi.reward.secondary;
	temp.heroBonuses = vi.reward.heroBonuses;
	temp.manaDiff = vi.reward.manaDiff;
	temp.manaPercentage = vi.reward.manaPercentage;
	
	MetaString txt;
	if(!vi.reward.spells.empty())
		txt = setText(temp.spells.size() == 1, 184, 188, h);
	
	if(vi.reward.heroExperience || vi.reward.heroLevel || !vi.reward.secondary.empty())
		txt = setText(true, 175, 175, h);
	
	for(int i : vi.reward.primary)
	{
		if(i)
		{
			txt = setText(true, 175, 175, h);
			break;
		}
	}
	
	if(vi.reward.manaDiff || vi.reward.manaPercentage >= 0)
		txt = setText(temp.manaDiff > 0, 177, 176, h);
	
	for(auto b : vi.reward.heroBonuses)
	{
		if(b.val && b.type == BonusType::MORALE)
			txt = setText(b.val > 0, 179, 178, h);
		if(b.val && b.type == BonusType::LUCK)
			txt = setText(b.val > 0, 181, 180, h);
	}
	sendInfoWindow(txt, temp);
	
	//resource message
	temp = Rewardable::Reward{};
	temp.resources = vi.reward.resources;
	sendInfoWindow(setText(vi.reward.resources.marketValue() > 0, 183, 182, h), temp);
	
	//artifacts message
	temp = Rewardable::Reward{};
	temp.grantedArtifacts = vi.reward.grantedArtifacts;
	sendInfoWindow(setText(true, 183, 183, h), temp);
	
	//creatures message
	temp = Rewardable::Reward{};
	temp.creatures = vi.reward.creatures;
	txt.clear();
	if(!vi.reward.creatures.empty())
	{
		MetaString loot;
		for(auto c : vi.reward.creatures)
		{
			loot.appendRawString("%s");
			loot.replaceName(c);
		}
		
		if(vi.reward.creatures.size() == 1 && vi.reward.creatures[0].getCount() == 1)
			txt.appendLocalString(EMetaText::ADVOB_TXT, 185);
		else
			txt.appendLocalString(EMetaText::ADVOB_TXT, 186);
		
		txt.replaceRawString(loot.buildList());
		txt.replaceRawString(h->getNameTranslated());
	}
	sendInfoWindow(txt, temp);
	
	//everything else
	temp = vi.reward;
	temp.heroExperience = 0;
	temp.heroLevel = 0;
	temp.secondary.clear();
	temp.primary.clear();
	temp.resources.amin(0);
	temp.resources.amax(0);
	temp.manaDiff = 0;
	temp.manaPercentage = -1;
	temp.spells.clear();
	temp.creatures.clear();
	temp.heroBonuses.clear();
	temp.grantedArtifacts.clear();
	sendInfoWindow(setText(true, 175, 175, h), temp);
	
	// grant reward afterwards. Note that it may remove object
	if(markAsVisit)
		markAsVisited(h);
	grantReward(index, h);
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
	BlockingDialog bd (true, false);
	bd.player = h->getOwner();
	bd.text.appendLocalString(EMetaText::ADVOB_TXT, 14);
	cb->showBlockingDialog(this, &bd);
}

void CGPandoraBox::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == BattleSide::ATTACKER)
	{
		CRewardableObject::onHeroVisit(hero);
	}
}

void CGPandoraBox::blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const
{
	if(answer)
	{
		if(stacksCount() > 0) //if pandora's box is protected by army
		{
			hero->showInfoDialog(16, 0, EInfoWindowMode::MODAL);
			cb->startBattle(hero, this); //grants things after battle
		}
		else if(getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT).empty())
		{
			hero->showInfoDialog(15);
			cb->removeObject(this, hero->getOwner());
		}
		else //if it gives something without battle
		{
			CRewardableObject::onHeroVisit(hero);
		}
	}
}

void CGPandoraBox::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CRewardableObject::serializeJsonOptions(handler);
	
	handler.serializeStruct("guardMessage", message);
	
	if(!handler.saving)
	{
		//backward compatibility for VCMI maps that use old Pandora Box format
		if(!handler.getCurrent()["guards"].Vector().empty())
			CCreatureSet::serializeJson(handler, "guards", 7);
		
		bool hasSomething = false;
		Rewardable::VisitInfo vinfo;
		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
				
		handler.serializeInt("experience", vinfo.reward.heroExperience, 0);
		handler.serializeInt("mana", vinfo.reward.manaDiff, 0);
		
		int val = 0;
		handler.serializeInt("morale", val, 0);
		if(val)
			vinfo.reward.heroBonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id));
		
		handler.serializeInt("luck", val, 0);
		if(val)
			vinfo.reward.heroBonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(id));
		
		vinfo.reward.resources.serializeJson(handler, "resources");
		{
			auto s = handler.enterStruct("primarySkills");
			for(int idx = 0; idx < vinfo.reward.primary.size(); idx ++)
			{
				handler.serializeInt(NPrimarySkill::names[idx], vinfo.reward.primary[idx], 0);
				if(vinfo.reward.primary[idx])
					hasSomething = true;
			}
		}
		
		handler.serializeIdArray("artifacts", vinfo.reward.grantedArtifacts);
		handler.serializeIdArray("spells", vinfo.reward.spells);
		handler.enterArray("creatures").serializeStruct(vinfo.reward.creatures);
		
		{
			auto s = handler.enterStruct("secondarySkills");
			for(const auto & p : handler.getCurrent().Struct())
			{
				const std::string skillName = p.first;
				const std::string levelId = p.second.String();
				
				const int rawId = SecondarySkill::decode(skillName);
				if(rawId < 0)
				{
					logGlobal->error("Invalid secondary skill %s", skillName);
					continue;
				}
				
				const int level = vstd::find_pos(NSecondarySkill::levels, levelId);
				if(level < 0)
				{
					logGlobal->error("Invalid secondary skill level %s", levelId);
					continue;
				}
				
				vinfo.reward.secondary[rawId] = level;
			}
		}
		
		hasSomething = hasSomething
		|| vinfo.reward.heroExperience
		|| vinfo.reward.manaDiff
		|| vinfo.reward.resources.nonZero()
		|| !vinfo.reward.grantedArtifacts.empty()
		|| !vinfo.reward.heroBonuses.empty()
		|| !vinfo.reward.creatures.empty()
		|| !vinfo.reward.secondary.empty();
		
		if(hasSomething)
			configuration.info.push_back(vinfo);
	}
}

void CGEvent::init()
{
	blockVisit = false;
	configuration.infoWindowType = EInfoWindowMode::MODAL;
	
	for(auto & i : configuration.info)
	{
		i.reward.removeObject = removeAfterVisit;
		if(!message.empty() && i.message.empty())
			i.message = message;
	}
}

void CGEvent::grantRewardWithMessage(const CGHeroInstance * contextHero, int rewardIndex, bool markAsVisit) const
{
	CRewardableObject::grantRewardWithMessage(contextHero, rewardIndex, markAsVisit);
}

void CGEvent::onHeroVisit( const CGHeroInstance * h ) const
{
	if(availableFor.count(h->tempOwner) == 0)
		return;

	if(cb->getPlayerSettings(h->tempOwner)->isControlledByHuman())
	{
		if(humanActivate)
			activated(h);
	}
	else if(computerActivate)
		activated(h);
}

void CGEvent::activated( const CGHeroInstance * h ) const
{
	if(stacksCount() > 0)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		if(!message.empty())
			iw.text = message;
		else
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, 16);
		cb->showInfoDialog(&iw);
		cb->startBattle(h, this);
	}
	else
	{
		CRewardableObject::onHeroVisit(h);
	}
}

void CGEvent::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGPandoraBox::serializeJsonOptions(handler);

	handler.serializeBool("aIActivable", computerActivate, false);
	handler.serializeBool("humanActivable", humanActivate, true);
	handler.serializeBool("removeAfterVisit", removeAfterVisit, false);
	handler.serializeIdArray("availableFor", availableFor);
}

VCMI_LIB_NAMESPACE_END
