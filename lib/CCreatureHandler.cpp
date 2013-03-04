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
 
static inline void registerCreature(const std::string &name, const si32 id)
{
	const std::string fullname = "creature." + name;
	VLC->modh->identifiers.registerObject(fullname,id);
}

///CCreatureHandler

CCreatureHandler::CCreatureHandler()
{
	VLC->creh = this;

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
	return VLC->townh->factions[faction].alignment == EAlignment::GOOD;
}

/**
 * Determines if the creature is of an evil alignment.
 * @return true if the creature is evil, false otherwise.
 */
bool CCreature::isEvil () const
{
	return VLC->townh->factions[faction].alignment == EAlignment::EVIL;
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
void CCreature::addBonus(int val, Bonus::BonusType type, int subtype /*= -1*/)
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
	return VLC->townh->factions[faction].nativeTerrain == terrain;
}

static void AddAbility(CCreature *cre, const JsonVector &ability_vec)
{
	Bonus *nsf = new Bonus();
	std::string type = ability_vec[0].String();

	auto it = bonusNameMap.find(type);

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
			tlog1 << "Error: invalid ability type " << type << " in creatures config" << std::endl;

		return;
	}

	nsf->type = it->second;
	
	JsonUtils::parseTypedBonusShort(ability_vec,nsf);

	nsf->source = Bonus::CREATURE_ABILITY;
	nsf->sid = cre->idNumber;
	
	cre->addNewBonus(nsf);
}

static void RemoveAbility(CCreature *cre, const JsonNode &ability)
{
	const std::string type = ability.String();

	auto it = bonusNameMap.find(type);

	if (it == bonusNameMap.end()) {
		if (type == "DOUBLE_WIDE")
			cre->doubleWide = false;
		else
			tlog1 << "Error: invalid ability type " << type << " in creatures config" << std::endl;

		return;
	}

	const int typeNo = it->second;

	Bonus::BonusType ecf = static_cast<Bonus::BonusType>(typeNo);

	Bonus *b = cre->getBonusLocalFirst(Selector::type(ecf));
	cre->removeBonus(b);
}

void CCreatureHandler::loadBonuses(CCreature & ncre, std::string bonuses)
{
	static const std::map<std::string,Bonus::BonusType> abilityMap =
	  boost::assign::map_list_of
	    ("FLYING_ARMY", Bonus::FLYING)
	    ("SHOOTING_ARMY", Bonus::SHOOTER)
	    ("SIEGE_WEAPON", Bonus::SIEGE_WEAPON)
	    ("const_free_attack", Bonus::BLOCKS_RETALIATION)
	    ("IS_UNDEAD", Bonus::UNDEAD)
	    ("const_no_melee_penalty",Bonus::NO_MELEE_PENALTY)
	    ("const_jousting",Bonus::JOUSTING)
	    ("KING_1",Bonus::KING1)
	    ("KING_2",Bonus::KING2)
		("KING_3",Bonus::KING3)
		("const_no_wall_penalty",Bonus::NO_WALL_PENALTY)
		("CATAPULT",Bonus::CATAPULT)
		("MULTI_HEADED",Bonus::ATTACKS_ALL_ADJACENT)
		("IMMUNE_TO_MIND_SPELLS",Bonus::MIND_IMMUNITY)
		("HAS_EXTENDED_ATTACK",Bonus::TWO_HEX_ATTACK_BREATH);

	auto hasAbility = [&](const std::string name) -> bool
	{
		return boost::algorithm::find_first(bonuses, name);
	};
	BOOST_FOREACH(auto a, abilityMap)
	{
		if(hasAbility(a.first))
			ncre.addBonus(0, a.second);
	}
	if(hasAbility("DOUBLE_WIDE"))
		ncre.doubleWide = true;
	if(hasAbility("const_raises_morale"))
	{
		ncre.addBonus(+1, Bonus::MORALE);;
		ncre.getBonusList().back()->addPropagator(make_shared<CPropagatorNodeType>(CBonusSystemNode::HERO));
	}
	if(hasAbility("const_lowers_morale"))
	{
		ncre.addBonus(-1, Bonus::MORALE);;
		ncre.getBonusList().back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
	}
}

