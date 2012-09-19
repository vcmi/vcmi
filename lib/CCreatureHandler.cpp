#include "StdInc.h"
#include "CCreatureHandler.h"

#include "CGeneralTextHandler.h"
#include "Filesystem/CResourceLoader.h"
#include "VCMI_Lib.h"
#include "CGameState.h"
#include "CTownHandler.h"
#include "CModHandler.h"

using namespace boost::assign;

/*
 * CCreatureHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCreatureHandler::CCreatureHandler()
{
	VLC->creh = this;

	// Set the faction alignments to the defaults:
	// Good: Castle, Rampart, Tower
	// Evil: Inferno, Necropolis, Dungeon
	// Neutral: Stronghold, Fortess, Conflux
	factionAlignments += 1, 1, 1, -1, -1, -1, 0, 0, 0;
	doubledCreatures +=  4, 14, 20, 28, 44, 60, 70, 72, 85, 86, 100, 104; //according to Strategija

	allCreatures.setDescription("All creatures");
	creaturesOfLevel[0].setDescription("Creatures of unnormalized tier");
	for(int i = 1; i < ARRAY_COUNT(creaturesOfLevel); i++)
		creaturesOfLevel[i].setDescription("Creatures of tier " + boost::lexical_cast<std::string>(i));
}

int CCreature::getQuantityID(const int & quantity)
{
	if (quantity<5)
		return 1;
	if (quantity<10)
		return 2;
	if (quantity<20)
		return 3;
	if (quantity<50)
		return 4;
	if (quantity<100)
		return 5;
	if (quantity<250)
		return 6;
	if (quantity<500)
		return 7;
	if (quantity<1000)
		return 8;
	return 9;
}

int CCreature::estimateCreatureCount(ui32 countID)
{
	static const int creature_count[] = { 0, 3, 8, 15, 35, 75, 175, 375, 750, 2500 };

	if (countID > 9)
		assert("Wrong countID!");

	return creature_count[countID];
}

bool CCreature::isDoubleWide() const
{
	return doubleWide;
}

bool CCreature::isFlying() const
{
	return hasBonusOfType(Bonus::FLYING);
}

bool CCreature::isShooting() const
{
	return hasBonusOfType(Bonus::SHOOTER);
}

bool CCreature::isUndead() const
{
	return hasBonusOfType(Bonus::UNDEAD);
}

/**
 * Determines if the creature is of a good alignment.
 * @return true if the creture is good, false otherwise.
 */
bool CCreature::isGood () const
{
	return VLC->creh->isGood(faction);
}

/**
 * Determines if the creature is of an evil alignment.
 * @return true if the creature is evil, false otherwise.
 */
bool CCreature::isEvil () const
{
	return VLC->creh->isEvil(faction);
}

si32 CCreature::maxAmount(const std::vector<si32> &res) const //how many creatures can be bought
{
	int ret = 2147483645;
	int resAmnt = std::min(res.size(),cost.size());
	for(int i=0;i<resAmnt;i++)
		if(cost[i])
			ret = std::min(ret,(int)(res[i]/cost[i]));
	return ret;
}

CCreature::CCreature()
{
	doubleWide = false;
	setNodeType(CBonusSystemNode::CREATURE);
}
void CCreature::addBonus(int val, int type, int subtype /*= -1*/)
{
	Bonus *added = new Bonus(Bonus::PERMANENT, type, Bonus::CREATURE_ABILITY, val, idNumber, subtype, Bonus::BASE_NUMBER);
	addNewBonus(added);
}
// void CCreature::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	out.insert (VLC->creh->globalEffects);
// }
bool CCreature::isMyUpgrade(const CCreature *anotherCre) const
{
	//TODO upgrade of upgrade?
	return vstd::contains(upgrades, anotherCre->idNumber);
}

bool CCreature::valid() const
{
	return this == VLC->creh->creatures[idNumber];
}

std::string CCreature::nodeName() const
{
	return "\"" + namePl + "\"";
}

bool CCreature::isItNativeTerrain(int terrain) const
{
	assert(vstd::contains(VLC->townh->factions, faction));
	//not good handler dependency
	return VLC->townh->factions[faction].nativeTerrain == terrain;
}

