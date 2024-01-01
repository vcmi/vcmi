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

#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "../GameSettings.h"
#include "../CPlayerState.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/CBankInstanceConstructor.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/Component.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
#include "../MetaString.h"
#include "../IGameCallback.h"
#include "../gameState/CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

///helpers
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

CBank::CBank(IGameCallback *cb)
	: CArmedInstance(cb)
{}

//must be instantiated in .cpp file for access to complete types of all member fields
CBank::~CBank() = default;

void CBank::initObj(CRandomGenerator & rand)
{
	daycounter = 0;
	resetDuration = 0;
	getObjectHandler()->configureObject(this, rand);
}

bool CBank::isCoastVisitable() const
{
	return coastVisitable;
}

std::string CBank::getHoverText(PlayerColor player) const
{
	if (!wasVisited(player))
		return getObjectName();

	return getObjectName() + "\n" + visitedTxt(bc == nullptr);
}

std::vector<Component> CBank::getPopupComponents(PlayerColor player) const
{
	if (!wasVisited(player))
		return {};

	if (!VLC->settings()->getBoolean(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION))
		return {};

	if (bc == nullptr)
		return {};

	std::map<CreatureID, int> guardsAmounts;
	std::vector<Component> result;

	for (auto const & slot : Slots())
		if (slot.second)
			guardsAmounts[slot.second->getCreatureID()] += slot.second->getCount();

	for (auto const & guard : guardsAmounts)
	{
		Component comp(ComponentType::CREATURE, guard.first, guard.second);
		result.push_back(comp);
	}
	return result;
}

void CBank::setConfig(const BankConfig & config)
{
	bc = std::make_unique<BankConfig>(config);
	clearSlots(); // remove all stacks, if any

	for(const auto & stack : config.guards)
		setCreature (SlotID(stacksCount()), stack.type->getId(), stack.count);
}

void CBank::setPropertyDer (ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
				daycounter+= identifier.getNum();
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
				cb->setObjPropertyValue(id, ObjProperty::BANK_RESET); //daycounter 0
			else
				cb->setObjPropertyValue(id, ObjProperty::BANK_DAYCOUNTER, 1); //daycounter++
		}
	}
}

bool CBank::wasVisited (PlayerColor player) const
{
	return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
}

