/*
 * CGHeroInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CGHeroInstance.h"

#include "../NetPacks.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CModHandler.h"
#include "../CSoundBase.h"
#include "../spells/CSpellHandler.h"
#include "CObjectClassesHandler.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../CCreatureHandler.h"
#include "../BattleState.h"
#include "../CTownHandler.h"
#include "../mapping/CMap.h"
#include "CGTownInstance.h"

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

static int lowestSpeed(const CGHeroInstance * chi)
{
	if(!chi->stacksCount())
	{
        logGlobal->errorStream() << "Error! Hero " << chi->id.getNum() << " ("<<chi->name<<") has no army!";
		return 20;
	}
	auto i = chi->Slots().begin();
	//TODO? should speed modifiers (eg from artifacts) affect hero movement?
	int ret = (i++)->second->valOfBonuses(Bonus::STACKS_SPEED);
	for (;i!=chi->Slots().end();i++)
	{
		ret = std::min(ret, i->second->valOfBonuses(Bonus::STACKS_SPEED));
	}
	return ret;
}

ui32 CGHeroInstance::getTileCost(const TerrainTile &dest, const TerrainTile &from, const TurnInfo * ti) const
{
	unsigned ret = GameConstants::BASE_MOVEMENT_COST;

	//if there is road both on dest and src tiles - use road movement cost
	if(dest.roadType != ERoadType::NO_ROAD && from.roadType != ERoadType::NO_ROAD)
	{
		int road = std::min(dest.roadType,from.roadType); //used road ID
		switch(road)
		{
		case ERoadType::DIRT_ROAD:
			ret = 75;
			break;
		case ERoadType::GRAVEL_ROAD:
			ret = 65;
			break;
		case ERoadType::COBBLESTONE_ROAD:
			ret = 50;
			break;
		default:
			logGlobal->errorStream() << "Unknown road type: " << road << "... Something wrong!";
			break;
		}
	}
	else if(ti->nativeTerrain != from.terType && !ti->hasBonusOfType(Bonus::NO_TERRAIN_PENALTY, from.terType))
	{
		ret = VLC->heroh->terrCosts[from.terType];
		ret -= getSecSkillLevel(SecondarySkill::PATHFINDING) * 25;
		if(ret < GameConstants::BASE_MOVEMENT_COST)
			ret = GameConstants::BASE_MOVEMENT_COST;
	}
	return ret;
}

int CGHeroInstance::getNativeTerrain() const
{
	// NOTE: in H3 neutral stacks will ignore terrain penalty only if placed as topmost stack(s) in hero army.
	// This is clearly bug in H3 however intended behaviour is not clear.
	// Current VCMI behaviour will ignore neutrals in calculations so army in VCMI
	// will always have best penalty without any influence from player-defined stacks order

	// TODO: What should we do if all hero stacks are neutral creatures?
	int nativeTerrain = -1;
	for(auto stack : stacks)
	{
		int stackNativeTerrain = VLC->townh->factions[stack.second->type->faction]->nativeTerrain;
		if(stackNativeTerrain == -1)
			continue;

		if(nativeTerrain == -1)
			nativeTerrain = stackNativeTerrain;
		else if(nativeTerrain != stackNativeTerrain)
			return -1;
	}

	return nativeTerrain;
}

int3 CGHeroInstance::convertPosition(int3 src, bool toh3m) //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
{
	if (toh3m)
	{
		src.x+=1;
		return src;
	}
	else
	{
		src.x-=1;
		return src;
	}
}
int3 CGHeroInstance::getPosition(bool h3m) const //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
{
	if (h3m)
	{
		return pos;
	}
	else
	{
		return convertPosition(pos,false);
	}
}

ui8 CGHeroInstance::getSecSkillLevel(SecondarySkill skill) const
{
	for(auto & elem : secSkills)
		if(elem.first == skill)
			return elem.second;
	return 0;
}

void CGHeroInstance::setSecSkillLevel(SecondarySkill which, int val, bool abs)
{
	if(getSecSkillLevel(which) == 0)
	{
		secSkills.push_back(std::pair<SecondarySkill,ui8>(which, val));
		updateSkill(which, val);
	}
	else
	{
		for (auto & elem : secSkills)
		{
			if(elem.first == which)
			{
				if(abs)
					elem.second = val;
				else
					elem.second += val;

				if(elem.second > 3) //workaround to avoid crashes when same sec skill is given more than once
				{
                    logGlobal->warnStream() << "Warning: Skill " << which << " increased over limit! Decreasing to Expert.";
					elem.second = 3;
				}
				updateSkill(which, elem.second); //when we know final value
			}
		}
	}
}

bool CGHeroInstance::canLearnSkill() const
{
	return secSkills.size() < GameConstants::SKILL_PER_HERO;
}

int CGHeroInstance::maxMovePoints(bool onLand, const TurnInfo * ti) const
{
	if(!ti)
		ti = new TurnInfo(this);

	int base;

	if(onLand)
	{
		// used function is f(x) = 66.6x + 1300, rounded to second digit, where x is lowest speed in army
		static const int baseSpeed = 1300; // base speed from creature with 0 speed

		int armySpeed = lowestSpeed(this) * 20 / 3;

		base = armySpeed * 10 + baseSpeed; // separate *10 is intentional to receive same rounding as in h3
		vstd::abetween(base, 1500, 2000); // base speed is limited by these values
	}
	else
	{
		base = 1500; //on water base movement is always 1500 (speed of army doesn't matter)
	}

	const Bonus::BonusType bt = onLand ? Bonus::LAND_MOVEMENT : Bonus::SEA_MOVEMENT;
	const int bonus = ti->valOfBonuses(Bonus::MOVEMENT) + ti->valOfBonuses(bt);

	const int subtype = onLand ? SecondarySkill::LOGISTICS : SecondarySkill::NAVIGATION;
	const double modifier = ti->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, subtype) / 100.0;

	return int(base* (1+modifier)) + bonus;
}

CGHeroInstance::CGHeroInstance()
 : IBoatGenerator(this)
{
	setNodeType(HERO);
	ID = Obj::HERO;
	tacticFormationEnabled = inTownGarrison = false;
	mana = UNINITIALIZED_MANA;
	movement = UNINITIALIZED_MOVEMENT;
	portrait = UNINITIALIZED_PORTRAIT;
	isStanding = true;
	moveDir = 4;
	level = 1;
	exp = 0xffffffff;
	visitedTown = nullptr;
	type = nullptr;
	boat = nullptr;
	commander = nullptr;
	sex = 0xff;
	secSkills.push_back(std::make_pair(SecondarySkill::DEFAULT, -1));
}

void CGHeroInstance::initHero(HeroTypeID SUBID)
{
	subID = SUBID.getNum();
	initHero();
}

void CGHeroInstance::setType(si32 ID, si32 subID)
{
	assert(ID == Obj::HERO); // just in case
	type = VLC->heroh->heroes[subID];
	portrait = type->imageIndex;
	CGObjectInstance::setType(ID, type->heroClass->id);
	randomizeArmy(type->heroClass->faction);
}

void CGHeroInstance::initHero()
{
	assert(validTypes(true));
	if(!type)
		type = VLC->heroh->heroes[subID];

	if (ID == Obj::HERO)
		appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, type->heroClass->id)->getTemplates().front();

	if(!vstd::contains(spells, SpellID::PRESET)) //hero starts with a spell
	{
		for(auto spellID : type->spells)
			spells.insert(spellID);
	}
	else //remove placeholder
		spells -= SpellID::PRESET;

	if(!getArt(ArtifactPosition::MACH4) && !getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook) //no catapult means we haven't read pre-existent set -> use default rules for spellbook
		putArtifact(ArtifactPosition::SPELLBOOK, CArtifactInstance::createNewArtifactInstance(ArtifactID::SPELLBOOK));

	if(!getArt(ArtifactPosition::MACH4))
		putArtifact(ArtifactPosition::MACH4, CArtifactInstance::createNewArtifactInstance(ArtifactID::CATAPULT)); //everyone has a catapult

	if(portrait < 0 || portrait == 255)
		portrait = type->imageIndex;
	if(!hasBonus(Selector::sourceType(Bonus::HERO_BASE_SKILL)))
	{
		for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
		{
			pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(g), type->heroClass->primarySkillInitial[g]);
		}
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<SecondarySkill,ui8>(SecondarySkill::DEFAULT, -1)) //set secondary skills to default
		secSkills = type->secSkillsInit;
	if (!name.length())
		name = type->name;

	if (sex == 0xFF)//sex is default
		sex = type->sex;

	setFormation(false);
	if (!stacksCount()) //standard army//initial army
	{
		initArmy();
	}
	assert(validTypes());

	if(exp == 0xffffffff)
	{
		initExp();
	}
	else
	{
		levelUpAutomatically();
	}

	if (VLC->modh->modules.COMMANDERS && !commander)
	{
		commander = new CCommanderInstance(type->heroClass->commander->idNumber);
		commander->setArmyObj (castToArmyObj()); //TODO: separate function for setting commanders
		commander->giveStackExp (exp); //after our exp is set
	}

	if (mana < 0)
		mana = manaLimit();
}

void CGHeroInstance::initArmy(IArmyDescriptor *dst /*= nullptr*/)
{
	if(!dst)
		dst = this;

	int howManyStacks = 0; //how many stacks will hero receives <1 - 3>
	int pom = cb->gameState()->getRandomGenerator().nextInt(99);
	int warMachinesGiven = 0;

	if(pom < 9)
		howManyStacks = 1;
	else if(pom < 79)
		howManyStacks = 2;
	else
		howManyStacks = 3;

	vstd::amin(howManyStacks, type->initialArmy.size());

	for(int stackNo=0; stackNo < howManyStacks; stackNo++)
	{
		auto & stack = type->initialArmy[stackNo];

		int count = cb->gameState()->getRandomGenerator().nextInt(stack.minAmount, stack.maxAmount);

		if(stack.creature >= CreatureID::CATAPULT &&
		   stack.creature <= CreatureID::ARROW_TOWERS) //war machine
		{
			warMachinesGiven++;
			if(dst != this)
				continue;

			int slot = -1;
			ArtifactID aid = ArtifactID::NONE;
			switch (stack.creature)
			{
			case CreatureID::CATAPULT:
				slot = ArtifactPosition::MACH4;
				aid = ArtifactID::CATAPULT;
				break;
			default:
				aid = CArtHandler::creatureToMachineID(stack.creature);
				slot = 9 + aid;
				break;
			}
			auto convSlot = ArtifactPosition(slot);
			if(!getArt(convSlot))
				putArtifact(convSlot, CArtifactInstance::createNewArtifactInstance(aid));
			else
                logGlobal->warnStream() << "Hero " << name << " already has artifact at " << slot << ", omitting giving " << aid;
		}
		else
			dst->setCreature(SlotID(stackNo-warMachinesGiven), stack.creature, count);
	}
}