int readNumber(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadCreatures() and loadUnitAnimInfo()
{
	befi=i;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	std::string tmp = buf.substr(befi, i-befi);
	int ret = atoi(buf.substr(befi, i-befi).c_str());
	++i;
	return ret;
}

double readFloat(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadUnitAnimInfo()
{
	befi=i;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	std::string tmp = buf.substr(befi, i-befi);
	double ret = atof(buf.substr(befi, i-befi).c_str());
	++i;
	return ret;
}

/**
 * Determines if a faction is good.
 * @param ID of the faction.
 * @return true if the faction is good, false otherwise.
 */
bool CCreatureHandler::isGood (si8 faction) const
{
	return faction != -1 && factionAlignments[faction] == 1;
}

/**
 * Determines if a faction is evil.
 * @param ID of the faction.
 * @return true if the faction is evil, false otherwise.
 */
bool CCreatureHandler::isEvil (si8 faction) const
{
	return faction != -1 && factionAlignments[faction] == -1;
}

static void AddAbility(CCreature *cre, const JsonVector &ability_vec)
{
	Bonus *nsf = new Bonus();
	std::string type = ability_vec[0].String();

	std::map<std::string, int>::const_iterator it = bonusNameMap.find(type);

	if (it == bonusNameMap.end()) {
		if (type == "DOUBLE_WIDE")
			cre->doubleWide = true;
		else if (type == "ENEMY_MORALE_DECREASING") {
			cre->addBonus(-1, Bonus::MORALE);
			cre->getBonusList().back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
		}
		else if (type == "ENEMY_LUCK_DECREASING") {
			cre->addBonus(-1, Bonus::LUCK);
			cre->getBonusList().back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
		} else
			tlog1 << "Error: invalid ability type " << type << " in creatures.txt" << std::endl;

		return;
	}

	nsf->type = it->second;

	nsf->val = ability_vec[1].Float();
	nsf->subtype = ability_vec[2].Float();
	nsf->additionalInfo = ability_vec[3].Float();
	nsf->source = Bonus::CREATURE_ABILITY;
	nsf->sid = cre->idNumber;
	//nsf->duration = Bonus::ONE_BATTLE; //what the?
	nsf->duration = Bonus::PERMANENT;
	nsf->turnsRemain = 0;

	cre->addNewBonus(nsf);
}

static void RemoveAbility(CCreature *cre, const JsonNode &ability)
{
	std::string type = ability.String();

	std::map<std::string, int>::const_iterator it = bonusNameMap.find(type);

	if (it == bonusNameMap.end()) {
		if (type == "DOUBLE_WIDE")
			cre->doubleWide = false;
		else
			tlog1 << "Error: invalid ability type " << type << " in creatures.json" << std::endl;

		return;
	}

	int typeNo = it->second;

	Bonus::BonusType ecf = static_cast<Bonus::BonusType>(typeNo);

	Bonus *b = cre->getBonus(Selector::type(ecf));
	cre->removeBonus(b);
}

void CCreatureHandler::loadCreatures()
{
	tlog5 << "\t\tReading config/cr_abils.json and ZCRTRAIT.TXT" << std::endl;

	////////////reading ZCRTRAIT.TXT ///////////////////
	auto textFile = CResourceHandler::get()->loadData(ResourceID("DATA/ZCRTRAIT.TXT"));
	std::string buf((char*)textFile.first.get(), textFile.second);
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;

	while(i<buf.size())
	{
		CCreature &ncre = *new CCreature;
		ncre.idNumber = creatures.size();
		ncre.cost.resize(GameConstants::RESOURCE_QUANTITY);
		ncre.level=0;

		int befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.nameSing = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.namePl = buf.substr(befi, i-befi);
		++i;

		for(int v=0; v<7; ++v)
		{
			ncre.cost[v] = readNumber(befi, i, andame, buf);
		}
		ncre.fightValue = readNumber(befi, i, andame, buf);
		ncre.AIValue = readNumber(befi, i, andame, buf);
		ncre.growth = readNumber(befi, i, andame, buf);
		ncre.hordeGrowth = readNumber(befi, i, andame, buf);

		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::STACK_HEALTH);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::STACKS_SPEED);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::CREATURE_DAMAGE, 1);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::CREATURE_DAMAGE, 2);
		ncre.addBonus(readNumber(befi, i, andame, buf), Bonus::SHOTS);

		//spells - not used?
		readNumber(befi, i, andame, buf);
		ncre.ammMin = readNumber(befi, i, andame, buf);
		ncre.ammMax = readNumber(befi, i, andame, buf);

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.abilityText = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		ncre.abilityRefs = buf.substr(befi, i-befi);
		i+=2;
		if(true)
		{ //adding abilities from ZCRTRAIT.TXT
			if(boost::algorithm::find_first(ncre.abilityRefs, "DOUBLE_WIDE"))
				ncre.doubleWide = true;
			if(boost::algorithm::find_first(ncre.abilityRefs, "FLYING_ARMY"))
				ncre.addBonus(0, Bonus::FLYING);
			if(boost::algorithm::find_first(ncre.abilityRefs, "SHOOTING_ARMY"))
				ncre.addBonus(0, Bonus::SHOOTER);
			if(boost::algorithm::find_first(ncre.abilityRefs, "SIEGE_WEAPON"))
				ncre.addBonus(0, Bonus::SIEGE_WEAPON);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_two_attacks"))
				ncre.addBonus(1, Bonus::ADDITIONAL_ATTACK);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_free_attack"))
				ncre.addBonus(0, Bonus::BLOCKS_RETALIATION);
			if(boost::algorithm::find_first(ncre.abilityRefs, "IS_UNDEAD"))
				ncre.addBonus(0, Bonus::UNDEAD);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_melee_penalty"))
				ncre.addBonus(0, Bonus::NO_MELEE_PENALTY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_jousting"))
				ncre.addBonus(0, Bonus::JOUSTING);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_raises_morale"))
			{
				ncre.addBonus(+1, Bonus::MORALE);;
				ncre.getBonusList().back()->addPropagator(make_shared<CPropagatorNodeType>(CBonusSystemNode::HERO));
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_lowers_morale"))
			{
				ncre.addBonus(-1, Bonus::MORALE);;
				ncre.getBonusList().back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_1"))
				ncre.addBonus(0, Bonus::KING1);
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_2"))
				ncre.addBonus(0, Bonus::KING2);
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_3"))
				ncre.addBonus(0, Bonus::KING3);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_wall_penalty"))
				ncre.addBonus(0, Bonus::NO_WALL_PENALTY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "CATAPULT"))
				ncre.addBonus(0, Bonus::CATAPULT);
			if(boost::algorithm::find_first(ncre.abilityRefs, "MULTI_HEADED"))
				ncre.addBonus(0, Bonus::ATTACKS_ALL_ADJACENT);

			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_MIND_SPELLS"))
				ncre.addBonus(0, Bonus::MIND_IMMUNITY); //giants are immune to mind spells
			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_FIRE_SPELLS"))
				ncre.addBonus(0, Bonus::FIRE_IMMUNITY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "HAS_EXTENDED_ATTACK"))
				ncre.addBonus(0, Bonus::TWO_HEX_ATTACK_BREATH);;
		}

		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber = creatures.size();
			creatures.push_back(&ncre);
		}
	}

	// loading creatures properties
	tlog5 << "\t\tReading config/creatures.json" << std::endl;
	const JsonNode config(ResourceID("config/creatures.json"));

	BOOST_FOREACH(const JsonNode &creature, config["creatures"].Vector())
	{
		int creatureID = creature["id"].Float();
		const JsonNode *value;

		/* A creature can have several names. */
		BOOST_FOREACH(const JsonNode &name, creature["name"].Vector())
		{
			boost::assign::insert(nameToID)(name.String(), creatureID);
		}

		// Set various creature properties
		CCreature *c = creatures[creatureID];
		c->level = creature["level"].Float();
		c->faction = creature["faction"].Float();
		c->animDefName = creature["defname"].String();

		BOOST_FOREACH(const JsonNode &value, creature["upgrades"].Vector())
		{
			c->upgrades.insert(value.Float());
		}

		value = &creature["projectile_defname"];
		if (!value->isNull())
		{
			idToProjectile[creatureID] = value->String();

			value = &creature["projectile_spin"];
			idToProjectileSpin[creatureID] = value->Bool();
		}

		value = &creature["turret_shooter"];
		if (!value->isNull() && value->Bool())
			factionToTurretCreature[c->faction] = creatureID;

		value = &creature["ability_add"];
		if (!value->isNull()) {
			BOOST_FOREACH(const JsonNode &ability, value->Vector())
			{
				AddAbility(c, ability.Vector());
			}
		}

		value = &creature["ability_remove"];
		if (!value->isNull())
		{
			BOOST_FOREACH(const JsonNode &ability, value->Vector())
			{
				RemoveAbility(c, ability);
			}
		}
	}

	BOOST_FOREACH(const JsonNode &creature, config["unused_creatures"].Vector())
	{
		notUsedMonsters += creature.Float();
	}

	loadAnimationInfo();
	loadSoundsInfo();


	//reading creature ability names
	const JsonNode config2(ResourceID("config/bonusnames.json"));

	BOOST_FOREACH(const JsonNode &bonus, config2["bonuses"].Vector())
	{
		std::map<std::string,int>::const_iterator it_map;
		std::string bonusID = bonus["id"].String();

		it_map = bonusNameMap.find(bonusID);
		if (it_map != bonusNameMap.end())
			stackBonuses[it_map->second] = std::pair<std::string, std::string>(bonus["name"].String(), bonus["description"].String());
		else
			tlog2 << "Bonus " << bonusID << " not recognized, ignoring\n";
	}

	//handle magic resistance secondary skill premy, potentialy may be buggy
	//std::map<TBonusType, std::pair<std::string, std::string> >::iterator it = stackBonuses.find(Bonus::MAGIC_RESISTANCE);
	//stackBonuses[Bonus::SECONDARY_SKILL_PREMY] = std::pair<std::string, std::string>(it->second.first, it->second.second);

	if (VLC->modh->modules.STACK_EXP) 	//reading default stack experience bonuses
	{
		CLegacyConfigParser parser("DATA/CREXPBON.TXT");

		Bonus b; //prototype with some default properties
		b.source = Bonus::STACK_EXPERIENCE;
		b.duration = Bonus::PERMANENT;
		b.valType = Bonus::ADDITIVE_VALUE;
		b.effectRange = Bonus::NO_LIMIT;
		b.additionalInfo = 0;
		b.turnsRemain = 0;
		BonusList bl;

		parser.endLine();

		parser.readString(); //ignore index
		loadStackExp(b, bl, parser);
		BOOST_FOREACH(Bonus * b, bl)
			addBonusForAllCreatures(b); //health bonus is common for all
		parser.endLine();

		for (i = 1; i < 7; ++i)
		{
			for (int j = 0; j < 4; ++j) //four modifiers common for tiers
			{
				parser.readString(); //ignore index
				bl.clear();
				loadStackExp(b, bl, parser);
				BOOST_FOREACH(Bonus * b, bl)
					addBonusForTier(i, b);
				parser.endLine();
			}
		}
		for (int j = 0; j < 4; ++j) //tier 7
		{
			parser.readString(); //ignore index
			bl.clear();
			loadStackExp(b, bl, parser);
			BOOST_FOREACH(Bonus * b, bl)
			{
				addBonusForTier(7, b);
				creaturesOfLevel[0].addNewBonus(b); //bonuses from level 7 are given to high-level creatures
			}
			parser.endLine();
		}
		do //parse everything that's left
		{
			b.sid = parser.readNumber(); //id = this particular creature ID
			loadStackExp(b, creatures[b.sid]->getBonusList(), parser); //add directly to CCreature Node
		}
		while (parser.endLine());

		//Calculate rank exp values, formula appears complicated bu no parsing needed
		expRanks.resize(8);
		int dif = 0;
		int it = 8000; //ignore name of this variable
		expRanks[0].push_back(it);
		for (int j = 1; j < 10; ++j) //used for tiers 8-10, and all other probably
		{
			expRanks[0].push_back(expRanks[0][j-1] + it + dif);
			dif += it/5;
		}
		for (i = 1; i < 8; ++i)
		{
			dif = 0;
			it = 1000 * i;
			expRanks[i].push_back(it);
			for (int j = 1; j < 10; ++j)
			{
				expRanks[i].push_back(expRanks[i][j-1] + it + dif);
				dif += it/5;
			}
		}

		CLegacyConfigParser expBonParser("DATA/CREXPMOD.TXT");

		expBonParser.endLine(); //header

		maxExpPerBattle.resize(8);
		for (i = 1; i < 8; ++i)
		{
			expBonParser.readString(); //index
			expBonParser.readString(); //float multiplier -> hardcoded
			expBonParser.readString(); //ignore upgrade mod? ->hardcoded
			expBonParser.readString(); //already calculated

			maxExpPerBattle[i] = expBonParser.readNumber();
			expRanks[i].push_back(expRanks[i].back() + expBonParser.readNumber());

			expBonParser.endLine();
		}
		//skeleton gets exp penalty
			creatures[56].get()->addBonus(-50, Bonus::EXP_MULTIPLIER, -1);
			creatures[57].get()->addBonus(-50, Bonus::EXP_MULTIPLIER, -1);
		//exp for tier >7, rank 11
			expRanks[0].push_back(147000);
			expAfterUpgrade = 75; //percent
			maxExpPerBattle[0] = maxExpPerBattle[7];

	}//end of Stack Experience

	tlog5 << "\t\tReading config/commanders.json" << std::endl;
	const JsonNode config3(ResourceID("config/commanders.json"));

	BOOST_FOREACH (auto creature, config3["factionCreatures"].Vector())
	{
		factionCommanders[creature["faction"].Float()] =  creature["id"].Float();
	}
	BOOST_FOREACH (auto bonus, config3["bonusPerLevel"].Vector())
	{
		commanderLevelPremy.push_back(ParseBonus (bonus.Vector()));
	}

	i = 0;
	BOOST_FOREACH (auto skill, config3["skillLevels"].Vector())
	{
		skillLevels.push_back (std::vector<ui8>());
		BOOST_FOREACH (auto skillLevel, skill["levels"].Vector())
		{
			skillLevels[i].push_back (skillLevel.Float());
		}
		++i;
	}

	BOOST_FOREACH (auto ability, config3["abilityRequirements"].Vector())
	{
		std::pair <Bonus, std::pair <ui8, ui8> > a;
		a.first = *ParseBonus (ability["ability"].Vector());
		a.second.first = ability["skills"].Vector()[0].Float();
		a.second.second = ability["skills"].Vector()[1].Float();
		skillRequirements.push_back (a);
	}
}

