#include "StdInc.h"
#include "CSpellHandler.h"

#include "CGeneralTextHandler.h"
#include "Filesystem/CResourceLoader.h"
#include "VCMI_Lib.h"
#include "JsonNode.h"
#include <cctype>
#include "GameConstants.h"
#include "BattleHex.h"
#include "CModHandler.h"

/*
 * CSpellHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
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

	//helper fonction for std::set<ui16> CSpell::rangeInHexes(unsigned int centralHex, ui8 schoolLvl ) const
	static std::set<ui16> getInRange(unsigned int center, int low, int high)
	{
		std::set<ui16> ret;
		if(low == 0)
		{
			ret.insert(center);
		}

		std::pair<int, int> mainPointForLayer[6]; //A, B, C, D, E, F points
		for(int b=0; b<6; ++b)
			mainPointForLayer[b] = hexToPair(center);

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
CSpellHandler::CSpellHandler()
{
}

CSpell::CSpell()
{
	_isDamage = false;
	_isMind = false;
	_isRising = false;
}

std::vector<BattleHex> CSpell::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, ui8 side, bool *outDroppedHexes) const
{
	std::vector<BattleHex> ret;

	if(id == Spells::FIRE_WALL  ||  id == Spells::FORCE_FIELD)
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
			addIfValid(centralHex.moveInDir(firstStep, false).moveInDir(secondStep, false));

		return ret;
	}


	std::string rng = range[schoolLvl] + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 1 && rng[0] != 'X') //there is at lest one hex in range
	{
		std::string number1, number2;
		int beg, end;
		bool readingFirst = true;
		for(int it=0; it<rng.size(); ++it)
		{
			if( std::isdigit(rng[it]) ) //reading numer
			{
				if(readingFirst)
					number1 += rng[it];
				else
					number2 += rng[it];
			}
			else if(rng[it] == ',') //comma
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
				for(std::set<ui16>::iterator it = curLayer.begin(); it != curLayer.end(); ++it)
				{
					ret.push_back(*it);
				}

			}
			else if(rng[it] == '-') //dash
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

CSpell::ETargetType CSpell::getTargetType() const //TODO: parse these at game launch
{
	if(attributes.find("CREATURE_TARGET_1") != std::string::npos
		|| attributes.find("CREATURE_TARGET_2") != std::string::npos)
		return CREATURE_EXPERT_MASSIVE;

	if(attributes.find("CREATURE_TARGET") != std::string::npos)
		return CREATURE;

	if(attributes.find("OBSTACLE_TARGET") != std::string::npos)
		return OBSTACLE;

	return NO_TARGET;
}

bool CSpell::isPositive() const
{
	return positiveness == POSITIVE;
}

bool CSpell::isNegative() const
{
	return positiveness == NEGATIVE;
}

bool CSpell::isRisingSpell() const
{
	return _isRising;
}

bool CSpell::isDamageSpell() const
{
	return _isDamage;
}

bool CSpell::isMindSpell() const
{
	return _isMind;
}

void CSpell::getEffects(std::vector<Bonus>& lst) const
{
	lst.reserve(lst.size() + _effects.size());

	BOOST_FOREACH (Bonus b, _effects)
	{
		lst.push_back(b);
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

CSpell * CSpellHandler::loadSpell(CLegacyConfigParser & parser)
{
	CSpell * spell = new CSpell; //new currently being read spell

	spell->name    = parser.readString();
	spell->abbName = parser.readString();
	spell->level   = parser.readNumber();
	spell->earth   = parser.readString() == "x";
	spell->water   = parser.readString() == "x";
	spell->fire    = parser.readString() == "x";
	spell->air     = parser.readString() == "x";

	spell->costs = parser.readNumArray<si32>(4);

	spell->power = parser.readNumber();
	spell->powers = parser.readNumArray<si32>(4);

	for (int i = 0; i < 9 ; i++)
		spell->probabilities[i] = parser.readNumber();

	spell->AIVals = parser.readNumArray<si32>(4);

	for (int i = 0; i < 4 ; i++)
		spell->descriptions.push_back(parser.readString());

	spell->attributes = parser.readString();
	spell->mainEffectAnim = -1;
	return spell;
}

void CSpellHandler::loadSpells()
{
	CLegacyConfigParser parser("DATA/SPTRAITS.TXT");

	for(int i=0; i<5; i++) // header
		parser.endLine();

	do //read adventure map spells
	{
		CSpell * spell = loadSpell(parser);
		spell->id = spells.size();
		spell->combatSpell = false;
		spell->creatureAbility = false;
		spells.push_back(spell);
	}
	while (parser.endLine() && !parser.isNextEntryEmpty());

	for(int i=0; i<3; i++)
		parser.endLine();

	do //read battle spells
	{
		CSpell * spell = loadSpell(parser);
		spell->id = spells.size();
		spell->combatSpell = true;
		spell->creatureAbility = false;
		spells.push_back(spell);
	}
	while (parser.endLine() && !parser.isNextEntryEmpty());

	for(int i=0; i<3; i++)
		parser.endLine();

	do //read creature abilities
	{
		CSpell * spell = loadSpell(parser);
		spell->id = spells.size();
		spell->combatSpell = true;
		spell->creatureAbility = true;
		spells.push_back(spell);
	}
	while (parser.endLine() && !parser.isNextEntryEmpty());

	boost::replace_first (spells[Spells::DISRUPTING_RAY]->attributes, "2", ""); // disrupting ray will now affect single creature


	spells.push_back(spells[Spells::ACID_BREATH_DEFENSE]); //clone Acid Breath attributes for Acid Breath damage effect

	//loading of additional spell traits
	const JsonNode config(ResourceID("config/spell_info.json"));

	BOOST_FOREACH(auto &spell, config["spells"].Struct())
	{
		//reading exact info
		int spellID = spell.second["id"].Float();
		CSpell *s = spells[spellID];

		s->positiveness = spell.second["effect"].Float();
		s->mainEffectAnim = spell.second["anim"].Float();

		s->range.resize(4);
		int idx = 0;
		BOOST_FOREACH(const JsonNode &range, spell.second["ranges"].Vector())
			s->range[idx++] = range.String();

		s->counteredSpells = spell.second["counters"].convertTo<std::vector<TSpell> >();

		s->identifier = spell.first;
		VLC->modh->identifiers.registerObject("spell." + spell.first, spellID);

		const JsonNode & flags_node = spell.second["flags"];
		if (!flags_node.isNull())
		{
			auto flags = flags_node.convertTo<std::vector<std::string> >();

			BOOST_FOREACH (const auto & flag, flags)
			{
				if (flag == "damage")
				{
					s->_isDamage = true;
				}
				else if (flag == "rising")
				{
					s->_isRising = true;
				}
				else if (flag == "mind")
				{
					s->_isMind = true;
				}

			}
		}

		const JsonNode & effects_node = spell.second["effects"];

		if (!effects_node.isNull())
		{
			BOOST_FOREACH (const JsonNode & bonus_node, effects_node.Vector())
			{
				Bonus * b = JsonUtils::parseBonus(bonus_node);
				b->sid = s->id;
				b->source = Bonus::SPELL_EFFECT;
				b->duration = Bonus::N_TURNS; //default
				//TODO: make duration configurable
				s->_effects.push_back(*b);
			}
		}

	}

	//spell fixes

	//forgetfulness needs to get targets automatically on expert level
	boost::replace_first(spells[Spells::FORGETFULNESS]->attributes, "CREATURE_TARGET", "CREATURE_TARGET_2"); //TODO: use flags instead?
}

std::vector<ui8> CSpellHandler::getDefaultAllowedSpells() const
{
	std::vector<ui8> allowedSpells;
	allowedSpells.resize(GameConstants::SPELLS_QUANTITY, 1);
	return allowedSpells;
}