CGHeroInstance::~CGHeroInstance()
{
	commander.dellNull();
}

bool CGHeroInstance::needsLastStack() const
{
	return true;
}

void CGHeroInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(h == this) return; //exclude potential self-visiting

	if (ID == Obj::HERO)
	{
		if( cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner)) //our or ally hero
		{
			//exchange
			cb->heroExchange(h->id, id);
		}
		else //battle
		{
			if(visitedTown) //we're in town
				visitedTown->onHeroVisit(h); //town will handle attacking
			else
				cb->startBattleI(h,	this);
		}
	}
	else if(ID == Obj::PRISON)
	{
		int txt_id;

		if (cb->getHeroCount(h->tempOwner, false) < VLC->modh->settings.MAX_HEROES_ON_MAP_PER_PLAYER)//GameConstants::MAX_HEROES_PER_PLAYER) //free hero slot
		{
			cb->changeObjPos(id,pos+int3(1,0,0),0);
			//update hero parameters
			SetMovePoints smp;
			smp.hid = id;
			smp.val = maxMovePoints (true); //TODO: hota prison on water?
			cb->setMovePoints (&smp);
			cb->setManaPoints (id, manaLimit());

			cb->setObjProperty(id, ObjProperty::ID, Obj::HERO); //set ID to 34
			cb->giveHero(id,h->tempOwner); //recreates def and adds hero to player

			txt_id = 102;
		}
		else //already 8 wandering heroes
		{
			txt_id = 103;
		}

		showInfoDialog(h,txt_id,soundBase::ROGUE);
	}
}

