#include "StdInc.h"
#include "CSpellHandler.h"

#include "CGeneralTextHandler.h"
#include "filesystem/Filesystem.h"

#include "JsonNode.h"
#include <cctype>
#include "BattleHex.h"
#include "CModHandler.h"
#include "StringConstants.h"

/*
 * CSpellHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace SpellConfigJson
{
    static const std::string level_names[] = {"none","basic","advanced","expert"};
}

using namespace boost::assign;

namespace SRSLPraserHelpers
{
	static int XYToHex(int x, int y)
	{
		return x + 17 * y;
	}

	static int XYToHex(std::pair<int, int> xy)
	{
		return XYToHex(xy.first, xy.second);
	}

	static int hexToY(int battleFieldPosition)
	{
		return battleFieldPosition/17;
	}

	static int hexToX(int battleFieldPosition)
	{
		int pos = battleFieldPosition - hexToY(battleFieldPosition) * 17;
		return pos;
	}

	static std::pair<int, int> hexToPair(int battleFieldPosition)
	{
		return std::make_pair(hexToX(battleFieldPosition), hexToY(battleFieldPosition));
	}

	//moves hex by one hex in given direction
	//0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left
	static std::pair<int, int> gotoDir(int x, int y, int direction)
	{
		switch(direction)
		{
		case 0: //top left
			return std::make_pair((y%2) ? x-1 : x, y-1);
		case 1: //top right
			return std::make_pair((y%2) ? x : x+1, y-1);
		case 2:  //right
			return std::make_pair(x+1, y);
		case 3: //right bottom
			return std::make_pair((y%2) ? x : x+1, y+1);
		case 4: //left bottom
			return std::make_pair((y%2) ? x-1 : x, y+1);
		case 5: //left
			return std::make_pair(x-1, y);
		default:
			throw std::runtime_error("Disaster: wrong direction in SRSLPraserHelpers::gotoDir!\n");
		}
	}

	static std::pair<int, int> gotoDir(std::pair<int, int> xy, int direction)
	{
		return gotoDir(xy.first, xy.second, direction);
	}

	static bool isGoodHex(std::pair<int, int> xy)
	{
		return xy.first >=0 && xy.first < 17 && xy.second >= 0 && xy.second < 11;
	}

	//helper function for std::set<ui16> CSpell::rangeInHexes(unsigned int centralHex, ui8 schoolLvl ) const
	static std::set<ui16> getInRange(unsigned int center, int low, int high)
	{
		std::set<ui16> ret;
		if(low == 0)
		{
			ret.insert(center);
		}

		std::pair<int, int> mainPointForLayer[6]; //A, B, C, D, E, F points
		for(auto & elem : mainPointForLayer)
			elem = hexToPair(center);

		for(int it=1; it<=high; ++it) //it - distance to the center
		{
			for(int b=0; b<6; ++b)
				mainPointForLayer[b] = gotoDir(mainPointForLayer[b], b);

			if(it>=low)
			{
				std::pair<int, int> curHex;

				//adding lines (A-b, B-c, C-d, etc)
				for(int v=0; v<6; ++v)
				{
					curHex = mainPointForLayer[v];
					for(int h=0; h<it; ++h)
					{
						if(isGoodHex(curHex))
							ret.insert(XYToHex(curHex));
						curHex = gotoDir(curHex, (v+2)%6);
					}
				}

			} //if(it>=low)
		}

		return ret;
	}
}

using namespace SRSLPraserHelpers;

CSpell::CSpell():
    id(SpellID::NONE), level(0),
    earth(false),water(false),fire(false),air(false),
    power(0),
    combatSpell(false),creatureAbility(false),
    positiveness(ESpellPositiveness::NEUTRAL),
    mainEffectAnim(-1),
    defaultProbability(0),
    isRising(false),isDamage(false),isOffensive(false),targetType(ETargetType::NO_TARGET)

{

}

CSpell::~CSpell()
{
	for (auto & elem : effects)
	{
		for (size_t j=0; j<elem.size(); j++)
			delete elem[j];
	}
}

std::vector<BattleHex> CSpell::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes) const
{
	std::vector<BattleHex> ret;

	if(id == SpellID::FIRE_WALL  ||  id == SpellID::FORCE_FIELD)
	{
		//Special case - shape of obstacle depends on caster's side
		//TODO make it possible through spell_info config

		BattleHex::EDir firstStep, secondStep;
		if(side)
		{
			firstStep = BattleHex::TOP_LEFT;
			secondStep = BattleHex::TOP_RIGHT;
		}
		else
		{
			firstStep = BattleHex::TOP_RIGHT;
			secondStep = BattleHex::TOP_LEFT;
		}

		//Adds hex to the ret if it's valid. Otherwise sets output arg flag if given.
		auto addIfValid = [&](BattleHex hex)
		{
			if(hex.isValid())
				ret.push_back(hex);
			else if(outDroppedHexes)
				*outDroppedHexes = true;
		};

		ret.push_back(centralHex);
		addIfValid(centralHex.moveInDir(firstStep, false));
		if(schoolLvl >= 2) //advanced versions of fire wall / force field cotnains of 3 hexes
			addIfValid(centralHex.moveInDir(secondStep, false)); //moveInDir function modifies subject hex

		return ret;
	}


	std::string rng = range[schoolLvl] + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 1 && rng[0] != 'X') //there is at lest one hex in range
	{
		std::string number1, number2;
		int beg, end;
		bool readingFirst = true;
		for(auto & elem : rng)
		{
			if( std::isdigit(elem) ) //reading numer
			{
				if(readingFirst)
					number1 += elem;
				else
					number2 += elem;
			}
			else if(elem == ',') //comma
			{
				//calculating variables
				if(readingFirst)
				{
					beg = atoi(number1.c_str());
					number1 = "";
				}
				else
				{
					end = atoi(number2.c_str());
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					curLayer = getInRange(centralHex, beg, beg);
				}
				else
				{
					curLayer = getInRange(centralHex, beg, end);
					readingFirst = true;
				}
				//adding abtained hexes
				for(auto & curLayer_it : curLayer)
				{
					ret.push_back(curLayer_it);
				}

			}
			else if(elem == '-') //dash
			{
				beg = atoi(number1.c_str());
				number1 = "";
				readingFirst = false;
			}
		}
	}

	//remove duplicates (TODO check if actually needed)
	range::unique(ret);
	return ret;
}

CSpell::ETargetType CSpell::getTargetType() const
{
	return targetType;
}


void CSpell::getEffects(std::vector<Bonus>& lst, const int level) const
{
	if (level < 0 || level >= GameConstants::SPELL_SCHOOL_LEVELS)
	{
        logGlobal->errorStream() << __FUNCTION__ << " invalid school level " << level;
		return;
	}
	if (effects.empty())
	{
        logGlobal->errorStream() << __FUNCTION__ << " This spell ("  + name + ") has no bonus effects! " << name;
		return;
	}
	if (effects.size() <= level)
	{
		logGlobal->errorStream() << __FUNCTION__ << " This spell ("  + name + ") is missing entry for level " << level;
		return;
	}
	if (effects[level].empty())
    {
		logGlobal->errorStream() << __FUNCTION__ << " This spell ("  + name + ") has no effects for level " << level;
		return;
    }

	lst.reserve(lst.size() + effects[level].size());

	for (Bonus *b : effects[level])
	{
		lst.push_back(Bonus(*b));
	}
}

bool CSpell::isImmuneBy(const IBonusBearer* obj) const
{
	//todo: use new bonus API
	//1. Check limiters
	for(auto b : limiters)
	{
		if (!obj->hasBonusOfType(b))
			return true;
	}

	//2. Check absolute immunities
	//todo: check config: some creatures are unaffected always, for example undead to resurrection.
    for(auto b : absoluteImmunities)
	{
		if (obj->hasBonusOfType(b))
			return true;
	}

    //3. Check negation
	if (obj->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES)) //Orb of vulnerability
		return false;

    //4. Check negatable immunities
	for(auto b : immunities)
	{
		if (obj->hasBonusOfType(b))
			return true;
	}

	auto battleTestElementalImmunity = [&,this](Bonus::BonusType element) -> bool
	{
		if (obj->hasBonusOfType(element, 0)) //always resist if immune to all spells altogether
				return true;
		else if (!isPositive()) //negative or indifferent
		{
			if ((isDamageSpell() && obj->hasBonusOfType(element, 2)) || obj->hasBonusOfType(element, 1))
				return true;
		}
		return false;
	};

    //4. Check elemental immunities
	if (fire)
	{
		if (battleTestElementalImmunity(Bonus::FIRE_IMMUNITY))
			return true;
	}
	if (water)
	{
		if (battleTestElementalImmunity(Bonus::WATER_IMMUNITY))
			return true;
	}

	if (earth)
	{
		if (battleTestElementalImmunity(Bonus::EARTH_IMMUNITY))
			return true;
	}
	if (air)
	{
		if (battleTestElementalImmunity(Bonus::AIR_IMMUNITY))
			return true;
	}

	TBonusListPtr levelImmunities = obj->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));

	if(obj->hasBonusOfType(Bonus::SPELL_IMMUNITY, id)
		|| ( levelImmunities->size() > 0  &&  levelImmunities->totalValue() >= level  &&  level))
	{
		return true;
	}

	return false;
}

void CSpell::setAttributes(const std::string& newValue)
{
	attributes = newValue;
	if(attributes.find("CREATURE_TARGET_1") != std::string::npos
		|| attributes.find("CREATURE_TARGET_2") != std::string::npos)
		targetType = CREATURE_EXPERT_MASSIVE;
	else if(attributes.find("CREATURE_TARGET") != std::string::npos)
		targetType = CREATURE;
	else if(attributes.find("OBSTACLE_TARGET") != std::string::npos)
		targetType = OBSTACLE;
	else
		targetType = NO_TARGET;
}

void CSpell::setIsOffensive(const bool val)
{
   isOffensive = val;

   if (val)
   {
       positiveness = CSpell::NEGATIVE;
       isDamage = true;
   }
}

void CSpell::setIsRising(const bool val)
{
    isRising = val;

    if (val)
    {
        positiveness = CSpell::POSITIVE;
    }
}



bool DLL_LINKAGE isInScreenRange(const int3 &center, const int3 &pos)
{
	int3 diff = pos - center;
	if(diff.x >= -9  &&  diff.x <= 9  &&  diff.y >= -8  &&  diff.y <= 8)
		return true;
	else
		return false;
}

CSpellHandler::CSpellHandler()
{

}

std::vector<JsonNode> CSpellHandler::loadLegacyData(size_t dataSize)
{
    using namespace SpellConfigJson;
    std::vector<JsonNode> legacyData;

	CLegacyConfigParser parser("DATA/SPTRAITS.TXT");

	auto readSchool = [&](JsonMap& schools, const std::string& name)
	{
        if (parser.readString() == "x")
        {
            schools[name].Bool() = true;
        }
	};

	auto read = [&,this](bool combat, bool ability)
	{
		do
		{
		    JsonNode lineNode(JsonNode::DATA_STRUCT);

		    const si32 id = legacyData.size();

            lineNode["index"].Float() = id;
            lineNode["type"].String() = ability ? "ability" : (combat ? "combat" : "adventure");

            lineNode["name"].String() = parser.readString();

            parser.readString(); //ignored unused abbreviated name
            lineNode["level"].Float()      = parser.readNumber();

            auto& schools = lineNode["school"].Struct();

            readSchool(schools, "earth");
            readSchool(schools, "water");
            readSchool(schools, "fire");
            readSchool(schools, "air");

            auto& levels = lineNode["levels"].Struct();



            auto getLevel = [&](const size_t idx)->JsonMap&
            {
                assert(idx < GameConstants::SPELL_SCHOOL_LEVELS);
                return levels[level_names[idx]].Struct();

            };

            auto costs = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);
            lineNode["power"].Float() = parser.readNumber();
            auto powers = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

            auto& chances = lineNode["gainChance"].Struct();

            for (size_t i = 0; i < GameConstants::F_NUMBER ; i++){
                chances[ETownType::names[i]].Float() = parser.readNumber();
            }

            auto AIVals = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

            std::vector<std::string> descriptions;
            for (size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS ; i++)
                descriptions.push_back(parser.readString());

            std::string attributes = parser.readString();

            std::string targetType = "NO_TARGET";

            if(attributes.find("CREATURE_TARGET_1") != std::string::npos
                || attributes.find("CREATURE_TARGET_2") != std::string::npos)
                targetType = "CREATURE_EXPERT_MASSIVE";
            else if(attributes.find("CREATURE_TARGET") != std::string::npos)
                targetType = "CREATURE";
            else if(attributes.find("OBSTACLE_TARGET") != std::string::npos)
                targetType = "OBSTACLE";

            lineNode["targetType"].String() = targetType;



            //save parsed level specific data
            for (size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
            {
                auto& level = getLevel(i);
                level["description"].String() = descriptions[i];
                level["cost"].Float() = costs[i];
                level["power"].Float() = powers[i];
                level["aiValue"].Float() = AIVals[i];

            }


//            logGlobal->errorStream() << lineNode;
		    legacyData.push_back(lineNode);


		}
		while (parser.endLine() && !parser.isNextEntryEmpty());
	};

	auto skip = [&](int cnt)
	{
		for(int i=0; i<cnt; i++)
			parser.endLine();
	};

	skip(5);// header
	read(false,false); //read adventure map spells
	skip(3);
	read(true,false); //read battle spells
	skip(3);
	read(true,true);//read creature abilities

    //TODO: maybe move to config
	//clone Acid Breath attributes for Acid Breath damage effect
	JsonNode temp = legacyData[SpellID::ACID_BREATH_DEFENSE];
	temp["index"].Float() = SpellID::ACID_BREATH_DAMAGE;
    legacyData.push_back(temp);

    objects.resize(legacyData.size());

	return legacyData;
}

const std::string CSpellHandler::getTypeName()
{
    return "spell";
}

static void fatalConfigurationError()
{
   throw std::runtime_error("SpellHandler: Fatal configuration error, See log for details");
}

CSpell * CSpellHandler::loadFromJson(const JsonNode& json)
{
    using namespace SpellConfigJson;

    CSpell * spell = new CSpell();

    const auto type_str = json["type"].String();

    if (type_str == "ability")
    {
        spell->creatureAbility = true;
        spell->combatSpell = true;
    }
    else
    {
       spell->creatureAbility = false;
       spell->combatSpell = type_str == "combat";
    }

    spell->name = json["name"].String();

    logGlobal->traceStream() << __FUNCTION__ << ": loading spell " << spell->name;

    auto readFlag = [](const JsonNode& flagsNode, const std::string& name)
    {
        if (flagsNode.getType() != JsonNode::DATA_STRUCT)
        {
            logGlobal->errorStream() << "Flags node shall be object";
            return false;
        }

        const JsonNode& flag = flagsNode[name];

        if (flag.isNull())
        {
            return false;
        }
        else if (flag.getType() == JsonNode::DATA_BOOL)
        {
            return flag.Bool();
        }
        else
        {
            logGlobal->errorStream() << "Flag shall be boolean: "<<name;
            return false;
        }

    };

    const auto school_names = json["school"];

    spell->air = readFlag(school_names, "air");
    spell->earth = readFlag(school_names, "earth");
    spell->fire = readFlag(school_names, "fire");
    spell->water = readFlag(school_names, "water");

    spell->level = json["level"].Float();
    spell->power = json["power"].Float();

    //TODO: default chance
    spell->defaultProbability = json["defaultGainChance"].Float();

    auto chances = json["gainChance"].Struct();

    for(auto &node : chances)
	{
		int chance = node.second.Float();

		VLC->modh->identifiers.requestIdentifier(node.second.meta, "faction",node.first, [=](si32 factionID)
		{
			spell->probabilities[factionID] = chance;
		});
	}


    auto target_type_str = json["targetType"].String();

    if (target_type_str == "NO_TARGET")
        spell->targetType = CSpell::NO_TARGET;
    else if (target_type_str == "CREATURE")
        spell->targetType = CSpell::CREATURE;
    else if (target_type_str == "OBSTACLE")
        spell->targetType = CSpell::OBSTACLE;
    else if (target_type_str == "CREATURE_EXPERT_MASSIVE")
        spell->targetType = CSpell::CREATURE_EXPERT_MASSIVE;
    else
    {
        logGlobal->errorStream() << spell->name << ": invalid target type '" <<target_type_str<<"'";
        fatalConfigurationError();
    }

    spell->mainEffectAnim = json["anim"].Float();

    for(const auto& k_v: json["counters"].Struct())
    {
        if (k_v.second.Bool())
        {
            JsonNode tmp(JsonNode::DATA_STRING);
            tmp.meta = json.meta;
            tmp.String() = k_v.first;

            VLC->modh->identifiers.requestIdentifier(tmp,[=](si32 id){
                spell->counteredSpells.push_back(SpellID(id));
            });

        }
    }
    //TODO: more error checking - f.e. conflicting flags
    const auto flags = json["flags"];

    //by default all flags are set to false in constructor

    if (readFlag(flags,"summoning"))
    {
        logGlobal->warnStream() << spell->name << ": summoning flag in unimplemented";
    }

    spell->isDamage = readFlag(flags,"damage"); //do this before "offensive"

    if (readFlag(flags,"offensive"))
    {
        spell->setIsOffensive(true);
    }

    if (readFlag(flags,"rising"))
    {
        spell->setIsRising(true);
    }

    const bool implicit_positiveness = spell->isOffensive || spell->isRising; //(!) "damage" does not mean NEGATIVE  --AVS

    if (readFlag(flags,"indifferent"))
    {
        spell->positiveness = CSpell::NEUTRAL;
    }
    else if (readFlag(flags,"negative"))
    {
        spell->positiveness = CSpell::NEGATIVE;
    }
    else if (readFlag(flags,"positive"))
    {
        spell->positiveness = CSpell::POSITIVE;
    }
    else if(!implicit_positiveness)
    {
        spell->positiveness = CSpell::NEUTRAL; //duplicates constructor but, just in case
        logGlobal->errorStream() << "No positiveness specified, assumed NEUTRAL";
    }

    spell->isSpecial = readFlag(flags,"special");

    auto find_in_map = [&](std::string name, std::vector<Bonus::BonusType> &vec)
    {
        auto it = bonusNameMap.find(name);
        if (it == bonusNameMap.end())
        {
            logGlobal->errorStream() << spell->name << ": invalid bonus name" << name;
        }
        else
        {
            vec.push_back((Bonus::BonusType)it->second);
        }
    };

    auto read_node = [&](std::string name, std::vector<Bonus::BonusType> &vec)
    {
        const JsonNode & node = json[name];

        if (!node.isNull())
        {
            for (auto key_value: node.Struct())
            {
                const std::string bonus_id = key_value.first;
                const bool flag = key_value.second.Bool();

                if (flag)
                {
                   find_in_map(bonus_id, vec);
                }
            }
        }
    };

    read_node("immunity",spell->immunities);

    read_node("absoluteImmunity", spell->absoluteImmunities);

    read_node("limit",spell->limiters);


    const JsonNode & graphicsNode = json["graphics"];
    if (!graphicsNode.isNull())
    {
        spell->iconImmune = graphicsNode["iconImmune"].String();
    }

    //load level attributes

    const int level_count = GameConstants::SPELL_SCHOOL_LEVELS;

    spell->AIVals.resize(level_count);
    spell->costs.resize(level_count);
    spell->descriptions.resize(level_count);

    spell->powers.resize(level_count);
    spell->range.resize(level_count);


    const JsonNode & levels_node = json["levels"];

    if (levels_node.isNull())
    {
        logGlobal->errorStream() << spell->name << ": no level specific data";
        fatalConfigurationError();
    }

    if (levels_node.getType()!=JsonNode::DATA_STRUCT)
    {
        logGlobal->errorStream() << spell->name << ": level specific data shall be JSON object";
        fatalConfigurationError();
    }

    const JsonMap & levels = json["levels"].Struct();


    for(int level_idx = 0; level_idx < level_count; level_idx++)
    {
        const auto& level_node = levels.at(level_names[level_idx]);


        if (level_node.getType()!=JsonNode::DATA_STRUCT)
        {
            logGlobal->errorStream() << spell->name << ": level specific data shall be JSON object";
            fatalConfigurationError();
        }

        auto ensure_field = [&](const std::string json_name,JsonNode::JsonType type)->JsonNode
        {
            const auto& node = level_node[json_name];

            if (node.isNull())
            {
                logGlobal->errorStream() << spell->name << ": mandatory field "<<json_name<<" missing";
                fatalConfigurationError();
            }

            if (node.getType()!=type)
            {
                logGlobal->errorStream() << spell->name << ": field "<<json_name<<" - type mismatch";
                fatalConfigurationError();
            }
            return node;
        };


        auto get_string_mandatory = [&](const std::string json_name, std::vector<std::string>& target)
        {
            const auto& node = ensure_field(json_name, JsonNode::DATA_STRING);
            target[level_idx] = node.String();

        };

        auto get_string = [&](const std::string json_name, std::vector<std::string>& target)
        {
            const auto& node = level_node[json_name];
            if (node.getType() == JsonNode::DATA_STRING)
            {
                target[level_idx] = node.String();
            }
        };

        auto get_nomber = [&](const std::string json_name, std::vector<si32>& target)
        {
            const auto& node = level_node[json_name];
            if (node.getType() == JsonNode::DATA_FLOAT)
            {
                target[level_idx] = node.Float();
            }
        };

        auto get_nomber_mandatory = [&](const std::string json_name, std::vector<si32>& target)
        {
            const auto& node = ensure_field(json_name, JsonNode::DATA_FLOAT);
            target[level_idx] = node.Float();
        };

        if (spell->isCreatureAbility())
        {
            get_string("description", spell->descriptions);
            get_nomber("cost", spell->costs);
            get_nomber("power", spell->powers);
            get_nomber("aiValue", spell->AIVals);

        }
        else
        {
            get_string_mandatory("description", spell->descriptions);
            get_nomber_mandatory("cost", spell->costs);
            get_nomber_mandatory("power", spell->powers);
            get_nomber_mandatory("aiValue", spell->AIVals);
        }



        const JsonNode& effects_node = level_node["effects"];

        if (!effects_node.isNull())
        {
            if (spell->effects.empty())
                spell->effects.resize(level_count);

            for (const auto& elem : effects_node.Struct())
            {
                const JsonNode& bonus_node = elem.second;
                Bonus * b = JsonUtils::parseBonus(bonus_node);
                const bool usePowerAsValue = bonus_node["val"].isNull();

				//TODO: make this work. see CSpellHandler::afterLoadFinalization()
				//b->sid = spell->id; //for all
				b->source = Bonus::SPELL_EFFECT;//for all

				if (usePowerAsValue)
                {
                   b->val = spell->powers[level_idx];
                }

				spell->effects[level_idx].push_back(b);

            }
        }
    }

    return spell;
}

void CSpellHandler::afterLoadFinalization()
{
    //FIXME: this a bad place for this code, should refactor loadFromJson to know object id during load

    for (auto spell: objects)
        for (auto & level: spell->effects)
            for (auto * bonus: level)
                bonus->sid = spell->id;


}



CSpellHandler::~CSpellHandler()
{

}

std::vector<bool> CSpellHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedSpells;
	allowedSpells.resize(GameConstants::SPELLS_QUANTITY, true);
	return allowedSpells;
}
