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

#include "../texts/CGeneralTextHandler.h"
#include "../IGameSettings.h"
#include "../CPlayerState.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/CBankInstanceConstructor.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/Component.h"
#include "../networkPacks/PacksForClient.h"
#include "../networkPacks/PacksForClientBattle.h"
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

void CBank::initObj(vstd::RNG & rand)
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

	return getObjectName() + "\n" + visitedTxt(bankConfig == nullptr);
}

std::vector<Component> CBank::getPopupComponents(PlayerColor player) const
{
	if (!wasVisited(player))
		return {};

	if (!cb->getSettings().getBoolean(EGameSettings::BANKS_SHOW_GUARDS_COMPOSITION))
		return {};

	if (bankConfig == nullptr)
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
	bankConfig = std::make_unique<BankConfig>(config);
	clearSlots(); // remove all stacks, if any

	for(const auto & stack : config.guards)
		setCreature (SlotID(stacksCount()), stack.type->getId(), stack.count);

	daycounter = 1; //yes, 1 since "today" daycounter won't be incremented
}

void CBank::setPropertyDer (ObjProperty what, ObjPropertyID identifier)
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
				daycounter+= identifier.getNum();
			break;
		case ObjProperty::BANK_CLEAR:
			bankConfig.reset();
			break;
	}
}

void CBank::newTurn(vstd::RNG & rand) const
{
	if (bankConfig == nullptr)
	{
		if (resetDuration != 0)
		{
			if (daycounter >= resetDuration)
			{
				auto handler = std::dynamic_pointer_cast<CBankInstanceConstructor>(getObjectHandler());
				auto config = handler->generateConfiguration(cb, rand, ID);
				cb->setBankObjectConfiguration(id, config);
			}
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
	ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_PLAYER, id, h->id);
	cb->sendAndApply(cov);

	BlockingDialog bd(true, false);
	bd.player = h->getOwner();
	bd.text.appendLocalString(EMetaText::ADVOB_TXT, 32);
	bd.components = getPopupComponents(h->getOwner());
	bd.text.replaceTextID(getObjectHandler()->getNameTextID());
	cb->showBlockingDialog(this, &bd);
}

void CBank::doVisit(const CGHeroInstance * hero) const
{
	InfoWindow iw;
	iw.type = EInfoWindowMode::AUTO;
	iw.player = hero->getOwner();
	MetaString loot;

	if (!bankConfig)
	{
		iw.text.appendRawString(VLC->generaltexth->advobtxt[33]);// This was X, now is completely empty
		iw.text.replaceTextID(getObjectHandler()->getNameTextID());
		cb->showInfoDialog(&iw);
	}

	//grant resources
	if (bankConfig)
	{
		for (GameResID it : GameResID::ALL_RESOURCES())
		{
			if (bankConfig->resources[it] != 0)
			{
				iw.components.emplace_back(ComponentType::RESOURCE, it, bankConfig->resources[it]);
				loot.appendRawString("%d %s");
				loot.replaceNumber(bankConfig->resources[it]);
				loot.replaceName(it);
				cb->giveResource(hero->getOwner(), it, bankConfig->resources[it]);
			}
		}
		//grant artifacts
		for (auto & elem : bankConfig->artifacts)
		{
			iw.components.emplace_back(ComponentType::ARTIFACT, elem);
			loot.appendRawString("%s");
			loot.replaceName(elem);
			cb->giveHeroNewArtifact(hero, elem, ArtifactPosition::FIRST_AVAILABLE);
		}
		//display loot
		if (!iw.components.empty())
		{
			iw.text.appendLocalString(EMetaText::ADVOB_TXT, 34);
			const auto * strongest = boost::range::max_element(bankConfig->guards, [](const CStackBasicDescriptor & a, const CStackBasicDescriptor & b)
			{
				return a.type->getFightValue() < b.type->getFightValue();
			})->type;

			iw.text.replaceNamePlural(strongest->getId());
			iw.text.replaceRawString(loot.buildList());

			cb->showInfoDialog(&iw);
		}

		loot.clear();
		iw.components.clear();
		iw.text.clear();

		if (!bankConfig->spells.empty())
		{
			std::set<SpellID> spells;

			bool noWisdom = false;

			for(const SpellID & spellId : bankConfig->spells)
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
		for(const auto & slot : bankConfig->creatures)
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
	if (result.winner == BattleSide::ATTACKER)
	{
		doVisit(hero);
	}
}

void CBank::blockingDialogAnswered(const CGHeroInstance *hero, int32_t answer) const
{
	if (answer)
	{
		if (bankConfig) // not looted bank
			cb->startBattle(hero, this);
		else
			doVisit(hero);
	}
}

VCMI_LIB_NAMESPACE_END