std::string CGHeroInstance::getObjectName() const
{
	if(ID != Obj::PRISON)
	{
		std::string hoverName = VLC->generaltexth->allTexts[15];
		boost::algorithm::replace_first(hoverName,"%s",name);
		boost::algorithm::replace_first(hoverName,"%s", type->heroClass->name);
		return hoverName;
	}
	else
		return CGObjectInstance::getObjectName();
}

const std::string & CGHeroInstance::getBiography() const
{
	if (biography.length())
		return biography;
	return type->biography;
}

ui8 CGHeroInstance::maxlevelsToMagicSchool() const
{
	return type->heroClass->isMagicHero() ? 3 : 4;
}
ui8 CGHeroInstance::maxlevelsToWisdom() const
{
	return type->heroClass->isMagicHero() ? 3 : 6;
}

void CGHeroInstance::SecondarySkillsInfo::resetMagicSchoolCounter()
{
	magicSchoolCounter = 1;
}
void CGHeroInstance::SecondarySkillsInfo::resetWisdomCounter()
{
	wisdomCounter = 1;
}

void CGHeroInstance::initObj()
{
	blockVisit = true;
	auto  hs = new HeroSpecial();
	hs->setNodeType(CBonusSystemNode::SPECIALTY);
	attachTo(hs); //do we ever need to detach it?

	if(!type)
		initHero(); //TODO: set up everything for prison before specialties are configured

	skillsInfo.rand.setSeed(cb->gameState()->getRandomGenerator().nextInt());
	skillsInfo.resetMagicSchoolCounter();
	skillsInfo.resetWisdomCounter();

	if (ID != Obj::PRISON)
	{
		auto customApp = VLC->objtypeh->getHandlerFor(ID, type->heroClass->id)->getOverride(cb->gameState()->getTile(visitablePos())->terType, this);
		if (customApp)
			appearance = customApp.get();
	}

	for(const auto &spec : type->spec) //TODO: unfity with bonus system
	{
		auto bonus = new Bonus();
		bonus->val = spec.val;
		bonus->sid = id.getNum(); //from the hero, specialty has no unique id
		bonus->duration = Bonus::PERMANENT;
		bonus->source = Bonus::HERO_SPECIAL;
		switch (spec.type)
		{
			case 1:// creature specialty
				{
					hs->growsWithLevel = true;

					const CCreature &specCreature = *VLC->creh->creatures[spec.additionalinfo]; //creature in which we have specialty

					//int creLevel = specCreature.level;
					//if(!creLevel)
					//{
					//	if(spec.additionalinfo == 146)
					//		creLevel = 5; //treat ballista as 5-level
					//	else
					//	{
					//        logGlobal->warnStream() << "Warning: unknown level of " << specCreature.namePl;
					//		continue;
					//	}
					//}

					//bonus->additionalInfo = spec.additionalinfo; //creature id, should not be used again - this works only with limiter
					bonus->limiter.reset(new CCreatureTypeLimiter (specCreature, true)); //with upgrades
					bonus->type = Bonus::PRIMARY_SKILL;
					bonus->valType = Bonus::ADDITIVE_VALUE;

					bonus->subtype = PrimarySkill::ATTACK;
					hs->addNewBonus(bonus);

					bonus = new Bonus(*bonus);
					bonus->subtype = PrimarySkill::DEFENSE;
					hs->addNewBonus(bonus);
					//values will be calculated later

					bonus = new Bonus(*bonus);
					bonus->type = Bonus::STACKS_SPEED;
					bonus->val = 1; //+1 speed
					hs->addNewBonus(bonus);
				}
				break;
			case 2://secondary skill
				hs->growsWithLevel = true;
				bonus->type = Bonus::SPECIAL_SECONDARY_SKILL; //needs to be recalculated with level, based on this value
				bonus->valType = Bonus::BASE_NUMBER; // to receive nonzero value
				bonus->subtype = spec.subtype; //skill id
				bonus->val = spec.val; //value per level, in percent
				hs->addNewBonus(bonus);
				bonus = new Bonus(*bonus);

				switch (spec.additionalinfo)
				{
					case 0: //normal
						bonus->valType = Bonus::PERCENT_TO_BASE;
						break;
					case 1: //when it's navigation or there's no 'base' at all
						bonus->valType = Bonus::PERCENT_TO_ALL;
						break;
				}
				bonus->type = Bonus::SECONDARY_SKILL_PREMY; //value will be calculated later
				hs->addNewBonus(bonus);
				break;
			case 3://spell damage bonus, level dependent but calculated elsewhere
				bonus->type = Bonus::SPECIAL_SPELL_LEV;
				bonus->subtype = spec.subtype;
				hs->addNewBonus(bonus);
				break;
			case 4://creature stat boost
				switch (spec.subtype)
				{
					case 1://attack
						bonus->type = Bonus::PRIMARY_SKILL;
						bonus->subtype = PrimarySkill::ATTACK;
						break;
					case 2://defense
						bonus->type = Bonus::PRIMARY_SKILL;
						bonus->subtype = PrimarySkill::DEFENSE;
						break;
					case 3:
						bonus->type = Bonus::CREATURE_DAMAGE;
						bonus->subtype = 0; //both min and max
						break;
					case 4://hp
						bonus->type = Bonus::STACK_HEALTH;
						break;
					case 5:
						bonus->type = Bonus::STACKS_SPEED;
						break;
					default:
						continue;
				}
				bonus->additionalInfo = spec.additionalinfo; //creature id
				bonus->valType = Bonus::ADDITIVE_VALUE;
				bonus->limiter.reset(new CCreatureTypeLimiter (*VLC->creh->creatures[spec.additionalinfo], true));
				hs->addNewBonus(bonus);
				break;
			case 5://spell damage bonus in percent
				bonus->type = Bonus::SPECIFIC_SPELL_DAMAGE;
				bonus->valType = Bonus::BASE_NUMBER; // current spell system is screwed
				bonus->subtype = spec.subtype; //spell id
				hs->addNewBonus(bonus);
				break;
			case 6://damage bonus for bless (Adela)
				bonus->type = Bonus::SPECIAL_BLESS_DAMAGE;
				bonus->subtype = spec.subtype; //spell id if you ever wanted to use it otherwise
				bonus->additionalInfo = spec.additionalinfo; //damage factor
				hs->addNewBonus(bonus);
				break;
			case 7://maxed mastery for spell
				bonus->type = Bonus::MAXED_SPELL;
				bonus->subtype = spec.subtype; //spell i
				hs->addNewBonus(bonus);
				break;
			case 8://peculiar spells - enchantments
				bonus->type = Bonus::SPECIAL_PECULIAR_ENCHANT;
				bonus->subtype = spec.subtype; //spell id
				bonus->additionalInfo = spec.additionalinfo;//0, 1 for Coronius
				hs->addNewBonus(bonus);
				break;
			case 9://upgrade creatures
			{
				const auto &creatures = VLC->creh->creatures;
				bonus->type = Bonus::SPECIAL_UPGRADE;
				bonus->subtype = spec.subtype; //base id
				bonus->additionalInfo = spec.additionalinfo; //target id
				hs->addNewBonus(bonus);
				bonus = new Bonus(*bonus);

				for(auto cre_id : creatures[spec.subtype]->upgrades)
				{
					bonus->subtype = cre_id; //propagate for regular upgrades of base creature
					hs->addNewBonus(bonus);
					bonus = new Bonus(*bonus);
				}
				vstd::clear_pointer(bonus);
				break;
			}
			case 10://resource generation
				bonus->type = Bonus::GENERATE_RESOURCE;
				bonus->subtype = spec.subtype;
				hs->addNewBonus(bonus);
				break;
			case 11://starting skill with mastery (Adrienne)
				setSecSkillLevel(SecondarySkill(spec.val), spec.additionalinfo, true);
				break;
			case 12://army speed
				bonus->type = Bonus::STACKS_SPEED;
				hs->addNewBonus(bonus);
				break;
			case 13://Dragon bonuses (Mutare)
				bonus->type = Bonus::PRIMARY_SKILL;
				bonus->valType = Bonus::ADDITIVE_VALUE;
				switch (spec.subtype)
				{
					case 1:
						bonus->subtype = PrimarySkill::ATTACK;
						break;
					case 2:
						bonus->subtype = PrimarySkill::DEFENSE;
						break;
				}
				bonus->limiter.reset(new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE));
				hs->addNewBonus(bonus);
				break;
			default:
                logGlobal->warnStream() << "Unexpected hero specialty " << type;
		}
	}
	specialty.push_back(hs); //will it work?

	for (auto hs2 : type->specialty) //copy active (probably growing) bonuses from hero prootype to hero object
	{
		auto  hs = new HeroSpecial();
		attachTo(hs); //do we ever need to detach it?

		hs->setNodeType(CBonusSystemNode::SPECIALTY);
		for (auto bonus : hs2.bonuses)
		{
			hs->addNewBonus (bonus);
		}
		hs->growsWithLevel = hs2.growsWithLevel;

		specialty.push_back(hs); //will it work?
	}

	//initialize bonuses
	recreateSecondarySkillsBonuses();
	Updatespecialty();

	mana = manaLimit(); //after all bonuses are taken into account, make sure this line is the last one
	type->name = name;
}
void CGHeroInstance::Updatespecialty() //TODO: calculate special value of bonuses on-the-fly?
{
	for (auto hs : specialty)
	{
		if (hs->growsWithLevel)
		{
			//const auto &creatures = VLC->creh->creatures;

			for(Bonus * b : hs->getBonusList())
			{
				switch (b->type)
				{
					case Bonus::SECONDARY_SKILL_PREMY:
						b->val = (hs->valOfBonuses(Bonus::SPECIAL_SECONDARY_SKILL, b->subtype) * level);
						break; //use only hero skills as bonuses to avoid feedback loop
					case Bonus::PRIMARY_SKILL: //for creatures, that is
					{
						const CCreature * cre = nullptr;
						int creLevel = 0;
						if (auto creatureLimiter = std::dynamic_pointer_cast<CCreatureTypeLimiter>(b->limiter)) //TODO: more general eveluation of bonuses?
						{
							cre = creatureLimiter->creature;
							creLevel = cre->level;
							if (!creLevel)
							{
								creLevel = 5; //treat ballista as tier 5
							}
						}
						else //no creature found, can't calculate value
						{
                            logGlobal->warnStream() << "Primary skill specialty growth supported only with creature type limiters";
							break;
						}

						double primSkillModifier = (int)(level / creLevel) / 20.0;
						int param;
						switch (b->subtype)
						{
							case PrimarySkill::ATTACK:
								param = cre->Attack();
								break;
							case PrimarySkill::DEFENSE:
								param = cre->Defense();
								break;
							default:
								continue;
						}
						b->val = ceil(param * (1 + primSkillModifier)) - param; //yep, overcomplicated but matches original
						break;
					}
				}
			}
		}
	}
}

