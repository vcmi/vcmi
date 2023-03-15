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
#include "../StringConstants.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

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
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->getOwner();

	bool changesPrimSkill = false;
	for(const auto & elem : primskills)
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
			unpossessedAbilities.emplace_back(abilities[i], abilityLevels[i]);
		}
	}

	if(gainedExp || changesPrimSkill || !unpossessedAbilities.empty())
	{
		TExpType expVal = h->calculateXp(gainedExp);
		//getText(iw,afterBattle,175,h); //wtf?
		iw.text.addTxt(MetaString::ADVOB_TXT, 175); //%s learns something
		iw.text.addReplacement(h->getNameTranslated());

		if(expVal)
			iw.components.emplace_back(Component::EComponentType::EXPERIENCE, 0, static_cast<si32>(expVal), 0);

		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, i, primskills[i], 0);

		for(const auto & abilityData : unpossessedAbilities)
			iw.components.emplace_back(Component::EComponentType::SEC_SKILL, abilityData.first, abilityData.second, 0);

		cb->showInfoDialog(&iw);

		//give sec skills
		for(const auto & abilityData : unpossessedAbilities)
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
	iw.type = EInfoWindowMode::AUTO;
	iw.player = h->getOwner();

	//TODO: reuse this code for Scholar skill
	if(!spells.empty())
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
				const auto * spell = (*i).toSpell(VLC->spells());
				if(h->canLearnSpell(spell))
				{
					iw.components.emplace_back(Component::EComponentType::SPELL, *i, 0, 0);
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
				iw.text.addReplacement(h->getNameTranslated());
				cb->changeSpells(h, true, spellsToGive);
				cb->showInfoDialog(&iw);
			}
		}
	}

	if(manaDiff)
	{
		getText(iw,hadGuardians,manaDiff,176,177,h);
		iw.components.emplace_back(Component::EComponentType::PRIM_SKILL, 5, manaDiff, 0);
		cb->showInfoDialog(&iw);
		cb->setManaPoints(h->id, h->mana + manaDiff);
	}

	if(moraleDiff)
	{
		getText(iw,hadGuardians,moraleDiff,178,179,h);
		iw.components.emplace_back(Component::EComponentType::MORALE, 0, moraleDiff, 0);
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,moraleDiff,id.getNum(),"");
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	if(luckDiff)
	{
		getText(iw,hadGuardians,luckDiff,180,181,h);
		iw.components.emplace_back(Component::EComponentType::LUCK, 0, luckDiff, 0);
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
			iw.components.emplace_back(Component::EComponentType::RESOURCE, i, resources[i], 0);
	}
	if(!iw.components.empty())
	{
		getText(iw,hadGuardians,182,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	iw.text.clear();
	for(int i=0; i<resources.size(); i++)
	{
		if(resources[i] > 0)
			iw.components.emplace_back(Component::EComponentType::RESOURCE, i, resources[i], 0);
	}
	if(!iw.components.empty())
	{
		getText(iw,hadGuardians,183,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	// 	getText(iw,afterBattle,183,h);
	iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure
	iw.text.addReplacement(h->getNameTranslated());
	for(const auto & elem : artifacts)
	{
		iw.components.emplace_back(Component::EComponentType::ARTIFACT, elem, 0, 0);
		if(iw.components.size() >= 14)
		{
			cb->showInfoDialog(&iw);
			iw.components.clear();
			iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure - once more?
			iw.text.addReplacement(h->getNameTranslated());
		}
	}
	if(!iw.components.empty())
	{
		cb->showInfoDialog(&iw);
	}

	cb->giveResources(h->getOwner(), resources);

	for(const auto & elem : artifacts)
		cb->giveHeroNewArtifact(h, VLC->arth->objects[elem],ArtifactPosition::FIRST_AVAILABLE);

	iw.components.clear();
	iw.text.clear();

	if(creatures.stacksCount())
	{ //this part is taken straight from creature bank
		MetaString loot;
		for(const auto & elem : creatures.Slots())
		{ //build list of joined creatures
			iw.components.emplace_back(*elem.second);
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if(creatures.stacksCount() == 1 && creatures.Slots().begin()->second->count == 1)
			iw.text.addTxt(MetaString::ADVOB_TXT, 185);
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 186);

		iw.text.addReplacement(loot.buildList());
		iw.text.addReplacement(h->getNameTranslated());

		cb->showInfoDialog(&iw);
		cb->giveCreatures(this, h, creatures, false);
	}
	if(!hasGuardians && !msg.empty())
	{
		iw.text << msg;
		cb->showInfoDialog(&iw);
	}
}

void CGPandoraBox::getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const
{
	if(afterBattle || message.empty())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,text);//%s has lost treasure.
		iw.text.addReplacement(h->getNameTranslated());
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
	if(afterBattle || message.empty())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,val < 0 ? negative : positive); //%s's luck takes a turn for the worse / %s's luck increases
		iw.text.addReplacement(h->getNameTranslated());
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
			hero->showInfoDialog(16, 0, EInfoWindowMode::MODAL);
			cb->startBattleI(hero, this); //grants things after battle
		}
		else if(message.empty() && resources.empty()
			&& primskills.empty() && abilities.empty()
			&& abilityLevels.empty() && artifacts.empty()
			&& spells.empty() && creatures.stacksCount() > 0
			&& gainedExp == 0 && manaDiff == 0 && moraleDiff == 0 && luckDiff == 0) //if it gives nothing without battle
		{
			hero->showInfoDialog(15);
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
			for(int primskill : primskills)
				if(primskill != 0)
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
				handler.serializeEnum(CSkillHandler::encodeSkill(abilities[idx]), abilityLevels[idx], NSecondarySkill::levels);
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

			abilities.emplace_back(rawId);
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
		if(!message.empty())
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

VCMI_LIB_NAMESPACE_END
