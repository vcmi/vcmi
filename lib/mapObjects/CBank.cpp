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
#include "../client/CSoundBase.h"

using namespace boost::assign;

///helpers
static std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

void CBank::initObj()
{
	index = VLC->objh->bankObjToIndex(this);
	bc = nullptr;
	daycounter = 0;
	multiplier = 1;
}
const std::string & CBank::getHoverText() const
{
	bool visited = (bc == nullptr);
	hoverName = VLC->objh->creBanksNames[index] + " " + visitedTxt(visited);
	return hoverName;
}
void CBank::reset(ui16 var1) //prevents desync
{
	ui8 chance = 0;
	for (auto & elem : VLC->objh->banksInfo[index])
	{
		if (var1 < (chance += elem->chance))
		{
 			bc = elem;
			break;
		}
	}
	artifacts.clear();
}

void CBank::initialize() const
{
	cb->setObjProperty(id, ObjProperty::BANK_RESET, cb->gameState()->getRandomGenerator().nextInt()); //synchronous reset

	for (ui8 i = 0; i <= 3; i++)
	{
		for (ui8 n = 0; n < bc->artifacts[i]; n++)
		{
			CArtifact::EartClass artClass;
			switch(i)
			{
			case 0: artClass = CArtifact::ART_TREASURE; break;
			case 1: artClass = CArtifact::ART_MINOR; break;
			case 2: artClass = CArtifact::ART_MAJOR; break;
			case 3: artClass = CArtifact::ART_RELIC; break;
			default: assert(0); continue;
			}

			int artID = VLC->arth->pickRandomArtifact(cb->gameState()->getRandomGenerator(), artClass);
			cb->setObjProperty(id, ObjProperty::BANK_ADD_ARTIFACT, artID);
		}
	}

	cb->setObjProperty(id, ObjProperty::BANK_INIT_ARMY, cb->gameState()->getRandomGenerator().nextInt()); //get army
}
void CBank::setPropertyDer (ui8 what, ui32 val)
/// random values are passed as arguments and processed identically on all clients
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
			if (val == 0)
				daycounter = 1; //yes, 1
			else
				daycounter++;
			break;
		case ObjProperty::BANK_MULTIPLIER: //multiplier, in percent
			multiplier = val / 100.0;
			break;
		case 13: //bank preset
			bc = VLC->objh->banksInfo[index][val];
			break;
		case ObjProperty::BANK_RESET:
			reset (val%100);
			break;
		case ObjProperty::BANK_CLEAR_CONFIG:
			bc = nullptr;
			break;
		case ObjProperty::BANK_CLEAR_ARTIFACTS: //remove rewards from Derelict Ship
			artifacts.clear();
			break;
		case ObjProperty::BANK_INIT_ARMY: //set ArmedInstance army
			{
				int upgraded = 0;
				if (val%100 < bc->upgradeChance) //once again anti-desync
					upgraded = 1;
				switch (bc->guards.size())
				{
					case 1:
						for	(int i = 0; i < 4; ++i)
							setCreature (SlotID(i), bc->guards[0].first, bc->guards[0].second  / 5 );
						setCreature (SlotID(4), CreatureID(bc->guards[0].first + upgraded), bc->guards[0].second  / 5 );
						break;
					case 4:
					{
						if (bc->guards.back().second) //all stacks are present
						{
							for (auto & elem : bc->guards)
							{
								setCreature (SlotID(stacksCount()), elem.first, elem.second);
							}
						}
						else if (bc->guards[2].second)//Wraiths are present, split two stacks in Crypt
						{
							setCreature (SlotID(0), bc->guards[0].first, bc->guards[0].second  / 2 );
							setCreature (SlotID(1), bc->guards[1].first, bc->guards[1].second / 2);
							setCreature (SlotID(2), CreatureID(bc->guards[2].first + upgraded), bc->guards[2].second);
							setCreature (SlotID(3), bc->guards[1].first, bc->guards[1].second / 2 );
							setCreature (SlotID(4), bc->guards[0].first, bc->guards[0].second - (bc->guards[0].second  / 2) );

						}
						else //split both stacks
						{
							for	(int i = 0; i < 3; ++i) //skellies
								setCreature (SlotID(2*i), bc->guards[0].first, bc->guards[0].second  / 3);
							for	(int i = 0; i < 2; ++i) //zombies
								setCreature (SlotID(2*i+1), bc->guards[1].first, bc->guards[1].second  / 2);
						}
					}
						break;
					default:
                        logGlobal->warnStream() << "Error: Unexpected army data: " << bc->guards.size() <<" items found";
						return;
				}
			}
			break;
		case ObjProperty::BANK_ADD_ARTIFACT: //add Artifact
		{
			artifacts.push_back (val);
			break;
		}
	}
}

void CBank::newTurn() const
{
	if (bc == nullptr)
	{
		if (cb->getDate() == 1)
			initialize(); //initialize on first day
		else if (daycounter >= 28 && (subID < 13 || subID > 16)) //no reset for Emissaries
		{
			initialize();
			cb->setObjProperty (id, ObjProperty::BANK_DAYCOUNTER, 0); //daycounter 0
			if (ID == Obj::DERELICT_SHIP && cb->getDate() > 1)
			{
				cb->setObjProperty (id, ObjProperty::BANK_MULTIPLIER, 0);//ugly hack to make derelict ships usable only once
				cb->setObjProperty (id, ObjProperty::BANK_CLEAR_ARTIFACTS, 0);
			}
		}
		else
			cb->setObjProperty (id, ObjProperty::BANK_DAYCOUNTER, 1); //daycounter++
	}
}
bool CBank::wasVisited (PlayerColor player) const
{
	return !bc;
}