void CBank::onHeroVisit(const CGHeroInstance * h) const
{
	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_TEAM, id, h->id);
	cb->sendAndApply(&cov);

	int banktext = 0;
	switch (ID.toEnum())
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
	bd.text.appendLocalString(EMetaText::ADVOB_TXT, banktext);
	bd.components = getPopupComponents(h->getOwner());
	if (banktext == 32)
		bd.text.replaceRawString(getObjectName());

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
		switch (ID.toEnum())
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
		switch (ID.toEnum())
		{
		case Obj::SHIPWRECK:
		case Obj::DERELICT_SHIP:
		case Obj::CRYPT:
		{
			GiveBonus gbonus;
			gbonus.id = hero->id;
			gbonus.bonus.duration = BonusDuration::ONE_BATTLE;
			gbonus.bonus.source = BonusSource::OBJECT_TYPE;
			gbonus.bonus.sid = BonusSourceID(ID);
			gbonus.bonus.type = BonusType::MORALE;
			gbonus.bonus.val = -1;
			switch (ID.toEnum())
			{
			case Obj::SHIPWRECK:
				textID = 123;
				gbonus.bdescr.appendRawString(VLC->generaltexth->arraytxt[99]);
				break;
			case Obj::DERELICT_SHIP:
				textID = 42;
				gbonus.bdescr.appendRawString(VLC->generaltexth->arraytxt[101]);
				break;
			case Obj::CRYPT:
				textID = 120;
				gbonus.bdescr.appendRawString(VLC->generaltexth->arraytxt[98]);
				break;
			}
			cb->giveHeroBonus(&gbonus);
			iw.components.emplace_back(ComponentType::MORALE, -1);
			iw.soundID = soundBase::GRAVEYARD;
			break;
		}
		case Obj::PYRAMID:
		{
			GiveBonus gb;
			gb.bonus = Bonus(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, -2, BonusSourceID(id), VLC->generaltexth->arraytxt[70]);
			gb.id = hero->id;
			cb->giveHeroBonus(&gb);
			textID = 107;
			iw.components.emplace_back(ComponentType::LUCK, -2);
			break;
		}
		case Obj::CREATURE_BANK:
		case Obj::DRAGON_UTOPIA:
		default:
			iw.text.appendRawString(VLC->generaltexth->advobtxt[33]);// This was X, now is completely empty
			iw.text.replaceRawString(getObjectName());
		}
		if(textID != -1)
		{
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, textID);
		}
		cb->showInfoDialog(&iw);
	}


	//grant resources
	if (bc)
	{
		for (GameResID it : GameResID::ALL_RESOURCES())
		{
			if (bc->resources[it] != 0)
			{
				iw.components.emplace_back(ComponentType::RESOURCE, it, bc->resources[it]);
				loot.appendRawString("%d %s");
				loot.replaceNumber(bc->resources[it]);
				loot.replaceName(it);
				cb->giveResource(hero->getOwner(), it, bc->resources[it]);
			}
		}
		//grant artifacts
		for (auto & elem : bc->artifacts)
		{
			iw.components.emplace_back(ComponentType::ARTIFACT, elem);
			loot.appendRawString("%s");
			loot.replaceName(elem);
			cb->giveHeroNewArtifact(hero, elem.toArtifact(), ArtifactPosition::FIRST_AVAILABLE);
		}
		//display loot
		if (!iw.components.empty())
		{
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, textID);
			if (textID == 34)
			{
				const auto * strongest = boost::range::max_element(bc->guards, [](const CStackBasicDescriptor & a, const CStackBasicDescriptor & b)
				{
					return a.type->getFightValue() < b.type->getFightValue();
				})->type;

				iw.text.replaceNamePlural(strongest->getId());
				iw.text.replaceRawString(loot.buildList());
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
				iw.text.appendLocalString(EMetaText::ADVOB_TXT, textID); //pyramid
			}
			for(const SpellID & spellId : bc->spells)
			{
				const auto * spell = spellId.toEntity(VLC);
				iw.text.appendName(spellId);
				if(spell->getLevel() <= hero->maxSpellLevel())
				{
					if(hero->canLearnSpell(spell))
					{
						spells.insert(spellId);
						iw.components.emplace_back(ComponentType::SPELL, spellId);
					}
				}
				else
					noWisdom = true;
			}

			if (!hero->getArt(ArtifactPosition::SPELLBOOK))
				iw.text.appendLocalString(EMetaText::ADVOB_TXT, 109); //no spellbook
			else if(noWisdom)
				iw.text.appendLocalString(EMetaText::ADVOB_TXT, 108); //no expert Wisdom

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
			ourArmy.addToSlot(ourArmy.getSlotFor(slot.type->getId()), slot.type->getId(), slot.count);
		}

		for(const auto & elem : ourArmy.Slots())
		{
			iw.components.emplace_back(ComponentType::CREATURE, elem.second->getId(), elem.second->getCount());
			loot.appendRawString("%s");
			loot.replaceName(*elem.second);
		}

		if(ourArmy.stacksCount())
		{
			if(ourArmy.stacksCount() == 1 && ourArmy.Slots().begin()->second->count == 1)
				iw.text.appendLocalString(EMetaText::ADVOB_TXT, 185);
			else
				iw.text.appendLocalString(EMetaText::ADVOB_TXT, 186);

			iw.text.replaceRawString(loot.buildList());
			iw.text.replaceRawString(hero->getNameTranslated());
			cb->showInfoDialog(&iw);
			cb->giveCreatures(this, hero, ourArmy, false);
		}
		cb->setObjPropertyValue(id, ObjProperty::BANK_CLEAR); //bc = nullptr
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
