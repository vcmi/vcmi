﻿/*
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

#include "../NetPacks.h"
#include "../CSoundBase.h"

#include "../spells/CSpellHandler.h"
#include "../StartInfo.h"
#include "../IGameCallback.h"
#include "../StringConstants.h"
#include "../serializer/JsonSerializeFormat.h"

///helpers
static void showInfoDialog(const PlayerColor playerID, const ui32 txtID, const ui16 soundID)
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = playerID;
	iw.text.addTxt(MetaString::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

CGPandoraBox::CGPandoraBox()
	: hasGuardians(false), gainedExp(0), manaDiff(0), moraleDiff(0), luckDiff(0)
{

}


void CGPandoraBox::initObj(CRandomGenerator & rand)
{
	blockVisit = (ID==Obj::PANDORAS_BOX); //block only if it's really pandora's box (events also derive from that class)
	hasGuardians = stacks.size();
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.text.addTxt (MetaString::ADVOB_TXT, 14);
		cb->showBlockingDialog (&bd);
}

void CGPandoraBox::giveContentsUpToExp(const CGHeroInstance *h) const
{
	afterSuccessfulVisit();

	InfoWindow iw;
	iw.player = h->getOwner();

	bool changesPrimSkill = false;
	for (auto & elem : primskills)
	{
		if(elem)
		{
			changesPrimSkill = true;
			break;
		}
	}

	std::vector<std::pair<SecondarySkill, ui8>> unpossessedAbilities; //ability + ability level
	int abilitiesRequiringSlot = 0;

	//filter out unnecessary secondary skills
	for (int i = 0; i < abilities.size(); i++)
	{
		int curLev = h->getSecSkillLevel(abilities[i]);
		bool abilityCanUseSlot = !curLev && ((h->secSkills.size() + abilitiesRequiringSlot) < GameConstants::SKILL_PER_HERO); //limit new abilities to number of slots

		if (abilityCanUseSlot)
			abilitiesRequiringSlot++;

		if ((curLev && curLev < abilityLevels[i]) || abilityCanUseSlot)
		{
			unpossessedAbilities.push_back({ abilities[i], abilityLevels[i] });
		}
	}

	if(gainedExp || changesPrimSkill || unpossessedAbilities.size())
	{
		TExpType expVal = h->calculateXp(gainedExp);
		//getText(iw,afterBattle,175,h); //wtf?
		iw.text.addTxt(MetaString::ADVOB_TXT, 175); //%s learns something
		iw.text.addReplacement(h->name);

		if(expVal)
			iw.components.push_back(Component(Component::EXPERIENCE,0,expVal,0));

		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				iw.components.push_back(Component(Component::PRIM_SKILL,i,primskills[i],0));

		for(auto abilityData : unpossessedAbilities)
			iw.components.push_back(Component(Component::SEC_SKILL, abilityData.first, abilityData.second, 0));

		cb->showInfoDialog(&iw);

		//give sec skills
		for (auto abilityData : unpossessedAbilities)
			cb->changeSecSkill(h, abilityData.first, abilityData.second, true);

		assert(h->secSkills.size() <= GameConstants::SKILL_PER_HERO);

		//give prim skills
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(i),primskills[i],false);

		assert(!cb->isVisitCoveredByAnotherQuery(this, h));

		//give exp
		if(expVal)
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, expVal, false);
	}
	//else { } //TODO:Create information that box was empty for now, and deliver to CGPandoraBox::giveContentsAfterExp or refactor

	if(!cb->isVisitCoveredByAnotherQuery(this, h))
		giveContentsAfterExp(h);
	//Otherwise continuation occurs via post-level-up callback.
}

void CGPandoraBox::giveContentsAfterExp(const CGHeroInstance *h) const
{
	bool hadGuardians = hasGuardians; //copy, because flag will be emptied after issuing first post-battle message

	std::string msg = message; //in case box is removed in the meantime
	InfoWindow iw;
	iw.player = h->getOwner();

	//TODO: reuse this code for Scholar skill
	if(spells.size())
	{
		std::set<SpellID> spellsToGive;

		auto i = spells.cbegin();
		while (i != spells.cend())
		{
			iw.components.clear();
			iw.text.clear();
			spellsToGive.clear();

			for (; i != spells.cend(); i++)
			{
				const CSpell * sp = (*i).toSpell();
				if(h->canLearnSpell(sp))
				{
					iw.components.push_back(Component(Component::SPELL, *i, 0, 0));
					spellsToGive.insert(*i);
				}
				if(spellsToGive.size() == 8) //display up to 8 spells at once
				{
					break;
				}
			}
			if (!spellsToGive.empty())
			{
				if (spellsToGive.size() > 1)
				{
					iw.text.addTxt(MetaString::ADVOB_TXT, 188); //%s learns spells
				}
				else
				{
					iw.text.addTxt(MetaString::ADVOB_TXT, 184); //%s learns a spell
				}
				iw.text.addReplacement(h->name);
				cb->changeSpells(h, true, spellsToGive);
				cb->showInfoDialog(&iw);
			}
		}
	}

	if(manaDiff)
	{
		getText(iw,hadGuardians,manaDiff,176,177,h);
		iw.components.push_back(Component(Component::PRIM_SKILL,5,manaDiff,0));
		cb->showInfoDialog(&iw);
		cb->setManaPoints(h->id, h->mana + manaDiff);
	}

	if(moraleDiff)
	{
		getText(iw,hadGuardians,moraleDiff,178,179,h);
		iw.components.push_back(Component(Component::MORALE,0,moraleDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,moraleDiff,id.getNum(),"");
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	if(luckDiff)
	{
		getText(iw,hadGuardians,luckDiff,180,181,h);
		iw.components.push_back(Component(Component::LUCK,0,luckDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,luckDiff,id.getNum(),"");
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	iw.components.clear();
	iw.text.clear();
	for(int i=0; i<resources.size(); i++)
	{
		if(resources[i] < 0)
			iw.components.push_back(Component(Component::RESOURCE,i,resources[i],0));
	}
	if(iw.components.size())
	{
		getText(iw,hadGuardians,182,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	iw.text.clear();
	for(int i=0; i<resources.size(); i++)
	{
		if(resources[i] > 0)
			iw.components.push_back(Component(Component::RESOURCE,i,resources[i],0));
	}
	if(iw.components.size())
	{
		getText(iw,hadGuardians,183,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	// 	getText(iw,afterBattle,183,h);
	iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure
	iw.text.addReplacement(h->name);
	for(auto & elem : artifacts)
	{
		iw.components.push_back(Component(Component::ARTIFACT,elem,0,0));
		if(iw.components.size() >= 14)
		{
			cb->showInfoDialog(&iw);
			iw.components.clear();
			iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure - once more?
			iw.text.addReplacement(h->name);
		}
	}
	if(iw.components.size())
	{
		cb->showInfoDialog(&iw);
	}

	cb->giveResources(h->getOwner(), resources);

	for(auto & elem : artifacts)
		cb->giveHeroNewArtifact(h, VLC->arth->artifacts[elem],ArtifactPosition::FIRST_AVAILABLE);

	iw.components.clear();
	iw.text.clear();

	if(creatures.stacksCount())
	{ //this part is taken straight from creature bank
		MetaString loot;
		for(auto & elem : creatures.Slots())
		{ //build list of joined creatures
			iw.components.push_back(Component(*elem.second));
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if(creatures.stacksCount() == 1 && creatures.Slots().begin()->second->count == 1)
			iw.text.addTxt(MetaString::ADVOB_TXT, 185);
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 186);

		iw.text.addReplacement(loot.buildList());
		iw.text.addReplacement(h->name);

		cb->showInfoDialog(&iw);
		cb->giveCreatures(this, h, creatures, false);
	}
	if(!hasGuardians && msg.size())
	{
		iw.text << msg;
		cb->showInfoDialog(&iw);
	}
}

void CGPandoraBox::getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const
{
	if(afterBattle || !message.size())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,text);//%s has lost treasure.
		iw.text.addReplacement(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
}

void CGPandoraBox::getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const
{
	iw.components.clear();
	iw.text.clear();
	if(afterBattle || !message.size())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,val < 0 ? negative : positive); //%s's luck takes a turn for the worse / %s's luck increases
		iw.text.addReplacement(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
}

void CGPandoraBox::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if(result.winner == 0)
	{
		giveContentsUpToExp(hero);
	}
}

void CGPandoraBox::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer)
	{
		if(stacksCount() > 0) //if pandora's box is protected by army
		{
			showInfoDialog(hero,16,0);
			cb->startBattleI(hero, this); //grants things after battle
		}
		else if(message.size() == 0 && resources.size() == 0
			&& primskills.size() == 0 && abilities.size() == 0
			&& abilityLevels.size() == 0 && artifacts.size() == 0
			&& spells.size() == 0 && creatures.stacksCount() > 0
			&& gainedExp == 0 && manaDiff == 0 && moraleDiff == 0 && luckDiff == 0) //if it gives nothing without battle
		{
			showInfoDialog(hero,15,0);
			cb->removeObject(this);
		}
		else //if it gives something without battle
		{
			giveContentsUpToExp(hero);
		}
	}
}

void CGPandoraBox::heroLevelUpDone(const CGHeroInstance *hero) const
{
	giveContentsAfterExp(hero);
}

void CGPandoraBox::afterSuccessfulVisit() const
{
	cb->removeAfterVisit(this);
}

CGEvent::CGEvent()
	: CGPandoraBox(), removeAfterVisit(false), availableFor(0), computerActivate(false), humanActivate(false)
{

}

void CGPandoraBox::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CCreatureSet::serializeJson(handler, "guards", 7);
	handler.serializeString("guardMessage", message);

	handler.serializeInt("experience", gainedExp, 0);
	handler.serializeInt("mana", manaDiff, 0);
	handler.serializeInt("morale", moraleDiff, 0);
	handler.serializeInt("luck", luckDiff, 0);

	resources.serializeJson(handler, "resources");

	{
		bool haveSkills = false;

		if(handler.saving)
		{
			for(int idx = 0; idx < primskills.size(); idx ++)
				if(primskills[idx] != 0)
					haveSkills = true;
		}
		else
		{
			primskills.resize(GameConstants::PRIMARY_SKILLS,0);
			haveSkills = true;
		}

		if(haveSkills)
		{
			auto s = handler.enterStruct("primarySkills");
			for(int idx = 0; idx < primskills.size(); idx ++)
				handler.serializeInt(PrimarySkill::names[idx], primskills[idx], 0);
		}
	}

	if(handler.saving)
	{
		if(!abilities.empty())
		{
			auto s = handler.enterStruct("secondarySkills");

			for(size_t idx = 0; idx < abilities.size(); idx++)
			{
				handler.serializeEnum(NSecondarySkill::names[abilities[idx]], abilityLevels[idx], NSecondarySkill::levels);
			}
		}
	}
	else
	{
		auto s = handler.enterStruct("secondarySkills");

		const JsonNode & skillMap = handler.getCurrent();

		abilities.clear();
		abilityLevels.clear();

		for(const auto & p : skillMap.Struct())
		{
			const std::string skillName = p.first;
			const std::string levelId = p.second.String();

			const int rawId = vstd::find_pos(NSecondarySkill::names, skillName);
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

			abilities.push_back(SecondarySkill(rawId));
			abilityLevels.push_back(level);
		}
	}


	handler.serializeIdArray("artifacts", artifacts);
	handler.serializeIdArray("spells", spells);

	creatures.serializeJson(handler, "creatures");
}

void CGEvent::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!(availableFor & (1 << h->tempOwner.getNum())))
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
		if(message.size())
			iw.text << message;
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 16);
		cb->showInfoDialog(&iw);
		cb->startBattleI(h, this);
	}
	else
	{
		giveContentsUpToExp(h);
	}
}

void CGEvent::afterSuccessfulVisit() const
{
	if(removeAfterVisit)
	{
		cb->removeAfterVisit(this);
	}
	else if(hasGuardians)
		hasGuardians = false;
}

void CGEvent::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGPandoraBox::serializeJsonOptions(handler);

	handler.serializeBool<bool>("aIActivable", computerActivate, true, false, false);
	handler.serializeBool<bool>("humanActivable", humanActivate, true, false, true);
	handler.serializeBool<bool>("removeAfterVisit", removeAfterVisit, true, false, false);

	{
		auto decodePlayer = [](const std::string & id)->si32
		{
			return vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, id);
		};

		auto encodePlayer = [](si32 idx)->std::string
		{
			return GameConstants::PLAYER_COLOR_NAMES[idx];
		};

		handler.serializeIdArray<ui8, PlayerColor::PLAYER_LIMIT_I>("availableFor", availableFor, GameConstants::ALL_PLAYERS, decodePlayer, encodePlayer);
    }
}