void CGHeroInstance::recreateSecondarySkillsBonuses()
{
	auto secondarySkillsBonuses = getBonuses(Selector::sourceType(Bonus::SECONDARY_SKILL));
	for(auto bonus : *secondarySkillsBonuses)
		removeBonus(bonus);

	for(auto skill_info : secSkills)
		updateSkill(SecondarySkill(skill_info.first), skill_info.second);
}

void CGHeroInstance::updateSkill(SecondarySkill which, int val)
{
	if(which == SecondarySkill::LEADERSHIP || which == SecondarySkill::LUCK)
	{ //luck-> VLC->generaltexth->arraytxt[73+luckSkill]; VLC->generaltexth->arraytxt[104+moraleSkill]
		bool luck = which == SecondarySkill::LUCK;
		Bonus::BonusType type[] = {Bonus::MORALE, Bonus::LUCK};

		Bonus *b = getBonusLocalFirst(Selector::type(type[luck]).And(Selector::sourceType(Bonus::SECONDARY_SKILL)));
		if(!b)
		{
			b = new Bonus(Bonus::PERMANENT, type[luck], Bonus::SECONDARY_SKILL, +val, which, which, Bonus::BASE_NUMBER);
			addNewBonus(b);
		}
		else
			b->val = +val;
	}
	else if(which == SecondarySkill::DIPLOMACY) //surrender discount: 20% per level
	{

		if(Bonus *b = getBonusLocalFirst(Selector::type(Bonus::SURRENDER_DISCOUNT).And(Selector::sourceType(Bonus::SECONDARY_SKILL))))
			b->val = +val;
		else
			addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::SURRENDER_DISCOUNT, Bonus::SECONDARY_SKILL, val * 20, which));
	}

	int skillVal = 0;
	switch (which)
	{
	case SecondarySkill::ARCHERY:
		switch (val)
		{
		case 1:
			skillVal = 10; break;
		case 2:
			skillVal = 25; break;
		case 3:
			skillVal = 50; break;
		}
		break;
	case SecondarySkill::LOGISTICS:
		skillVal = 10 * val; break;
	case SecondarySkill::NAVIGATION:
		skillVal = 50 * val; break;
	case SecondarySkill::MYSTICISM:
		skillVal = val; break;
	case SecondarySkill::EAGLE_EYE:
		skillVal = 30 + 10 * val; break;
	case SecondarySkill::NECROMANCY:
		skillVal = 10 * val; break;
	case SecondarySkill::LEARNING:
		skillVal = 5 * val; break;
	case SecondarySkill::OFFENCE:
		skillVal = 10 * val; break;
	case SecondarySkill::ARMORER:
		skillVal = 5 * val; break;
	case SecondarySkill::INTELLIGENCE:
		skillVal = 25 << (val-1); break;
	case SecondarySkill::SORCERY:
		skillVal = 5 * val; break;
	case SecondarySkill::RESISTANCE:
		skillVal = 5 << (val-1); break;
	case SecondarySkill::FIRST_AID:
		skillVal = 25 + 25*val; break;
	case SecondarySkill::ESTATES:
		skillVal = 125 << (val-1); break;
	}


	Bonus::ValueType skillValType = skillVal ? Bonus::BASE_NUMBER : Bonus::INDEPENDENT_MIN;
	if(Bonus * b = getExportedBonusList().getFirst(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, which)
										.And(Selector::sourceType(Bonus::SECONDARY_SKILL)))) //only local hero bonus
	{
		b->val = skillVal;
		b->valType = skillValType;
	}
	else
	{
		auto bonus = new Bonus(Bonus::PERMANENT, Bonus::SECONDARY_SKILL_PREMY, Bonus::SECONDARY_SKILL, skillVal, id.getNum(), which, skillValType);
		bonus->source = Bonus::SECONDARY_SKILL;
		addNewBonus(bonus);
	}

	CBonusSystemNode::treeHasChanged();
}
void CGHeroInstance::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::PRIMARY_STACK_COUNT)
		setStackCount(SlotID(0), val);
}

