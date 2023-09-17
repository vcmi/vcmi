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

#include "../NetPacks.h"
#include "../CSoundBase.h"

#include "../CSkillHandler.h"
#include "../StartInfo.h"
#include "../IGameCallback.h"
#include "../constants/StringConstants.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

void CGPandoraBox::init()
{
	blockVisit = true;
	configuration.selectMode = Rewardable::SELECT_ALL;
	
	for(auto & i : configuration.info)
		i.reward.removeObject = true;
}

void CGPandoraBox::initObj(CRandomGenerator & rand)
{
	init();
	
	CRewardableObject::initObj(rand);
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
	auto setText = [](MetaString & text, int tId, const CGHeroInstance * h)
	{
		text.appendLocalString(EMetaText::ADVOB_TXT, tId);
		text.replaceRawString(h->getNameTranslated());
	};
	
	for(auto i : getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT))
	{
		MetaString txt;
		auto & r = configuration.info[i];
		
		if(r.reward.spells.size() == 1)
			setText(txt, 184, h);
		else if(!r.reward.spells.empty())
			setText(txt, 188, h);
		
		if(r.reward.heroExperience || !r.reward.secondary.empty())
			setText(txt, 175, h);
		
		for(int ps : r.reward.primary)
		{
			if(ps)
			{
				setText(txt, 175, h);
				break;
			}
		}
		
		if(r.reward.manaDiff < 0)
			setText(txt, 176, h);
		if(r.reward.manaDiff > 0)
			setText(txt, 177, h);
		
		for(auto b : r.reward.bonuses)
		{
			if(b.type == BonusType::MORALE)
			{
				if(b.val < 0)
					setText(txt, 178, h);
				if(b.val > 0)
					setText(txt, 179, h);
			}
			if(b.type == BonusType::LUCK)
			{
				if(b.val < 0)
					setText(txt, 180, h);
				if(b.val > 0)
					setText(txt, 181, h);
			}
		}
		
		for(auto res : r.reward.resources)
		{
			if(res < 0)
				setText(txt, 182, h);
			if(res > 0)
				setText(txt, 183, h);
		}
		
		if(!r.reward.artifacts.empty())
			setText(txt, 183, h);
		
		if(!r.reward.creatures.empty())
		{
			MetaString loot;
			for(auto c : r.reward.creatures)
			{
				loot.appendRawString("%s");
				loot.replaceCreatureName(c);
			}
			
			if(r.reward.creatures.size() == 1 && r.reward.creatures[0].count == 1)
				txt.appendLocalString(EMetaText::ADVOB_TXT, 185);
			else
				txt.appendLocalString(EMetaText::ADVOB_TXT, 186);
			
			txt.replaceRawString(loot.buildList());
			txt.replaceRawString(h->getNameTranslated());
		}
		
		if(r.message.empty())
			const_cast<MetaString&>(r.message) = txt;
	}
	
	BlockingDialog bd (true, false);
	bd.player = h->getOwner();
	bd.text.appendLocalString(EMetaText::ADVOB_TXT, 14);
	cb->showBlockingDialog(&bd);
}

void CGPandoraBox::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0)
	{
		CRewardableObject::onHeroVisit(hero);
	}
}

void CGPandoraBox::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
	{
		if(stacksCount() > 0) //if pandora's box is protected by army
		{
			hero->showInfoDialog(16, 0, EInfoWindowMode::MODAL);
			cb->startBattleI(hero, this); //grants things after battle
		}
		else if(getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT).empty())
		{
			hero->showInfoDialog(15);
			cb->removeObject(this);
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
	handler.serializeString("guardMessage", message);
	
	if(!handler.saving)
	{
		//backward compatibility
		CCreatureSet::serializeJson(handler, "guards", 7);
		Rewardable::VisitInfo vinfo;
		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		
		auto addReward = [this, &vinfo](bool condition)
		{
			if(condition)
			{
				configuration.info.push_back(vinfo);
				vinfo = Rewardable::VisitInfo{};
				vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
			}
		};
				
		int val;
		handler.serializeInt("experience", vinfo.reward.heroExperience, 0);
		addReward(vinfo.reward.heroExperience);
		
		handler.serializeInt("mana", vinfo.reward.manaDiff, 0);
		addReward(vinfo.reward.manaDiff);
		
		handler.serializeInt("morale", val, 0);
		if(val)
			vinfo.reward.bonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT, val, id);
		addReward(val);
		
		handler.serializeInt("luck", val, 0);
		if(val)
			vinfo.reward.bonuses.emplace_back(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT, val, id);
		addReward(val);
		
		vinfo.reward.resources.serializeJson(handler, "resources");
		addReward(vinfo.reward.resources.nonZero());
		
		{
			bool updateReward = false;
			auto s = handler.enterStruct("primarySkills");
			for(int idx = 0; idx < vinfo.reward.primary.size(); idx ++)
			{
				handler.serializeInt(NPrimarySkill::names[idx], vinfo.reward.primary[idx], 0);
				updateReward |= bool(vinfo.reward.primary[idx]);
			}
			addReward(updateReward);
		}
		
		handler.serializeIdArray("artifacts", vinfo.reward.artifacts);
		addReward(!vinfo.reward.artifacts.empty());
		
		handler.serializeIdArray("spells", vinfo.reward.spells);
		addReward(!vinfo.reward.spells.empty());

		handler.enterArray("creatures").serializeStruct(vinfo.reward.creatures);
		addReward(!vinfo.reward.creatures.empty());
		
		{
			bool updateReward = false;
			auto s = handler.enterStruct("secondarySkills");
			for(const auto & p : handler.getCurrent().Struct())
			{
				const std::string skillName = p.first;
				const std::string levelId = p.second.String();
				
				const int rawId = CSkillHandler::decodeSkill(skillName);
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
				updateReward = true;
			}
			addReward(updateReward);
		}
	}
}

void CGEvent::init()
{
	blockVisit = false;
	configuration.selectMode = Rewardable::SELECT_ALL;
	
	for(auto & i : configuration.info)
		i.reward.removeObject = removeAfterVisit;
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
			iw.text.appendRawString(message);
		else
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, 16);
		cb->showInfoDialog(&iw);
		cb->startBattleI(h, this);
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