void CCreatureHandler::loadAnimationInfo()
{
	auto textFile = CResourceHandler::get()->loadData(ResourceID("DATA/CRANIM.TXT"));
	std::string buf((char*)textFile.first.get(), textFile.second);
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	for(int dd=0; dd<creatures.size(); ++dd)
	{
		//tlog5 << "\t\t\tReading animation info for creature " << dd << std::endl;
		loadUnitAnimInfo(*creatures[dd], buf, i);
	}
	return;
}

void CCreatureHandler::loadUnitAnimInfo(CCreature & unit, std::string & src, int & i)
{
	int befi=i;

	unit.timeBetweenFidgets = readFloat(befi, i, src.size(), src);

	while(unit.timeBetweenFidgets == 0.0)
	{
		for(; i<src.size(); ++i)
		{
			if(src[i]=='\r')
				break;
		}
		i+=2;

		unit.timeBetweenFidgets = readFloat(befi, i, src.size(), src);
	}

	unit.walkAnimationTime = readFloat(befi, i, src.size(), src);
	unit.attackAnimationTime = readFloat(befi, i, src.size(), src);
	unit.flightAnimationDistance = readFloat(befi, i, src.size(), src);
	///////////////////////

	unit.upperRightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.upperRightMissleOffsetY = readNumber(befi, i, src.size(), src);
	unit.rightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.rightMissleOffsetY = readNumber(befi, i, src.size(), src);
	unit.lowerRightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.lowerRightMissleOffsetY = readNumber(befi, i, src.size(), src);

	///////////////////////

	for(int jjj=0; jjj<12; ++jjj)
	{
		unit.missleFrameAngles[jjj] = readFloat(befi, i, src.size(), src);
	}

	unit.troopCountLocationOffset= readNumber(befi, i, src.size(), src);
	unit.attackClimaxFrame = readNumber(befi, i, src.size(), src);

	for(; i<src.size(); ++i)
	{
		if(src[i]=='\r')
			break;
	}
	i+=2;
}