double CGHeroInstance::getFightingStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(PrimarySkill::ATTACK)) * (1.0 + 0.05*getPrimSkillLevel(PrimarySkill::DEFENSE)));
}

double CGHeroInstance::getMagicStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(PrimarySkill::KNOWLEDGE)) * (1.0 + 0.05*getPrimSkillLevel(PrimarySkill::SPELL_POWER)));
}

double CGHeroInstance::getHeroStrength() const
{
	return sqrt(pow(getFightingStrength(), 2.0) * pow(getMagicStrength(), 2.0));
}

ui64 CGHeroInstance::getTotalStrength() const
{
	double ret = getFightingStrength() * getArmyStrength();
	return (ui64) ret;
}

TExpType CGHeroInstance::calculateXp(TExpType exp) const
{
	return exp * (100 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::LEARNING))/100.0;
}

ui8 CGHeroInstance::getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool) const
{
	si16 skill = -1; //skill level

	spell->forEachSchool([&, this](const SpellSchoolInfo & cnf, bool & stop)
	{
		int thisSchool = std::max<int>(getSecSkillLevel(cnf.skill),	valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 1 << ((ui8)cnf.id))); //FIXME: Bonus shouldn't be additive (Witchking Artifacts : Crown of Skies)
		if(thisSchool > skill)
		{
			skill = thisSchool;
			if(outSelectedSchool)
				*outSelectedSchool = (ui8)cnf.id;
		}
	});

	vstd::amax(skill, valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0)); //any school bonus
	vstd::amax(skill, valOfBonuses(Bonus::SPELL, spell->id.toEnum())); //given by artifact or other effect

	vstd::amax(skill, 0); //in case we don't know any school
	vstd::amin(skill, 3);
	return skill;
}

ui32 CGHeroInstance::getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const
{
	//applying sorcery secondary skill

	base *= (100.0 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::SORCERY)) / 100.0;
	base *= (100.0 + valOfBonuses(Bonus::SPELL_DAMAGE) + valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, spell->id.toEnum())) / 100.0;

	spell->forEachSchool([&base, this](const SpellSchoolInfo & cnf, bool & stop)
	{
		base *= (100.0 + valOfBonuses(cnf.damagePremyBonus)) / 100.0;
		stop = true; //only bonus from one school is used
	});

	if (affectedStack && affectedStack->getCreature()->level) //Hero specials like Solmyr, Deemer
		base *= (100. + ((valOfBonuses(Bonus::SPECIAL_SPELL_LEV,  spell->id.toEnum()) * level) / affectedStack->getCreature()->level)) / 100.0;

	return base;
}

int CGHeroInstance::getEffectLevel(const CSpell * spell) const
{
	if(hasBonusOfType(Bonus::MAXED_SPELL, spell->id))
		return 3;//todo: recheck specialty from where this bonus is. possible bug
	else
		return getSpellSchoolLevel(spell);
}

int CGHeroInstance::getEffectPower(const CSpell * spell) const
{
	return getPrimSkillLevel(PrimarySkill::SPELL_POWER);
}

int CGHeroInstance::getEnchantPower(const CSpell * spell) const
{
	return getPrimSkillLevel(PrimarySkill::SPELL_POWER) + valOfBonuses(Bonus::SPELL_DURATION);
}

int CGHeroInstance::getEffectValue(const CSpell * spell) const
{
	return 0;
}

const PlayerColor CGHeroInstance::getOwner() const
{
	return tempOwner;
}