void CCreatureHandler::loadCreatures()
{
	tlog5 << "\t\tReading ZCRTRAIT.TXT" << std::endl;

	////////////reading ZCRTRAIT.TXT ///////////////////
	CLegacyConfigParser parser("DATA/ZCRTRAIT.TXT");

	parser.endLine(); // header
	parser.endLine();

	do
	{
		//loop till non-empty line
		while (parser.isNextEntryEmpty())
			parser.endLine();

		CCreature &ncre = *new CCreature;
		ncre.idNumber = CreatureID(creatures.size());
		ncre.cost.resize(GameConstants::RESOURCE_QUANTITY);
		ncre.level=0;
		ncre.iconIndex = ncre.idNumber + 2; // +2 for empty\selection images

		ncre.nameSing = parser.readString();
		ncre.namePl   = parser.readString();

		for(int v=0; v<7; ++v)
		{
			ncre.cost[v] = parser.readNumber();
		}
		ncre.fightValue = parser.readNumber();
		ncre.AIValue = parser.readNumber();
		ncre.growth = parser.readNumber();
		ncre.hordeGrowth = parser.readNumber();

		ncre.addBonus(parser.readNumber(), Bonus::STACK_HEALTH);
		ncre.addBonus(parser.readNumber(), Bonus::STACKS_SPEED);
		ncre.addBonus(parser.readNumber(), Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
		ncre.addBonus(parser.readNumber(), Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
		ncre.addBonus(parser.readNumber(), Bonus::CREATURE_DAMAGE, 1);
		ncre.addBonus(parser.readNumber(), Bonus::CREATURE_DAMAGE, 2);
		ncre.addBonus(parser.readNumber(), Bonus::SHOTS);

		//spells - not used?
		parser.readNumber();
		ncre.ammMin = parser.readNumber();
		ncre.ammMax = parser.readNumber();

		std::string abilities = parser.readString();
		loadBonuses(ncre, parser.readString());

		{ //adding abilities from ZCRTRAIT.TXT
			static const std::map < std::string,Bonus::BonusType> abilityMap = boost::assign::map_list_of
			   ("FLYING_ARMY", Bonus::FLYING)
			    ("SHOOTING_ARMY", Bonus::SHOOTER)
			    ("SIEGE_WEAPON", Bonus::SIEGE_WEAPON)
			    ("const_free_attack", Bonus::BLOCKS_RETALIATION)
			    ("IS_UNDEAD", Bonus::UNDEAD)
			    ("const_no_melee_penalty",Bonus::NO_MELEE_PENALTY)
			    ("const_jousting",Bonus::JOUSTING)
			    ("KING_1",Bonus::KING1)
			    ("KING_2",Bonus::KING2)
				("KING_3",Bonus::KING3)
				("const_no_wall_penalty",Bonus::NO_WALL_PENALTY)
				("CATAPULT",Bonus::CATAPULT)
				("MULTI_HEADED",Bonus::ATTACKS_ALL_ADJACENT)
				("IMMUNE_TO_MIND_SPELLS",Bonus::MIND_IMMUNITY)
				("IMMUNE_TO_FIRE_SPELLS",Bonus::FIRE_IMMUNITY)
				("IMMUNE_TO_FIRE_SPELLS",Bonus::FIRE_IMMUNITY)
				("HAS_EXTENDED_ATTACK",Bonus::TWO_HEX_ATTACK_BREATH);

			auto hasAbility = [&](const std::string name) -> bool
			{
				return boost::algorithm::find_first(abilities, name);
			};
			BOOST_FOREACH(auto a, abilityMap)
			{
				if(hasAbility(a.first))
					ncre.addBonus(0, a.second);
			}			
			if(hasAbility("DOUBLE_WIDE"))
				ncre.doubleWide = true;
			if(hasAbility("const_raises_morale"))
			{
				ncre.addBonus(+1, Bonus::MORALE);;
				ncre.getBonusList().back()->addPropagator(make_shared<CPropagatorNodeType>(CBonusSystemNode::HERO));
			}
			if(hasAbility("const_lowers_morale"))
			{
				ncre.addBonus(-1, Bonus::MORALE);;
				ncre.getBonusList().back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
			}
		}
		creatures.push_back(&ncre);
	}
	while (parser.endLine());

	// loading creatures properties
	tlog5 << "\t\tReading creatures json configs" << std::endl;

	const JsonNode gameConf(ResourceID("config/gameConfig.json"));
	const JsonNode config(JsonUtils::assembleFromFiles(gameConf["creatures"].convertTo<std::vector<std::string> >()));

	BOOST_FOREACH(auto & node, config.Struct())
	{
		int creatureID = node.second["id"].Float();
		CCreature *c = creatures[creatureID];

		loadCreatureJson(c, node.second);

		// Main reference name, e.g. royalGriffin
		c->nameRef = node.first;
		registerCreature(node.first, c->idNumber);

		// Alternative names, if any
		BOOST_FOREACH(const JsonNode &name, node.second["extraNames"].Vector())
		{
			registerCreature(name.String(), c->idNumber);
		}
	}

	loadAnimationInfo();

	//reading creature ability names
	const JsonNode config2(ResourceID("config/bonusnames.json"));

	BOOST_FOREACH(const JsonNode &bonus, config2["bonuses"].Vector())
	{
		const std::string bonusID = bonus["id"].String();

		auto it_map = bonusNameMap.find(bonusID);
		if (it_map != bonusNameMap.end())
			stackBonuses[it_map->second] = std::pair<std::string, std::string>(bonus["name"].String(), bonus["description"].String());
		else
			tlog2 << "Bonus " << bonusID << " not recognized, ignoring\n";
	}

	//handle magic resistance secondary skill premy, potentialy may be buggy
	//std::map<Bonus::BonusType, std::pair<std::string, std::string> >::iterator it = stackBonuses.find(Bonus::MAGIC_RESISTANCE);
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

		for (int i = 1; i < 7; ++i)
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
		for (int i = 1; i < 8; ++i)
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
		for (int i = 1; i < 8; ++i)
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

	BOOST_FOREACH (auto bonus, config3["bonusPerLevel"].Vector())
	{
		commanderLevelPremy.push_back(JsonUtils::parseBonus (bonus.Vector()));
	}

	int i = 0;
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
		a.first = *JsonUtils::parseBonus (ability["ability"].Vector());
		a.second.first = ability["skills"].Vector()[0].Float();
		a.second.second = ability["skills"].Vector()[1].Float();
		skillRequirements.push_back (a);
	}
}

void CCreatureHandler::loadAnimationInfo()
{
	CLegacyConfigParser parser("DATA/CRANIM.TXT");

	parser.endLine(); // header
	parser.endLine();

	for(int dd=0; dd<creatures.size(); ++dd)
	{
		while (parser.isNextEntryEmpty() && parser.endLine()) // skip empty lines
			;

		loadUnitAnimInfo(*creatures[dd], parser);
	}
}

void CCreatureHandler::loadUnitAnimInfo(CCreature & unit, CLegacyConfigParser & parser)
{
	unit.animation.timeBetweenFidgets = parser.readNumber();
	unit.animation.walkAnimationTime = parser.readNumber();
	unit.animation.attackAnimationTime = parser.readNumber();
	unit.animation.flightAnimationDistance = parser.readNumber();
	///////////////////////

	unit.animation.upperRightMissleOffsetX = parser.readNumber();
	unit.animation.upperRightMissleOffsetY = parser.readNumber();
	unit.animation.rightMissleOffsetX = parser.readNumber();
	unit.animation.rightMissleOffsetY = parser.readNumber();
	unit.animation.lowerRightMissleOffsetX = parser.readNumber();
	unit.animation.lowerRightMissleOffsetY = parser.readNumber();

	///////////////////////

	for(int jjj=0; jjj<12; ++jjj)
	{
		unit.animation.missleFrameAngles[jjj] = parser.readNumber();
	}

	unit.animation.troopCountLocationOffset= parser.readNumber();
	unit.animation.attackClimaxFrame = parser.readNumber();

	parser.endLine();
}

void CCreatureHandler::load(std::string creatureID, const JsonNode & node)
{
	CCreature * creature = loadCreature(node);
	creature->nameRef = creatureID;
	creature->idNumber = CreatureID(creatures.size());

	creatures.push_back(creature);
	tlog5 << "Added creature: " << creatureID << "\n";
	registerCreature(creature->nameRef, creature->idNumber);
}

CCreature * CCreatureHandler::loadCreature(const JsonNode & node)
{
	CCreature * cre = new CCreature();

	const JsonNode & name = node["name"];
	cre->nameSing = name["singular"].String();
	cre->namePl = name["plural"].String();

	cre->cost = Res::ResourceSet(node["cost"]);

	cre->fightValue = node["fightValue"].Float();
	cre->AIValue = node["aiValue"].Float();
	cre->growth = node["growth"].Float();
	cre->hordeGrowth = node["horde"].Float(); // Needed at least until configurable buildings

	cre->addBonus(node["hitPoints"].Float(), Bonus::STACK_HEALTH);
	cre->addBonus(node["speed"].Float(), Bonus::STACKS_SPEED);
	cre->addBonus(node["attack"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
	cre->addBonus(node["defense"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
	const JsonNode &  vec = node["damage"];
	cre->addBonus(vec["min"].Float(), Bonus::CREATURE_DAMAGE, 1);
	cre->addBonus(vec["max"].Float(), Bonus::CREATURE_DAMAGE, 2);

	auto & amounts = node ["advMapAmount"];
	cre->ammMin = amounts["min"].Float();
	cre->ammMax = amounts["max"].Float();

	if (!node["shots"].isNull())
		cre->addBonus(node["shots"].Float(), Bonus::SHOTS);

	if (node["spellPoints"].isNull())
		cre->addBonus(node["spellPoints"].Float(), Bonus::CASTS);

	cre->doubleWide = node["doubleWide"].Bool();

	//graphics
	loadStackExperience(cre, node["stackExperience"]);

	const JsonNode & graphics = node["graphics"];
	cre->animation.timeBetweenFidgets = graphics["timeBetweenFidgets"].Float();
	cre->animation.troopCountLocationOffset = graphics["troopCountLocationOffset"].Float();
	cre->animation.attackClimaxFrame = graphics["attackClimaxFrame"].Float();

	const JsonNode & animationTime = graphics["animationTime"];
	cre->animation.walkAnimationTime = animationTime["walk"].Float();
	cre->animation.attackAnimationTime = animationTime["attack"].Float();
	cre->animation.flightAnimationDistance = animationTime["flight"].Float(); //?

	const JsonNode & missile = graphics["missile"];
	const JsonNode & offsets = missile["offset"];
	cre->animation.upperRightMissleOffsetX = offsets["upperX"].Float();
	cre->animation.upperRightMissleOffsetY = offsets["upperY"].Float();
	cre->animation.rightMissleOffsetX = offsets["middleX"].Float();
	cre->animation.rightMissleOffsetY = offsets["middleY"].Float();
	cre->animation.lowerRightMissleOffsetX = offsets["lowerX"].Float();
	cre->animation.lowerRightMissleOffsetY = offsets["lowerY"].Float();
	int i = 0;
	BOOST_FOREACH (auto & angle, missile["frameAngles"].Vector())
	{
		cre->animation.missleFrameAngles[i++] = angle.Float();
	}
	cre->advMapDef = graphics["map"].String();
	cre->iconIndex = graphics["iconIndex"].Float();

	loadCreatureJson(cre, node);
	return cre;
}

void CCreatureHandler::loadCreatureJson(CCreature * creature, const JsonNode & config)
{
	creature->level = config["level"].Float();
	creature->animDefName = config["graphics"]["animation"].String();

	BOOST_FOREACH(const JsonNode &ability, config["ability_remove"].Vector())
	{
		RemoveAbility(creature, ability);
	}

	BOOST_FOREACH(const JsonNode &ability, config["abilities"].Vector())
	{
		if (ability.getType() == JsonNode::DATA_VECTOR)
			AddAbility(creature, ability.Vector()); // used only for H3 creatures
		else
		{
			auto b = JsonUtils::parseBonus(ability);
			b->source = Bonus::CREATURE_ABILITY;
			b->duration = Bonus::PERMANENT;
			creature->addNewBonus(b);
		}
	}

	VLC->modh->identifiers.requestIdentifier(std::string("faction.") + config["faction"].String(), [=](si32 faction)
	{
		creature->faction = faction;
	});

	BOOST_FOREACH(const JsonNode &value, config["upgrades"].Vector())
	{
		VLC->modh->identifiers.requestIdentifier(std::string("creature.") + value.String(), [=](si32 identifier)
		{
			creature->upgrades.insert(CreatureID(identifier));
		});
	}

	if(config["hasDoubleWeek"].Bool())
		doubledCreatures.insert(creature->idNumber);

	creature->animation.projectileImageName = config["graphics"]["missile"]["projectile"].String();
	creature->animation.projectileSpin = config["graphics"]["missile"]["spinning"].Bool();

	creature->special = config["special"].Bool();

	const JsonNode & sounds = config["sound"];

#define GET_SOUND_VALUE(value_name) creature->sounds.value_name = sounds[#value_name].String()
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

void CCreatureHandler::loadStackExperience(CCreature * creature, const JsonNode & input)
{
	BOOST_FOREACH (const JsonNode &exp, input.Vector())
	{
		auto bonus = JsonUtils::parseBonus (exp["bonus"]); // FIXME: memory leak? Only copies of bonus is added to creature
		bonus->source = Bonus::STACK_EXPERIENCE;
		bonus->duration = Bonus::PERMANENT;
		const JsonVector &values = exp["values"].Vector();
		int lowerLimit = 1;//, upperLimit = 255;
		if (values[0].getType() == JsonNode::JsonType::DATA_BOOL)
		{
			BOOST_FOREACH (const JsonNode &val, values)
			{
				if (val.Bool() == true)
				{
					bonus->limiter = make_shared<RankRangeLimiter>(RankRangeLimiter(lowerLimit));
					creature->addNewBonus (new Bonus(*bonus)); //bonuses must be unique objects
					break; //TODO: allow bonuses to turn off?
				}
				++lowerLimit;
			}
		}
		else
		{
			int lastVal = 0;
			BOOST_FOREACH (const JsonNode &val, values)
			{
				if (val.Float() != lastVal)
				{
					bonus->val = val.Float() - lastVal;
					bonus->limiter.reset (new RankRangeLimiter(lowerLimit));
					creature->addNewBonus (new Bonus(*bonus));
				}
				lastVal = val.Float();
				++lowerLimit;
			}
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
		b.type = Bonus::DOUBLE_DAMAGE_CHANCE;
		b.subtype = 0;
		break;
	case 'E':
		b.type = Bonus::DEATH_STARE;
		b.subtype = 0; //Gorgon
		break;
	case 'F':
		b.type = Bonus::FEAR; break;
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
				b.subtype = SpellID::BLIND;
				break;
			case 'H': //Hypnotize
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::HYPNOTIZE;
				break;
			case 'I': //Implosion
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::IMPLOSION;
				break;
			case 'K': //Berserk
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::BERSERK;
				break;
			case 'M': //Meteor Shower
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::METEOR_SHOWER;
				break;
			case 'N': //dispell beneficial spells
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::DISPEL_HELPFUL_SPELLS;
				break;
			case 'R': //Armageddon
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::ARMAGEDDON;
				break;
			case 'S': //Slow
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = SpellID::SLOW;
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
			case 'm':
				b.type = Bonus::MIND_IMMUNITY;
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
	case 'c':
	case 'K':
	case 'k':
		b.type = Bonus::SPELL_AFTER_ATTACK;
		b.subtype = stringToNumber(mod);
		break;
	case 'h':
		b.type= Bonus::HATE;
		b.subtype = stringToNumber(mod);
		break;
	case 'p':
	case 'J':
		b.type = Bonus::SPELL_BEFORE_ATTACK;
		b.subtype = stringToNumber(mod);
		b.additionalInfo = 3; //always expert?
		break;
	case 'r':
		b.type = Bonus::HP_REGENERATION;
		b.val = stringToNumber(mod);
		break;
	case 's':
		b.type = Bonus::ENCHANTED;
		b.subtype = stringToNumber(mod);
		b.valType = Bonus::INDEPENDENT_MAX;
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

CreatureID CCreatureHandler::pickRandomMonster(const boost::function<int()> &randGen, int tier) const
{
	int r = 0;
	if(tier == -1) //pick any allowed creature
	{
		do
		{
			r = vstd::pickRandomElementOf(creatures, randGen)->idNumber;
		} while (VLC->creh->creatures[r] && VLC->creh->creatures[r]->special); // find first "not special" creature
	}
	else
	{
		assert(vstd::iswithin(tier, 1, 7));
		std::vector<CreatureID> allowed;
		BOOST_FOREACH(const CBonusSystemNode *b, creaturesOfLevel[tier].getChildrenNodes())
		{
			assert(b->getNodeType() == CBonusSystemNode::CREATURE);
			const CCreature * crea = dynamic_cast<const CCreature*>(b);
			if(crea && !crea->special)
				allowed.push_back(crea->idNumber);
		}

		if(!allowed.size())
		{
			tlog2 << "Cannot pick a random creature of tier " << tier << "!\n";
			return CreatureID::NONE;
		}

		return vstd::pickRandomElementOf(allowed, randGen);
	}
	return CreatureID(r);
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