void CCreatureHandler::loadSoundsInfo()
{
	tlog5 << "\t\tReading config/cr_sounds.json" << std::endl;
	const JsonNode config(ResourceID("config/cr_sounds.json"));

	if (!config["creature_sounds"].isNull())
	{

		BOOST_FOREACH(const JsonNode &node, config["creature_sounds"].Vector())
		{
			const JsonNode *value;
			int id;

			value = &node["name"];

			bmap<std::string,int>::const_iterator i = nameToID.find(value->String());
			if (i != nameToID.end())
				id = i->second;
			else
			{
				tlog1 << "Sound info for an unknown creature: " << value->String() << std::endl;
				continue;
			}

			/* This is a bit ugly. Maybe we should use an array for
			 * sound ids instead of separate variables and define
			 * attack/defend/killed/... as indexes. */
#define GET_SOUND_VALUE(value_name) do { value = &node[#value_name]; if (!value->isNull()) creatures[id]->sounds.value_name = value->String(); } while(0)
			GET_SOUND_VALUE(attack);
			GET_SOUND_VALUE(defend);
			GET_SOUND_VALUE(killed);
			GET_SOUND_VALUE(move);
			GET_SOUND_VALUE(shoot);
			GET_SOUND_VALUE(wince);
			GET_SOUND_VALUE(ext1);
			GET_SOUND_VALUE(ext2);
			GET_SOUND_VALUE(startMoving);
			GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE
		}
	}
}