bool CGHeroInstance::canCastThisSpell(const CSpell * spell) const
{
	if(nullptr == getArt(ArtifactPosition::SPELLBOOK))
		return false;

	const bool isAllowed = IObjectInterface::cb->isAllowed(0, spell->id);

	const bool inSpellBook = vstd::contains(spells, spell->id);
	const bool specificBonus = hasBonusOfType(Bonus::SPELL, spell->id);

	bool schoolBonus = false;

	spell->forEachSchool([this, &schoolBonus](const SpellSchoolInfo & cnf, bool & stop)
	{
		if(hasBonusOfType(cnf.knoledgeBonus))
		{
			schoolBonus = stop = true;
		}
	});

	const bool levelBonus = hasBonusOfType(Bonus::SPELLS_OF_LEVEL, spell->level);

    if (spell->isSpecialSpell())
    {
        if (inSpellBook)
        {//hero has this spell in spellbook
            logGlobal->errorStream() << "Special spell " << spell->name << "in spellbook.";
        }
        return specificBonus;
    }
    else if(!isAllowed)
    {
        if (inSpellBook)
        {//hero has this spell in spellbook
            logGlobal->errorStream() << "Banned spell " << spell->name << " in spellbook.";
        }
        return specificBonus || schoolBonus || levelBonus;
    }
    else
    {
		return inSpellBook || schoolBonus || specificBonus || levelBonus;
    }
}

/**
 * Calculates what creatures and how many to be raised from a battle.
 * @param battleResult The results of the battle.
 * @return Returns a pair with the first value indicating the ID of the creature
 * type and second value the amount. Both values are returned as -1 if necromancy
 * could not be applied.
 */
CStackBasicDescriptor CGHeroInstance::calculateNecromancy (const BattleResult &battleResult) const
{
	const ui8 necromancyLevel = getSecSkillLevel(SecondarySkill::NECROMANCY);

	// Hero knows necromancy or has Necromancer Cloak
	if (necromancyLevel > 0 || hasBonusOfType(Bonus::IMPROVED_NECROMANCY))
	{
		double necromancySkill = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::NECROMANCY)/100.0;
		vstd::amin(necromancySkill, 1.0); //it's impossible to raise more creatures than all...
		const std::map<ui32,si32> &casualties = battleResult.casualties[!battleResult.winner];
		ui32 raisedUnits = 0;

		// Figure out what to raise and how many.
		const CreatureID creatureTypes[] = {CreatureID::SKELETON, CreatureID::WALKING_DEAD, CreatureID::WIGHTS, CreatureID::LICHES};
		const bool improvedNecromancy = hasBonusOfType(Bonus::IMPROVED_NECROMANCY);
		const CCreature *raisedUnitType = VLC->creh->creatures[creatureTypes[improvedNecromancy ? necromancyLevel : 0]];
		const ui32 raisedUnitHP = raisedUnitType->valOfBonuses(Bonus::STACK_HEALTH);

		//calculate creatures raised from each defeated stack
		for (auto & casualtie : casualties)
		{
			// Get lost enemy hit points convertible to units.
			CCreature * c = VLC->creh->creatures[casualtie.first];

			const ui32 raisedHP = c->valOfBonuses(Bonus::STACK_HEALTH) * casualtie.second * necromancySkill;
			raisedUnits += std::min<ui32>(raisedHP / raisedUnitHP, casualtie.second * necromancySkill); //limit to % of HP and % of original stack count
		}

		// Make room for new units.
		SlotID slot = getSlotFor(raisedUnitType->idNumber);
		if (slot == SlotID())
		{
			// If there's no room for unit, try it's upgraded version 2/3rds the size.
			raisedUnitType = VLC->creh->creatures[*raisedUnitType->upgrades.begin()];
			raisedUnits = (raisedUnits*2)/3;

			slot = getSlotFor(raisedUnitType->idNumber);
		}
		if (raisedUnits <= 0)
			raisedUnits = 1;

		return CStackBasicDescriptor(raisedUnitType->idNumber, raisedUnits);
	}

	return CStackBasicDescriptor();
}

/**
 * Show the necromancy dialog with information about units raised.
 * @param raisedStack Pair where the first element represents ID of the raised creature
 * and the second element the amount.
 */
void CGHeroInstance::showNecromancyDialog(const CStackBasicDescriptor &raisedStack) const
{
	InfoWindow iw;
	iw.soundID = soundBase::pickup01 + cb->gameState()->getRandomGenerator().nextInt(6);
	iw.player = tempOwner;
	iw.components.push_back(Component(raisedStack));

	if (raisedStack.count > 1) // Practicing the dark arts of necromancy, ... (plural)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 145);
		iw.text.addReplacement(raisedStack.count);
	}
	else // Practicing the dark arts of necromancy, ... (singular)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 146);
	}
	iw.text.addReplacement(raisedStack);

	cb->showInfoDialog(&iw);
}
/*
int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}*/

int CGHeroInstance::getSightRadius() const
{
	return 5 + getSecSkillLevel(SecondarySkill::SCOUTING) + valOfBonuses(Bonus::SIGHT_RADIOUS); //default + scouting
}

si32 CGHeroInstance::manaRegain() const
{
	if (hasBonusOfType(Bonus::FULL_MANA_REGENERATION))
		return manaLimit();

	return 1 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 8) + valOfBonuses(Bonus::MANA_REGENERATION); //1 + Mysticism level
}

si32 CGHeroInstance::getManaNewTurn() const
{
	if(visitedTown && visitedTown->hasBuilt(BuildingID::MAGES_GUILD_1))
	{
		//if hero starts turn in town with mage guild - restore all mana
		return std::max(mana, manaLimit());
	}
	si32 res = mana + manaRegain();
	res = std::min(res, manaLimit());
	res = std::max(res, mana);
	res = std::max(res, 0);
	return res;
}

// /**
//  * Places an artifact in hero's backpack. If it's a big artifact equips it
//  * or discards it if it cannot be equipped.
//  */
// void CGHeroInstance::giveArtifact (ui32 aid) //use only for fixed artifacts
// {
// 	CArtifact * const artifact = VLC->arth->artifacts[aid]; //pointer to constant object
// 	CArtifactInstance *ai = CArtifactInstance::createNewArtifactInstance(artifact);
// 	ai->putAt(this, ai->firstAvailableSlot(this));
// }

