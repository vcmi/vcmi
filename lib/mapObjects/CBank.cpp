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

#include <vcmi/spells/Spell.h>
#include <vcmi/spells/Service.h>

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "CommonConstructors.h"
#include "../IGameCallback.h"
#include "../CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

///helpers
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

//must be instantiated in .cpp file for access to complete types of all member fields
CBank::CBank() = default;
//must be instantiated in .cpp file for access to complete types of all member fields
CBank::~CBank() = default;

void CBank::initObj(CRandomGenerator & rand)
{
	daycounter = 0;
	resetDuration = 0;
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);
}

std::string CBank::getHoverText(PlayerColor player) const
{
	// TODO: record visited players
	return getObjectName() + " " + visitedTxt(bc == nullptr);
}

void CBank::setConfig(const BankConfig & config)
{
	bc = std::make_unique<BankConfig>(config);
	clear(); // remove all stacks, if any

	for(const auto & stack : config.guards)
		setCreature (SlotID(stacksCount()), stack.type->getId(), stack.count);
}

void CBank::setPropertyDer (ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
				daycounter+=val;
			break;
		case ObjProperty::BANK_RESET:
			// FIXME: Object reset must be done by separate netpack from server
			initObj(cb->gameState()->getRandomGenerator());
			daycounter = 1; //yes, 1 since "today" daycounter won't be incremented
			break;
		case ObjProperty::BANK_CLEAR:
			bc.reset();
			break;
	}
}

void CBank::newTurn(CRandomGenerator & rand) const
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

void CBank::onHeroVisit(const CGHeroInstance * h) const
{
	int banktext = 0;
	switch (ID)
	{
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
		banktext = 105;
		break;
	case Obj::CREATURE_BANK:
	default:
		banktext = 32;
		break;
	}
	BlockingDialog bd(true, false);
	bd.player = h->getOwner();
	bd.soundID = soundBase::invalid; // Sound is handled in json files, else two sounds are played
	bd.text.addTxt(MetaString::ADVOB_TXT, banktext);
	if (banktext == 32)
		bd.text.addReplacement(getObjectName());
	cb->showBlockingDialog(&bd);
}