void CCreatureHandler::loadStackExp(Bonus & b, BonusList & bl, CLegacyConfigParser & parser) //help function for parsing CREXPBON.txt
{
	bool enable = false; //some bonuses are activated with values 2 or 1
	std::string buf = parser.readString();
	std::string mod = parser.readString();

	switch (buf[0])
	{
	case 'H':
		b.type = Bonus::STACK_HEALTH;
		b.valType = Bonus::PERCENT_TO_BASE;
		break;
	case 'A':
		b.type = Bonus::PRIMARY_SKILL;
		b.subtype = PrimarySkill::ATTACK;
		break;
	case 'D':
		b.type = Bonus::PRIMARY_SKILL;
		b.subtype = PrimarySkill::DEFENSE;
		break;
	case 'M': //Max damage
		b.type = Bonus::CREATURE_DAMAGE;
		b.subtype = 2;
		break;
	case 'm': //Min damage
		b.type = Bonus::CREATURE_DAMAGE;
		b.subtype = 1;
		break;
	case 'S':
		b.type = Bonus::STACKS_SPEED; break;
	case 'O':
		b.type = Bonus::SHOTS; break;

	case 'b':
		b.type = Bonus::ENEMY_DEFENCE_REDUCTION; break;
	case 'C':
		b.type = Bonus::CHANGES_SPELL_COST_FOR_ALLY; break;
	case 'd':
		b.type = Bonus::DEFENSIVE_STANCE; break;
	case 'e':
		b.type = Bonus::DOUBLE_DAMAGE_CHANCE; break;
	case 'E':
		b.type = Bonus::DEATH_STARE;
		b.subtype = 0; //Gorgon
		break;
	case 'g':
		b.type = Bonus::SPELL_DAMAGE_REDUCTION;
		b.subtype = -1; //all magic schools
		break;
	case 'P':
		b.type = Bonus::CASTS; break;
	case 'R':
		b.type = Bonus::ADDITIONAL_RETALIATION; break;
	case 'W':
		b.type = Bonus::MAGIC_RESISTANCE;
		b.subtype = 0; //otherwise creature window goes crazy
		break;

	case 'f': //on-off skill
		enable = true; //sometimes format is: 2 -> 0, 1 -> 1
		switch (mod[0])
		{
			case 'A':
				b.type = Bonus::ATTACKS_ALL_ADJACENT; break;
			case 'b':
				b.type = Bonus::RETURN_AFTER_STRIKE; break;
			case 'B':
				b.type = Bonus::TWO_HEX_ATTACK_BREATH; break;
			case 'c':
				b.type = Bonus::JOUSTING; break;
			case 'D':
				b.type = Bonus::ADDITIONAL_ATTACK; break;
			case 'f':
				b.type = Bonus::FEARLESS; break;
			case 'F':
				b.type = Bonus::FLYING; break;
			case 'm':
				b.type = Bonus::SELF_MORALE; break;
			case 'M':
				b.type = Bonus::NO_MORALE; break;
			case 'p': //Mind spells
			case 'P':
				b.type = Bonus::MIND_IMMUNITY; break;
			case 'r':
				b.type = Bonus::REBIRTH; //on/off? makes sense?
				b.subtype = 0;
				b.val = 20; //arbitrary value
				break;
			case 'R':
				b.type = Bonus::BLOCKS_RETALIATION; break;
			case 's':
				b.type = Bonus::FREE_SHOOTING; break;
			case 'u':
				b.type = Bonus::SPELL_RESISTANCE_AURA; break;
			case 'U':
				b.type = Bonus::UNDEAD; break;
			default:
			tlog5 << "Not parsed bonus " << buf << mod << "\n";
				return;
				break;
		}
		break;
	case 'w': //specific spell immunities, enabled/disabled
		enable = true;
		switch (mod[0])
		{
			case 'B': //Blind
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 74;
				break;
			case 'H': //Hypnotize
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 60;
				break;
			case 'I': //Implosion
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 18;
				break;
			case 'K': //Berserk
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 59;
				break;
			case 'M': //Meteor Shower
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 23;
				break;
			case 'N': //dispell beneficial spells
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 78;
				break;
			case 'R': //Armageddon
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 26;
				break;
			case 'S': //Slow
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 54;
				break;
			case '6':
			case '7':
			case '8':
			case '9':
				b.type = Bonus::LEVEL_SPELL_IMMUNITY;
				b.val = std::atoi(mod.c_str()) - 5;
				break;
			case ':':
				b.type = Bonus::LEVEL_SPELL_IMMUNITY;
				b.val = GameConstants::SPELL_LEVELS; //in case someone adds higher level spells?
				break;
			case 'F':
				b.type = Bonus::FIRE_IMMUNITY;
				b.subtype = 1; //not positive
				break;
			case 'O':
				b.type = Bonus::FIRE_IMMUNITY;
				b.subtype = 2; //only direct damage
				break;
			case 'f':
				b.type = Bonus::FIRE_IMMUNITY;
				b.subtype = 0; //all
				break;
			case 'C':
				b.type = Bonus::WATER_IMMUNITY;
				b.subtype = 1; //not positive
				break;
			case 'W':
				b.type = Bonus::WATER_IMMUNITY;
				b.subtype = 2; //only direct damage
				break;
			case 'w':
				b.type = Bonus::WATER_IMMUNITY;
				b.subtype = 0; //all
				break;
			case 'E':
				b.type = Bonus::EARTH_IMMUNITY;
				b.subtype = 2; //only direct damage
				break;
			case 'e':
				b.type = Bonus::EARTH_IMMUNITY;
				b.subtype = 0; //all
				break;
			case 'A':
				b.type = Bonus::AIR_IMMUNITY;
				b.subtype = 2; //only direct damage
				break;
			case 'a':
				b.type = Bonus::AIR_IMMUNITY;
				b.subtype = 0; //all
				break;
			case 'D':
				b.type = Bonus::DIRECT_DAMAGE_IMMUNITY;
				break;
			case '0':
				b.type = Bonus::RECEPTIVE;
				break;
			default:
				tlog5 << "Not parsed bonus " << buf << mod << "\n";
				return;
		}
		break;

	case 'i':
		enable = true;
		b.type = Bonus::NO_DISTANCE_PENALTY;
		break;
	case 'o':
		enable = true;
		b.type = Bonus::NO_WALL_PENALTY;
		break;

	case 'a':
	case 'c': //some special abilities are threated as spells, work in progress
		b.type = Bonus::SPELL_AFTER_ATTACK;
		b.subtype = stringToNumber(mod); 
		break;
	case 'h':
		b.type= Bonus::HATE;
		b.subtype = stringToNumber(mod);
		break;
	case 'p':
		b.type = Bonus::SPELL_BEFORE_ATTACK;
		b.subtype = stringToNumber(mod);
		b.additionalInfo = 3; //always expert?
		break;
	default:
		tlog5 << "Not parsed bonus " << buf << mod << "\n";
		return;
		break;
	}
	switch (mod[0])
	{
		case '+':
		case '=': //should we allow percent values to stack or pick highest?
			b.valType = Bonus::ADDITIVE_VALUE;
			break;
	}

	//limiters, range
	si32 lastVal, curVal, lastLev = 0;

	if (enable) //0 and 2 means non-active, 1 - active
	{
		if (b.type != Bonus::REBIRTH)
			b.val = 0; //on-off ability, no value specified
		curVal = parser.readNumber();// 0 level is never active
		for (int i = 1; i < 11; ++i)
		{
			curVal = parser.readNumber();
			if (curVal == 1)
			{
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(new Bonus(b));
				break; //never turned off it seems
			}
		}
	}
	else
	{
		lastVal = parser.readNumber(); //basic value, not particularly useful but existent
		for (int i = 1; i < 11; ++i)
		{
			curVal = parser.readNumber();
			if (b.type == Bonus::HATE)
				curVal *= 10; //odd fix
			if (curVal > lastVal) //threshold, add new bonus
			{
				b.val = curVal - lastVal;
				lastVal = curVal;
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(new Bonus(b));
				lastLev = i; //start new range from here, i = previous rank
			}
			else if (curVal < lastVal)
			{
				b.val = lastVal;
				b.limiter.reset (new RankRangeLimiter(lastLev, i));
			}
		}
	}
}