void CBank::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
	{
		int banktext = 0;
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
		}
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::ROGUE;
		bd.text.addTxt(MetaString::ADVOB_TXT,banktext);
		if (ID == Obj::CREATURE_BANK)
			bd.text.addReplacement(VLC->objh->creBanksNames[index]);
		cb->showBlockingDialog (&bd);
	}
	else
	{
		InfoWindow iw;
		iw.soundID = soundBase::GRAVEYARD;
		iw.player = h->getOwner();
		if (ID == Obj::CRYPT) //morale penalty for empty Crypt
		{
			GiveBonus gbonus;
			gbonus.id = h->id.getNum();
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.sid = ID;
			gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[98];
			gbonus.bonus.type = Bonus::MORALE;
			gbonus.bonus.val = -1;
			cb->giveHeroBonus(&gbonus);
			iw.text << VLC->generaltexth->advobtxt[120];
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
		}
		else
		{
			iw.text << VLC->generaltexth->advobtxt[33];
			iw.text.addReplacement(VLC->objh->creBanksNames[index]);
		}
		cb->showInfoDialog(&iw);
	}
}

void CBank::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
	{
		int textID = -1;
		InfoWindow iw;
		iw.player = hero->getOwner();
		MetaString loot;

		switch (ID)
		{
		case Obj::CREATURE_BANK: case Obj::DRAGON_UTOPIA:
			textID = 34;
			break;
		case Obj::DERELICT_SHIP:
			if (multiplier)
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
			if (bc->resources.size() != 0)
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
			if (bc->resources.size())
				textID = 124;
			else
				textID = 123;
			break;
		}

		//grant resources
		if (textID != 42) //empty derelict ship gives no cash
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
		}
		//grant artifacts
		for (auto & elem : artifacts)
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
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, result.casualties[1].begin()->first);
				iw.text.addReplacement(loot.buildList());
			}
			cb->showInfoDialog(&iw);
		}
		loot.clear();
		iw.components.clear();
		iw.text.clear();

		//grant creatures
		CCreatureSet ourArmy;
		for (auto it = bc->creatures.cbegin(); it != bc->creatures.cend(); it++)
		{
			SlotID slot = ourArmy.getSlotFor(it->first);
			ourArmy.addToSlot(slot, it->first, it->second);
		}
		for (auto & elem : ourArmy.Slots())
		{
			iw.components.push_back(Component(*elem.second));
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if (ourArmy.Slots().size())
		{
			if (ourArmy.Slots().size() == 1 && ourArmy.Slots().begin()->second->count == 1)
				iw.text.addTxt (MetaString::ADVOB_TXT, 185);
			else
				iw.text.addTxt (MetaString::ADVOB_TXT, 186);

			iw.text.addReplacement(loot.buildList());
			iw.text.addReplacement(hero->name);
			cb->showInfoDialog(&iw);
			cb->giveCreatures(this, hero, ourArmy, false);
		}
		cb->setObjProperty (id, ObjProperty::BANK_CLEAR_CONFIG, 0); //bc = nullptr
	}
}

void CBank::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if (answer)
	{
		cb->startBattleI(hero, this, true);
	}
}

void CGPyramid::initObj()
{
	std::vector<SpellID> available;
	cb->getAllowedSpells (available, 5);
	if (available.size())
	{
		bc = VLC->objh->banksInfo[21].front(); //TODO: remove hardcoded value?
		spell = *RandomGeneratorUtil::nextItem(available, cb->gameState()->getRandomGenerator());
	}
	else
	{
        logGlobal->errorStream() <<"No spells available for Pyramid! Object set to empty.";
	}
	setPropertyDer(ObjProperty::BANK_INIT_ARMY, cb->gameState()->getRandomGenerator().nextInt()); //set guards at game start
}
const std::string & CGPyramid::getHoverText() const
{
	hoverName = VLC->objh->creBanksNames[21]+ " " + visitedTxt((bc==nullptr));
	return hoverName;
}
void CGPyramid::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::MYSTERY;
		bd.text << VLC->generaltexth->advobtxt[105];
		cb->showBlockingDialog(&bd);
	}
	else
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.text << VLC->generaltexth->advobtxt[107];
		iw.components.push_back (Component (Component::LUCK, 0 , -2, 0));
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,-2,id.getNum(),VLC->generaltexth->arraytxt[70]);
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
		cb->showInfoDialog(&iw);
	}
}

void CGPyramid::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
	{
		InfoWindow iw;
		iw.player = hero->getOwner();
		iw.text.addTxt (MetaString::ADVOB_TXT, 106);
		iw.text.addTxt (MetaString::SPELL_NAME, spell);
		if (!hero->getArt(ArtifactPosition::SPELLBOOK))
			iw.text.addTxt (MetaString::ADVOB_TXT, 109); //no spellbook
		else if (hero->getSecSkillLevel(SecondarySkill::WISDOM) < 3)
			iw.text.addTxt (MetaString::ADVOB_TXT, 108); //no expert Wisdom
		else
		{
			std::set<SpellID> spells;
			spells.insert (SpellID(spell));
			cb->changeSpells (hero, true, spells);
			iw.components.push_back(Component (Component::SPELL, spell, 0, 0));
		}
		cb->showInfoDialog(&iw);
		cb->setObjProperty (id, ObjProperty::BANK_CLEAR_CONFIG, 0);
	}
}
