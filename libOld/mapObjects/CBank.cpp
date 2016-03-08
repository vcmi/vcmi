/*
 * CBank.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CBank.h"

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "CommonConstructors.h"
#include "../spells/CSpellHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"

///helpers
static std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

CBank::CBank()
{
}

CBank::~CBank()
{
}

void CBank::initObj()
{
	daycounter = 0;
	resetDuration = 0;
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, cb->gameState()->getRandomGenerator());
}

std::string CBank::getHoverText(PlayerColor player) const
{
	// TODO: record visited players
	return getObjectName() + " " + visitedTxt(bc == nullptr);
}

void CBank::setConfig(const BankConfig & config)
{
	bc.reset(new BankConfig(config));
	clear(); // remove all stacks, if any

	for (auto & stack : config.guards)
		setCreature (SlotID(stacksCount()), stack.type->idNumber, stack.count);
}

void CBank::setPropertyDer (ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
				daycounter+=val;
			break;
		case ObjProperty::BANK_RESET:
			initObj();
			daycounter = 1; //yes, 1 since "today" daycounter won't be incremented
			break;
		case ObjProperty::BANK_CLEAR:
			bc.reset();
			break;
	}
}

void CBank::newTurn() const
{
	if (bc == nullptr)
	{
		if (resetDuration != 0)
		{
			if (daycounter >= resetDuration)
				cb->setObjProperty (id, ObjProperty::BANK_RESET, 0); //daycounter 0
			else
				cb->setObjProperty (id, ObjProperty::BANK_DAYCOUNTER, 1); //daycounter++
		}
	}
}

bool CBank::wasVisited (PlayerColor player) const
{
	return !bc; //FIXME: player A should not know about visit done by player B
}

void CBank::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
	{
		int banktext = 0;
		ui16 soundID = soundBase::ROGUE;
		switch (ID)
		{
		case Obj::CREATURE_BANK:
			banktext = 32;
			break;
		case Obj::DERELICT_SHIP:
			banktext = 41;
			break;
		case Obj::DRAGON_UTOPIA:
			banktext = 47;
			break;
		case Obj::CRYPT:
			banktext = 119;
			break;
		case Obj::SHIPWRECK:
			banktext = 122;
			break;
		case Obj::PYRAMID:
			soundID = soundBase::MYSTERY;
			banktext = 105;
			break;
		}
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundID;
		bd.text.addTxt(MetaString::ADVOB_TXT, banktext);
		if (ID == Obj::CREATURE_BANK)
			bd.text.addReplacement(getObjectName());
		cb->showBlockingDialog (&bd);
	}
	else
	{
		InfoWindow iw;
		iw.soundID = soundBase::GRAVEYARD;
		iw.player = h->getOwner();
		if (ID == Obj::PYRAMID) // You come upon the pyramid ... pyramid is completely empty.
		{
			iw.text << VLC->generaltexth->advobtxt[107];
			iw.components.push_back (Component (Component::LUCK, 0 , -2, 0));
			GiveBonus gb;
			gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,-2,id.getNum(),VLC->generaltexth->arraytxt[70]);
			gb.id = h->id.getNum();
			cb->giveHeroBonus(&gb);
		}
		else
		{
			iw.text << VLC->generaltexth->advobtxt[33];// This was X, now is completely empty
			iw.text.addReplacement(getObjectName());
		}
		cb->showInfoDialog(&iw);
	}
}

void CBank::doVisit(const CGHeroInstance * hero) const
{
	int textID = -1;
	InfoWindow iw;
	iw.player = hero->getOwner();
	MetaString loot;

	switch (ID)
	{
	case Obj::CREATURE_BANK:
	case Obj::DRAGON_UTOPIA:
		textID = 34;
		break;
	case Obj::DERELICT_SHIP:
		if (!bc)
			textID = 43;
		else
		{
			GiveBonus gbonus;
			gbonus.id = hero->id.getNum();
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.sid = ID;
			gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[101];
			gbonus.bonus.type = Bonus::MORALE;
			gbonus.bonus.val = -1;
			cb->giveHeroBonus(&gbonus);
			textID = 42;
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
		}
		break;
	case Obj::CRYPT:
		if (bc)
			textID = 121;
		else
		{
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
			GiveBonus gbonus;
			gbonus.id = hero->id.getNum();
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.sid = ID;
			gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[ID];
			gbonus.bonus.type = Bonus::MORALE;
			gbonus.bonus.val = -1;
			cb->giveHeroBonus(&gbonus);
			textID = 120;
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
		}
		break;
	case Obj::SHIPWRECK:
		if (bc)
			textID = 124;
		else
			textID = 123;
		break;
	case Obj::PYRAMID:
		textID = 106;
	}

	//grant resources
	if (bc)
	{
		for (int it = 0; it < bc->resources.size(); it++)
		{
			if (bc->resources[it] != 0)
			{
				iw.components.push_back (Component (Component::RESOURCE, it, bc->resources[it], 0));
				loot << "%d %s";
				loot.addReplacement(iw.components.back().val);
				loot.addReplacement(MetaString::RES_NAMES, iw.components.back().subtype);
				cb->giveResource (hero->getOwner(), static_cast<Res::ERes>(it), bc->resources[it]);
			}
		}
		//grant artifacts
		for (auto & elem : bc->artifacts)
		{
			iw.components.push_back (Component (Component::ARTIFACT, elem, 0, 0));
			loot << "%s";
			loot.addReplacement(MetaString::ART_NAMES, elem);
			cb->giveHeroNewArtifact (hero, VLC->arth->artifacts[elem], ArtifactPosition::FIRST_AVAILABLE);
		}
		//display loot
		if (!iw.components.empty())
		{
			iw.text.addTxt (MetaString::ADVOB_TXT, textID);
			if (textID == 34)
			{
				const CCreature * strongest = boost::range::max_element(bc->guards, [](const CStackBasicDescriptor & a, const CStackBasicDescriptor & b)
				{
					return a.type->fightValue < b.type->fightValue;
				})->type;

				iw.text.addReplacement(MetaString::CRE_PL_NAMES, strongest->idNumber);
				iw.text.addReplacement(loot.buildList());
			}
			cb->showInfoDialog(&iw);
		}

		if (!bc->spells.empty())
		{
			std::set<SpellID> spells;

			bool noWisdom = false;
			for (SpellID spell : bc->spells)
			{
				iw.text.addTxt (MetaString::SPELL_NAME, spell);
				if (VLC->spellh->objects[spell]->level <= hero->getSecSkillLevel(SecondarySkill::WISDOM) + 2)
				{
					spells.insert(spell);
					iw.components.push_back(Component (Component::SPELL, spell, 0, 0));
				}
				else
					noWisdom = true;
			}

			if (!hero->getArt(ArtifactPosition::SPELLBOOK))
				iw.text.addTxt (MetaString::ADVOB_TXT, 109); //no spellbook
			else if (noWisdom)
				iw.text.addTxt (MetaString::ADVOB_TXT, 108); //no expert Wisdom
			if (spells.empty())
				cb->changeSpells (hero, true, spells);
		}
		loot.clear();
		iw.components.clear();
		iw.text.clear();

		//grant creatures
		CCreatureSet ourArmy;
		for (auto slot : bc->creatures)
		{
			ourArmy.addToSlot(ourArmy.getSlotFor(slot.type->idNumber), slot.type->idNumber, slot.count);
		}

		for (auto & elem : ourArmy.Slots())
		{
			iw.components.push_back(Component(*elem.second));
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if(ourArmy.stacksCount())
		{
			if(ourArmy.stacksCount() == 1 && ourArmy.Slots().begin()->second->count == 1)
				iw.text.addTxt (MetaString::ADVOB_TXT, 185);
			else
				iw.text.addTxt (MetaString::ADVOB_TXT, 186);

			iw.text.addReplacement(loot.buildList());
			iw.text.addReplacement(hero->name);
			cb->showInfoDialog(&iw);
			cb->giveCreatures(this, hero, ourArmy, false);
		}
	cb->setObjProperty (id, ObjProperty::BANK_CLEAR, 0); //bc = nullptr
	}
}

void CBank::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
	{
		doVisit(hero);
	}
}

void CBank::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if (answer)
	{
		if (bc) // not looted bank
			cb->startBattleI(hero, this, true);
		else
			doVisit(hero);
	}
}
