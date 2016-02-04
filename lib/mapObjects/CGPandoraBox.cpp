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

#include "../NetPacks.h"
#include "../CSoundBase.h"

#include "../spells/CSpellHandler.h"
#include "../StartInfo.h"
#include "../IGameCallback.h"

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

void CGPandoraBox::initObj()
{
	blockVisit = (ID==Obj::PANDORAS_BOX); //block only if it's really pandora's box (events also derive from that class)
	hasGuardians = stacks.size();
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::QUEST;
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

	if(gainedExp || changesPrimSkill || abilities.size())
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

		for(int i=0; i<abilities.size(); i++)
			iw.components.push_back(Component(Component::SEC_SKILL,abilities[i],abilityLevels[i],0));

		cb->showInfoDialog(&iw);

		//give sec skills
		for(int i=0; i<abilities.size(); i++)
		{
			int curLev = h->getSecSkillLevel(abilities[i]);

			if( (curLev  &&  curLev < abilityLevels[i])	|| (h->canLearnSkill() ))
			{
				cb->changeSecSkill(h,abilities[i],abilityLevels[i],true);
			}
		}

		//give prim skills
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(i),primskills[i],false);

		assert(!cb->isVisitCoveredByAnotherQuery(this, h));

		//give exp
		if(expVal)
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, expVal, false);
	}

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

	if(spells.size())
	{
		std::set<SpellID> spellsToGive;
		iw.components.clear();
		if (spells.size() > 1)
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, 188); //%s learns spells
		}
		else
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, 184); //%s learns a spell
		}
		iw.text.addReplacement(h->name);
		std::vector<ConstTransitivePtr<CSpell> > * sp = &VLC->spellh->objects;
		for(auto i=spells.cbegin(); i != spells.cend(); i++)
		{
			if ((*sp)[*i]->level <= h->getSecSkillLevel(SecondarySkill::WISDOM) + 2) //enough wisdom
			{
				iw.components.push_back(Component(Component::SPELL,*i,0,0));
				spellsToGive.insert(*i);
			}
		}
		if(!spellsToGive.empty())
		{
			cb->changeSpells(h,true,spellsToGive);
			cb->showInfoDialog(&iw);
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

	for(int i=0; i<resources.size(); i++)
		if(resources[i])
			cb->giveResource(h->getOwner(),static_cast<Res::ERes>(i),resources[i]);

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
		cb->giveCreatures(this, h, creatures, true);
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

void CGEvent::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!(availableFor & (1 << h->tempOwner.getNum())))
		return;
	if(cb->getPlayerSettings(h->tempOwner)->playerID)
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