int CGHeroInstance::getBoatType() const
{
	switch(type->heroClass->getAlignment())
	{
	case EAlignment::GOOD:
		return 1;
	case EAlignment::EVIL:
		return 0;
	case EAlignment::NEUTRAL:
		return 2;
	default:
		throw std::runtime_error("Wrong alignment!");
	}
}

void CGHeroInstance::getOutOffsets(std::vector<int3> &offsets) const
{
	// FIXME: Offsets need to be fixed once we get rid of convertPosition
	// Check issue 515 for details
	offsets =
	{
		int3(-1,1,0), int3(-1,-1,0), int3(-2,0,0), int3(0,0,0), int3(0,1,0), int3(-2,1,0), int3(0,-1,0), int3(-2,-1,0)
	};
}

int CGHeroInstance::getSpellCost(const CSpell *sp) const
{
	return sp->getCost(getSpellSchoolLevel(sp));
}

void CGHeroInstance::pushPrimSkill( PrimarySkill::PrimarySkill which, int val )
{
	assert(!hasBonus(Selector::typeSubtype(Bonus::PRIMARY_SKILL, which)
						.And(Selector::sourceType(Bonus::HERO_BASE_SKILL))));
	addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::HERO_BASE_SKILL, val, id.getNum(), which));
}

EAlignment::EAlignment CGHeroInstance::getAlignment() const
{
	return type->heroClass->getAlignment();
}

void CGHeroInstance::initExp()
{
	exp = cb->gameState()->getRandomGenerator().nextInt(40, 89);
}

std::string CGHeroInstance::nodeName() const
{
	return "Hero " + name;
}

void CGHeroInstance::putArtifact(ArtifactPosition pos, CArtifactInstance *art)
{
	assert(!getArt(pos));
	art->putAt(ArtifactLocation(this, pos));
}

void CGHeroInstance::putInBackpack(CArtifactInstance *art)
{
	putArtifact(art->firstBackpackSlot(this), art);
}

bool CGHeroInstance::hasSpellbook() const
{
	return getArt(ArtifactPosition::SPELLBOOK);
}

void CGHeroInstance::deserializationFix()
{
	artDeserializationFix(this);

	for (auto hs : specialty)
	{
		attachTo (hs);
	}
}

CBonusSystemNode * CGHeroInstance::whereShouldBeAttached(CGameState *gs)
{
	if(visitedTown)
	{
		if(inTownGarrison)
			return visitedTown;
		else
			return &visitedTown->townAndVis;
	}
	else
		return CArmedInstance::whereShouldBeAttached(gs);
}

int CGHeroInstance::movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark /*= false*/, const TurnInfo * ti) const
{
	if(!ti)
		ti = new TurnInfo(this);

	int mp1 = ti->getMaxMovePoints(disembark ? EPathfindingLayer::LAND : EPathfindingLayer::SAIL);
	int mp2 = ti->getMaxMovePoints(disembark ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND);
	if(ti->hasBonusOfType(Bonus::FREE_SHIP_BOARDING))
		return (MPsBefore - basicCost) * static_cast<double>(mp1) / mp2;

	return 0; //take all MPs otherwise
}

EDiggingStatus CGHeroInstance::diggingStatus() const
{
	if(movement < maxMovePoints(true))
		return EDiggingStatus::LACK_OF_MOVEMENT;

	return cb->getTile(getPosition(false))->getDiggingStatus();
}

ArtBearer::ArtBearer CGHeroInstance::bearerType() const
{
	return ArtBearer::HERO;
}

std::vector<SecondarySkill> CGHeroInstance::getLevelUpProposedSecondarySkills() const
{
	std::vector<SecondarySkill> obligatorySkills; //hero is offered magic school or wisdom if possible
	if (!skillsInfo.wisdomCounter)
	{
		if (cb->isAllowed(2, SecondarySkill::WISDOM) && !getSecSkillLevel(SecondarySkill::WISDOM))
			obligatorySkills.push_back(SecondarySkill::WISDOM);
	}
	if (!skillsInfo.magicSchoolCounter)
	{
		std::vector<SecondarySkill> ss =
		{
			SecondarySkill::FIRE_MAGIC, SecondarySkill::AIR_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC
		};

		std::shuffle(ss.begin(), ss.end(), skillsInfo.rand.getStdGenerator());

		for (auto skill : ss)
		{
			if (cb->isAllowed(2, skill) && !getSecSkillLevel(skill)) //only schools hero doesn't know yet
			{
				obligatorySkills.push_back(skill);
				break; //only one
			}
		}
	}

	std::vector<SecondarySkill> skills;
	//picking sec. skills for choice
	std::set<SecondarySkill> basicAndAdv, expert, none;
	for(int i=0;i<GameConstants::SKILL_QUANTITY;i++)
		if (cb->isAllowed(2,i))
			none.insert(SecondarySkill(i));

	for(auto & elem : secSkills)
	{
		if(elem.second < SecSkillLevel::EXPERT)
			basicAndAdv.insert(elem.first);
		else
			expert.insert(elem.first);
		none.erase(elem.first);
	}
	for (auto s : obligatorySkills) //don't duplicate them
	{
		none.erase (s);
		basicAndAdv.erase (s);
		expert.erase (s);
	}

	//first offered skill:
	// 1) give obligatory skill
	// 2) give any other new skill
	// 3) upgrade existing
	if (canLearnSkill() && obligatorySkills.size() > 0)
	{
		skills.push_back (obligatorySkills[0]);
	}
	else if(none.size() && canLearnSkill()) //hero have free skill slot
	{
		skills.push_back(type->heroClass->chooseSecSkill(none, skillsInfo.rand)); //new skill
		none.erase(skills.back());
	}
	else if(!basicAndAdv.empty())
	{
		skills.push_back(type->heroClass->chooseSecSkill(basicAndAdv, skillsInfo.rand)); //upgrade existing
		basicAndAdv.erase(skills.back());
	}

	//second offered skill:
	//1) upgrade existing
	//2) give obligatory skill
	//3) give any other new skill
	if(!basicAndAdv.empty())
	{
		SecondarySkill s = type->heroClass->chooseSecSkill(basicAndAdv, skillsInfo.rand);//upgrade existing
		skills.push_back(s);
		basicAndAdv.erase(s);
	}
	else if (canLearnSkill() && obligatorySkills.size() > 1)
	{
		skills.push_back (obligatorySkills[1]);
	}
	else if(none.size() && canLearnSkill())
	{
		skills.push_back(type->heroClass->chooseSecSkill(none, skillsInfo.rand)); //give new skill
		none.erase(skills.back());
	}

	if (skills.size() == 2) // Fix for #1868 to avoid changing logic (possibly causing bugs in process)
		std::swap(skills[0], skills[1]);
	return skills;
}