int CCreatureHandler::stringToNumber(std::string & s)
{
	boost::algorithm::replace_first(s,"#",""); //drop hash character
	return std::atoi(s.c_str());
}

CCreatureHandler::~CCreatureHandler()
{
}

static int retreiveRandNum(const boost::function<int()> &randGen)
{
	if(randGen)
		return randGen();
	else
		return rand();
}

template <typename T> const T & pickRandomElementOf(const std::vector<T> &v, const boost::function<int()> &randGen)
{
	return v[retreiveRandNum(randGen) % v.size()];
}

int CCreatureHandler::pickRandomMonster(const boost::function<int()> &randGen, int tier) const
{
	int r = 0;
	if(tier == -1) //pick any allowed creature
	{
		do 
		{
			r = pickRandomElementOf(creatures, randGen)->idNumber;
		} while (vstd::contains(VLC->creh->notUsedMonsters,r));
	}
	else
	{
		assert(vstd::iswithin(tier, 1, 7));
		std::vector<int> allowed;
		BOOST_FOREACH(const CBonusSystemNode *b, creaturesOfLevel[tier].getChildrenNodes())
		{
			assert(b->getNodeType() == CBonusSystemNode::CREATURE);
			int creid = static_cast<const CCreature*>(b)->idNumber;
			if(!vstd::contains(notUsedMonsters, creid))

				allowed.push_back(creid);
		}

		if(!allowed.size())
		{
			tlog2 << "Cannot pick a random creature of tier " << tier << "!\n";
			return 0;
		}

		return pickRandomElementOf(allowed, randGen);
	}
	return r;
}

void CCreatureHandler::addBonusForTier(int tier, Bonus *b)
{
	assert(vstd::iswithin(tier, 1, 7));
	creaturesOfLevel[tier].addNewBonus(b);
}

void CCreatureHandler::addBonusForAllCreatures(Bonus *b)
{
	allCreatures.addNewBonus(b);
}

void CCreatureHandler::buildBonusTreeForTiers()
{
	BOOST_FOREACH(CCreature *c, creatures)
	{
		if(vstd::isbetween(c->level, 0, ARRAY_COUNT(creaturesOfLevel)))
			c->attachTo(&creaturesOfLevel[c->level]);
		else
			c->attachTo(&creaturesOfLevel[0]);
	}
	BOOST_FOREACH(CBonusSystemNode &b, creaturesOfLevel)
		b.attachTo(&allCreatures);
}

void CCreatureHandler::deserializationFix()
{
	buildBonusTreeForTiers();
}