void CBank::doVisit(const CGHeroInstance * hero) const
{
	int textID = -1;
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = hero->getOwner();
	MetaString loot;

	if (bc)
	{
		switch (ID)
		{
		case Obj::DERELICT_SHIP:
			textID = 43;
			break;
		case Obj::CRYPT:
			textID = 121;
			break;
		case Obj::SHIPWRECK:
			textID = 124;
			break;
		case Obj::PYRAMID:
			textID = 106;
			break;
		case Obj::CREATURE_BANK:
		case Obj::DRAGON_UTOPIA:
		default:
			textID = 34;
			break;
		}
	}
	else
	{
		switch (ID)
		{
		case Obj::SHIPWRECK:
		case Obj::DERELICT_SHIP:
		case Obj::CRYPT:
		{
			GiveBonus gbonus;
			gbonus.id = hero->id.getNum();
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.sid = ID;
			gbonus.bonus.type = Bonus::MORALE;
			gbonus.bonus.val = -1;
			switch (ID)
			{
			case Obj::SHIPWRECK:
				textID = 123;
				gbonus.bdescr << VLC->generaltexth->arraytxt[99];
				break;
			case Obj::DERELICT_SHIP:
				textID = 42;
				gbonus.bdescr << VLC->generaltexth->arraytxt[101];
				break;
			case Obj::CRYPT:
				textID = 120;
				gbonus.bdescr << VLC->generaltexth->arraytxt[98];
				break;
			}
			cb->giveHeroBonus(&gbonus);
			iw.components.emplace_back(Component::MORALE, 0, -1, 0);
			iw.soundID = soundBase::GRAVEYARD;
			break;
		}
		case Obj::PYRAMID:
		{
			GiveBonus gb;
			gb.bonus = Bonus(Bonus::ONE_BATTLE, Bonus::LUCK, Bonus::OBJECT, -2, id.getNum(), VLC->generaltexth->arraytxt[70]);
			gb.id = hero->id.getNum();
			cb->giveHeroBonus(&gb);
			textID = 107;
			iw.components.emplace_back(Component::LUCK, 0, -2, 0);
			break;
		}
		case Obj::CREATURE_BANK:
		case Obj::DRAGON_UTOPIA:
		default:
			iw.text << VLC->generaltexth->advobtxt[33];// This was X, now is completely empty
			iw.text.addReplacement(getObjectName());
		}
		if(textID != -1)
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, textID);
		}
		cb->showInfoDialog(&iw);
	}


	//grant resources
	if (bc)
	{
		for (int it = 0; it < bc->resources.size(); it++)
		{
			if (bc->resources[it] != 0)
			{
				iw.components.emplace_back(Component::RESOURCE, it, bc->resources[it], 0);
				loot << "%d %s";
				loot.addReplacement(iw.components.back().val);
				loot.addReplacement(MetaString::RES_NAMES, iw.components.back().subtype);
				cb->giveResource(hero->getOwner(), static_cast<Res::ERes>(it), bc->resources[it]);
			}
		}
		//grant artifacts
		for (auto & elem : bc->artifacts)
		{
			iw.components.emplace_back(Component::ARTIFACT, elem, 0, 0);
			loot << "%s";
			loot.addReplacement(MetaString::ART_NAMES, elem);
			cb->giveHeroNewArtifact(hero, VLC->arth->objects[elem], ArtifactPosition::FIRST_AVAILABLE);
		}
		//display loot
		if (!iw.components.empty())
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, textID);
			if (textID == 34)
			{
				const CCreature * strongest = boost::range::max_element(bc->guards, [](const CStackBasicDescriptor & a, const CStackBasicDescriptor & b)
				{
					return a.type->fightValue < b.type->fightValue;
				})->type;

				iw.text.addReplacement(MetaString::CRE_PL_NAMES, strongest->getId());
				iw.text.addReplacement(loot.buildList());
			}
			cb->showInfoDialog(&iw);
		}

		loot.clear();
		iw.components.clear();
		iw.text.clear();

		if (!bc->spells.empty())
		{
			std::set<SpellID> spells;

			bool noWisdom = false;
			if(textID == 106)
			{
				iw.text.addTxt(MetaString::ADVOB_TXT, textID); //pyramid
			}
			for(const SpellID & spellId : bc->spells)
			{
				const auto * spell = spellId.toSpell(VLC->spells());
				iw.text.addTxt(MetaString::SPELL_NAME, spellId);
				if(spell->getLevel() <= hero->maxSpellLevel())
				{
					if(hero->canLearnSpell(spell))
					{
						spells.insert(spellId);
						iw.components.emplace_back(Component::SPELL, spellId, 0, 0);
					}
				}
				else
					noWisdom = true;
			}

			if (!hero->getArt(ArtifactPosition::SPELLBOOK))
				iw.text.addTxt(MetaString::ADVOB_TXT, 109); //no spellbook
			else if(noWisdom)
				iw.text.addTxt(MetaString::ADVOB_TXT, 108); //no expert Wisdom

			if(!iw.components.empty() || !iw.text.toString().empty())
				cb->showInfoDialog(&iw);

			if(!spells.empty())
				cb->changeSpells(hero, true, spells);
		}

		iw.components.clear();
		iw.text.clear();

		//grant creatures
		CCreatureSet ourArmy;
		for(const auto & slot : bc->creatures)
		{
			ourArmy.addToSlot(ourArmy.getSlotFor(slot.type->idNumber), slot.type->getId(), slot.count);
		}

		for(const auto & elem : ourArmy.Slots())
		{
			iw.components.emplace_back(*elem.second);
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if(ourArmy.stacksCount())
		{
			if(ourArmy.stacksCount() == 1 && ourArmy.Slots().begin()->second->count == 1)
				iw.text.addTxt(MetaString::ADVOB_TXT, 185);
			else
				iw.text.addTxt(MetaString::ADVOB_TXT, 186);

			iw.text.addReplacement(loot.buildList());
			iw.text.addReplacement(hero->getNameTranslated());
			cb->showInfoDialog(&iw);
			cb->giveCreatures(this, hero, ourArmy, false);
		}
		cb->setObjProperty(id, ObjProperty::BANK_CLEAR, 0); //bc = nullptr
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

VCMI_LIB_NAMESPACE_END