PrimarySkill::PrimarySkill CGHeroInstance::nextPrimarySkill() const
{
	assert(gainsLevel());
	int randomValue = cb->gameState()->getRandomGenerator().nextInt(99), pom = 0, primarySkill = 0;
	const auto & skillChances = (level > 9) ? type->heroClass->primarySkillLowLevel : type->heroClass->primarySkillHighLevel;

	for(; primarySkill < GameConstants::PRIMARY_SKILLS; ++primarySkill)
	{
		pom += skillChances[primarySkill];
		if(randomValue < pom)
		{
			break;
		}
	}

	logGlobal->traceStream() << "The hero gets the primary skill " << primarySkill << " with a probability of " << randomValue << "%.";
	return static_cast<PrimarySkill::PrimarySkill>(primarySkill);
}

boost::optional<SecondarySkill> CGHeroInstance::nextSecondarySkill() const
{
	assert(gainsLevel());

	boost::optional<SecondarySkill> chosenSecondarySkill;
	const auto proposedSecondarySkills = getLevelUpProposedSecondarySkills();
	if(!proposedSecondarySkills.empty())
	{
		std::vector<SecondarySkill> learnedSecondarySkills;
		for(auto secondarySkill : proposedSecondarySkills)
		{
			if(getSecSkillLevel(secondarySkill) > 0)
			{
				learnedSecondarySkills.push_back(secondarySkill);
			}
		}

		auto & rand = cb->gameState()->getRandomGenerator();
		if(learnedSecondarySkills.empty())
		{
			// there are only new skills to learn, so choose anyone of them
			chosenSecondarySkill = *RandomGeneratorUtil::nextItem(proposedSecondarySkills, rand);
		}
		else
		{
			// preferably upgrade a already learned secondary skill
			chosenSecondarySkill = *RandomGeneratorUtil::nextItem(learnedSecondarySkills, rand);
		}
	}
	return chosenSecondarySkill;
}

void CGHeroInstance::setPrimarySkill(PrimarySkill::PrimarySkill primarySkill, si64 value, ui8 abs)
{
	if(primarySkill < PrimarySkill::EXPERIENCE)
	{
		Bonus * skill = getBonusLocalFirst(Selector::type(Bonus::PRIMARY_SKILL)
											.And(Selector::subtype(primarySkill))
											.And(Selector::sourceType(Bonus::HERO_BASE_SKILL)));
		assert(skill);

		if(abs)
		{
			skill->val = value;
		}
		else
		{
			skill->val += value;
		}
		CBonusSystemNode::treeHasChanged();
	}
	else if(primarySkill == PrimarySkill::EXPERIENCE)
	{
		if(abs)
		{
			exp = value;
		}
		else
		{
			exp += value;
		}
	}
}

bool CGHeroInstance::gainsLevel() const
{
	return exp >= VLC->heroh->reqExp(level+1);
}

void CGHeroInstance::levelUp(std::vector<SecondarySkill> skills)
{
	++level;

	//deterministic secondary skills
	skillsInfo.magicSchoolCounter = (skillsInfo.magicSchoolCounter + 1) % maxlevelsToMagicSchool();
	skillsInfo.wisdomCounter = (skillsInfo.wisdomCounter + 1) % maxlevelsToWisdom();
	if(vstd::contains(skills, SecondarySkill::WISDOM))
	{
		skillsInfo.resetWisdomCounter();
	}

	SecondarySkill spellSchools[] = {
		SecondarySkill::FIRE_MAGIC, SecondarySkill::AIR_MAGIC, SecondarySkill::WATER_MAGIC, SecondarySkill::EARTH_MAGIC};
	for(auto skill : spellSchools)
	{
		if(vstd::contains(skills, skill))
		{
			skillsInfo.resetMagicSchoolCounter();
			break;
		}
	}

	//specialty
	Updatespecialty();
}

void CGHeroInstance::levelUpAutomatically()
{
	while(gainsLevel())
	{
		const auto primarySkill = nextPrimarySkill();
		setPrimarySkill(primarySkill, 1, false);

		auto proposedSecondarySkills = getLevelUpProposedSecondarySkills();

		const auto secondarySkill = nextSecondarySkill();
		if(secondarySkill)
		{
			setSecSkillLevel(*secondarySkill, 1, false);
		}

		//TODO why has the secondary skills to be passed to the method?
		levelUp(proposedSecondarySkills);
	}
}

bool CGHeroInstance::hasVisions(const CGObjectInstance * target, const int subtype) const
{
	//VISIONS spell support

	const std::string cached = boost::to_string((boost::format("type_%d__subtype_%d") % Bonus::VISIONS % subtype));

	const int visionsMultiplier = valOfBonuses(Selector::typeSubtype(Bonus::VISIONS,subtype), cached);

	int visionsRange =  visionsMultiplier * getPrimSkillLevel(PrimarySkill::SPELL_POWER);

	if (visionsMultiplier > 0)
		vstd::amax(visionsRange, 3); //minimum range is 3 tiles, but only if VISIONS bonus present

	const int distance = target->pos.dist2d(getPosition(false));

	//logGlobal->debug(boost::to_string(boost::format("Visions: dist %d, mult %d, range %d") % distance % visionsMultiplier % visionsRange));

	return (distance < visionsRange) && (target->pos.z == pos.z);
}

bool CGHeroInstance::isMissionCritical() const
{
	for(const TriggeredEvent & event : IObjectInterface::cb->getMapHeader()->triggeredEvents)
	{
		if(event.trigger.test([&](const EventCondition & condition)
		{
			if (condition.condition == EventCondition::CONTROL && condition.object)
			{
				auto hero = dynamic_cast<const CGHeroInstance*>(condition.object);
				return (hero != this);
			}
			else if(condition.condition == EventCondition::IS_HUMAN)
			{
				return true;
			}
			return false;
		}))
		{
			return true;
		}
	}
	return false;
}
