#include "StdInc.h"
#include "CMapEditManager.h"

#include "../JsonNode.h"
#include "../Filesystem/CResourceLoader.h"
#include "../CDefObjInfoHandler.h"

const std::string TerrainViewPattern::FLIP_MODE_SAME_IMAGE = "sameImage";
const std::string TerrainViewPattern::FLIP_MODE_DIFF_IMAGES = "diffImages";

const std::string TerrainViewPattern::RULE_DIRT = "D";
const std::string TerrainViewPattern::RULE_SAND = "S";
const std::string TerrainViewPattern::RULE_TRANSITION = "T";
const std::string TerrainViewPattern::RULE_NATIVE = "N";
const std::string TerrainViewPattern::RULE_ANY = "?";

TerrainViewPattern::TerrainViewPattern() : minPoints(0), flipMode(FLIP_MODE_SAME_IMAGE),
	terGroup(ETerrainGroup::NORMAL)
{

}

CTerrainViewPatternConfig::CTerrainViewPatternConfig()
{
	const JsonNode config(ResourceID("config/terrainViewPatterns.json"));
	const std::map<std::string, ETerrainGroup::ETerrainGroup> terGroups
			= boost::assign::map_list_of("normal", ETerrainGroup::NORMAL)("dirt", ETerrainGroup::DIRT)
			("sand", ETerrainGroup::SAND)("water", ETerrainGroup::WATER)("rock", ETerrainGroup::ROCK);
	BOOST_FOREACH(auto terMapping, terGroups)
	{
		BOOST_FOREACH(const JsonNode & ptrnNode, config[terMapping.first].Vector())
		{
			TerrainViewPattern pattern;

			// Read pattern data
			const JsonVector & data = ptrnNode["data"].Vector();
			if(data.size() != 9)
			{
				throw std::runtime_error("Size of pattern's data vector has to be 9.");
			}
			for(int i = 0; i < data.size(); ++i)
			{
				std::string cell = data[i].String();
				boost::algorithm::erase_all(cell, " ");
				std::vector<std::string> rules;
				boost::split(rules, cell, boost::is_any_of(","));
				BOOST_FOREACH(std::string ruleStr, rules)
				{
					std::vector<std::string> rule;
					boost::split(rule, ruleStr, boost::is_any_of("-"));
					std::pair<std::string, int> pair;
					pair.first = rule[0];
					if(rule.size() > 1)
					{
						pair.second = boost::lexical_cast<int>(rule[1]);
					}
					else
					{
						pair.second = 0;
					}
					pattern.data[i].push_back(pair);
				}
			}

			// Read mapping
			std::string mappingStr = ptrnNode["mapping"].String();
			boost::algorithm::erase_all(mappingStr, " ");
			std::vector<std::string> mappings;
			boost::split(mappings, mappingStr, boost::is_any_of(","));
			BOOST_FOREACH(std::string mapping, mappings)
			{
				std::vector<std::string> range;
				boost::split(range, mapping, boost::is_any_of("-"));
				pattern.mapping.push_back(std::make_pair(boost::lexical_cast<int>(range[0]),
					boost::lexical_cast<int>(range.size() > 1 ? range[1] : range[0])));
			}

			// Read optional attributes
			pattern.id = ptrnNode["id"].String();
			pattern.minPoints = static_cast<int>(ptrnNode["minPoints"].Float());
			pattern.flipMode = ptrnNode["flipMode"].String();
			if(pattern.flipMode.empty())
			{
				pattern.flipMode = TerrainViewPattern::FLIP_MODE_SAME_IMAGE;
			}

			pattern.terGroup = terMapping.second;
			patterns[terMapping.second].push_back(pattern);
		}
	}
}

const std::vector<TerrainViewPattern> & CTerrainViewPatternConfig::getPatternsForGroup(ETerrainGroup::ETerrainGroup terGroup) const
{
	return patterns.find(terGroup)->second;
}

CMapEditManager::CMapEditManager(const CTerrainViewPatternConfig * terViewPatternConfig, CMap * map, int randomSeed /*= std::time(nullptr)*/)
	: map(map), terViewPatternConfig(terViewPatternConfig)
{
	gen.seed(randomSeed);
}

void CMapEditManager::clearTerrain()
{
	for(int i = 0; i < map->width; ++i)
	{
		for(int j = 0; j < map->height; ++j)
		{
			map->terrain[i][j][0].terType = ETerrainType::WATER;
			map->terrain[i][j][0].terView = gen.getInteger(20, 32);

			if(map->twoLevel)
			{
				map->terrain[i][j][1].terType = ETerrainType::ROCK;
				map->terrain[i][j][1].terView = 0;
			}
		}
	}
}

void CMapEditManager::drawTerrain(ETerrainType::ETerrainType terType, int posx, int posy, int width, int height, bool underground)
{
	bool mapLevel = underground ? 1 : 0;
	for(int i = posx; i < posx + width; ++i)
	{
		for(int j = posy; j < posy + height; ++j)
		{
			map->terrain[i][j][mapLevel].terType = terType;
		}
	}

	//TODO there are situations where more tiles are affected implicitely
	//TODO add coastal bit to extTileFlags appropriately

	//updateTerrainViews(posx - 1, posy - 1, width + 2, height + 2, mapLevel);
}

void CMapEditManager::updateTerrainViews(int posx, int posy, int width, int height, int mapLevel)
{
	for(int i = posx; i < posx + width; ++i)
	{
		for(int j = posy; j < posy + height; ++j)
		{
			const std::vector<TerrainViewPattern> & patterns =
					terViewPatternConfig->getPatternsForGroup(getTerrainGroup(map->terrain[i][j][mapLevel].terType));

			// Detect a pattern which fits best
			int totalPoints, bestPattern, bestFlip = -1;
			std::string transitionReplacement;
			for(int i = 0; i < patterns.size(); ++i)
			{
				const TerrainViewPattern & pattern = patterns[i];

				for(int flip = 0; flip < 3; ++flip)
				{
					ValidationResult valRslt = validateTerrainView(i, j, mapLevel, flip > 0 ? getFlippedPattern(pattern, flip) : pattern);
					if(valRslt.result)
					{
						if(valRslt.points > totalPoints)
						{
							totalPoints = valRslt.points;
							bestPattern = i;
							bestFlip = flip;
							transitionReplacement = valRslt.transitionReplacement;
						}
						break;
					}
				}
			}
			if(bestPattern == -1)
			{
				continue;
			}

			// Get mapping
			const TerrainViewPattern & pattern = patterns[bestPattern];
			std::pair<int, int> mapping;
			if(transitionReplacement.empty())
			{
				mapping = pattern.mapping[0];
			}
			else
			{
				mapping = transitionReplacement == TerrainViewPattern::RULE_DIRT ? pattern.mapping[0] : pattern.mapping[1];
			}

			// Set terrain view
			if(pattern.flipMode == TerrainViewPattern::FLIP_MODE_SAME_IMAGE)
			{
				map->terrain[i][j][mapLevel].terView = gen.getInteger(mapping.first, mapping.second);
				map->terrain[i][j][mapLevel].extTileFlags = bestFlip;
			}
			else
			{
				int range = (mapping.second - mapping.first) / 4;
				map->terrain[i][j][mapLevel].terView = gen.getInteger(mapping.first + bestFlip * range,
					mapping.first + (bestFlip + 1) * range - 1);
				map->terrain[i][j][mapLevel].extTileFlags =	0;
			}
		}
	}
}

ETerrainGroup::ETerrainGroup CMapEditManager::getTerrainGroup(ETerrainType::ETerrainType terType) const
{
	switch(terType)
	{
	case ETerrainType::DIRT:
		return ETerrainGroup::DIRT;
	case ETerrainType::SAND:
		return ETerrainGroup::SAND;
	case ETerrainType::WATER:
		return ETerrainGroup::WATER;
	case ETerrainType::ROCK:
		return ETerrainGroup::ROCK;
	default:
		return ETerrainGroup::NORMAL;
	}
}

CMapEditManager::ValidationResult CMapEditManager::validateTerrainView(int posx, int posy, int mapLevel, const TerrainViewPattern & pattern) const
{
	ETerrainType::ETerrainType centerTerType = map->terrain[posx][posy][mapLevel].terType;
	int totalPoints = 0;
	std::string transitionReplacement;

	for(int i = 0; i < 9; ++i)
	{
		// The center, middle cell can be skipped
		if(i == 4)
		{
			continue;
		}

		// Get terrain group of the current cell
		int cx = posx + (i % 3) - 1;
		int cy = posy + (i / 3) - 1;
		bool isAlien = false;
		ETerrainType::ETerrainType terType;
		if(cx < 0 || cx >= map->width || cy < 0 || cy >= map->height)
		{
			terType = centerTerType;
		}
		else
		{
			terType = map->terrain[cx][cy][mapLevel].terType;
			if(terType != centerTerType)
			{
				isAlien = true;
			}
		}

		// Validate all rules per cell
		int topPoints = -1;
		for(int j = 0; j < pattern.data[i].size(); ++j)
		{
			const std::pair<std::string, int> & rulePair = pattern.data[i][j];
			const std::string & rule = rulePair.first;
			bool isNative = (rule == TerrainViewPattern::RULE_NATIVE || rule == TerrainViewPattern::RULE_ANY) && !isAlien;
			auto validationRslt = [&](bool rslt)
			{
				if(rslt)
				{
					topPoints = std::max(topPoints, rulePair.second);
				}
				return rslt;
			};

			// Validate cell with the ruleset of the pattern
			bool validation;
			if(pattern.terGroup == ETerrainGroup::NORMAL)
			{
				bool isDirt = (rule == TerrainViewPattern::RULE_DIRT
						|| rule == TerrainViewPattern::RULE_TRANSITION || rule == TerrainViewPattern::RULE_ANY)
						&& isAlien && !isSandType(terType);
				bool isSand = (rule == TerrainViewPattern::RULE_SAND || rule == TerrainViewPattern::RULE_TRANSITION
						|| rule == TerrainViewPattern::RULE_ANY)
						&& isSandType(terType);

				if(transitionReplacement.empty() && (rule == TerrainViewPattern::RULE_TRANSITION
					|| rule == TerrainViewPattern::RULE_ANY) && (isDirt || isSand))
				{
					transitionReplacement = isDirt ? TerrainViewPattern::RULE_DIRT : TerrainViewPattern::RULE_SAND;
				}
				validation = validationRslt((isDirt && transitionReplacement != TerrainViewPattern::RULE_SAND)
						|| (isSand && transitionReplacement != TerrainViewPattern::RULE_DIRT)
						|| isNative);
			}
			else if(pattern.terGroup == ETerrainGroup::DIRT)
			{
				bool isSand = rule == TerrainViewPattern::RULE_SAND && isSandType(terType);
				bool isDirt = rule == TerrainViewPattern::RULE_DIRT && !isSandType(terType) && !isNative;
				validation = validationRslt(rule == TerrainViewPattern::RULE_ANY || isSand || isDirt || isNative);
			}
			else if(pattern.terGroup == ETerrainGroup::SAND || pattern.terGroup == ETerrainGroup::WATER ||
					pattern.terGroup == ETerrainGroup::ROCK)
			{
				bool isSand = rule == TerrainViewPattern::RULE_SAND && isSandType(terType) && !isNative;
				validation = validationRslt(rule == TerrainViewPattern::RULE_ANY || isSand || isNative);
			}
			if(!validation)
			{
				return ValidationResult(false);
			}
		}

		if(topPoints == -1)
		{
			return ValidationResult(false);
		}
		else
		{
			totalPoints += topPoints;
		}
	}

	if(pattern.minPoints > totalPoints)
	{
		return ValidationResult(false);
	}

	return ValidationResult(true, totalPoints, transitionReplacement);
}

bool CMapEditManager::isSandType(ETerrainType::ETerrainType terType) const
{
	switch(terType)
	{
	case ETerrainType::WATER:
	case ETerrainType::SAND:
	case ETerrainType::ROCK:
		return true;
	default:
		return false;
	}
}

TerrainViewPattern CMapEditManager::getFlippedPattern(const TerrainViewPattern & pattern, int flip) const
{
	if(flip == 0)
	{
		return pattern;
	}

	TerrainViewPattern ret = pattern;
	if(flip == FLIP_PATTERN_HORIZONTAL || flip == FLIP_PATTERN_BOTH)
	{
		for(int i = 0; i < 3; ++i)
		{
			int y = i * 3;
			std::swap(ret.data[y], ret.data[y + 2]);
		}
	}
	if(flip == FLIP_PATTERN_VERTICAL || flip == FLIP_PATTERN_BOTH)
	{
		for(int i = 0; i < 3; ++i)
		{
			std::swap(ret.data[i], ret.data[6 + i]);
		}
	}

	return ret;
}

void CMapEditManager::insertObject(CGObjectInstance * obj, int posx, int posy, bool underground)
{
	obj->pos = int3(posx, posy, underground ? 1 : 0);
	obj->id = map->objects.size();
	map->objects.push_back(obj);
	if(obj->ID == Obj::TOWN)
	{
		map->towns.push_back(static_cast<CGTownInstance *>(obj));
	}
	if(obj->ID == Obj::HERO)
	{
		map->heroes.push_back(static_cast<CGHeroInstance*>(obj));
	}
	map->addBlockVisTiles(obj);
}

CMapEditManager::ValidationResult::ValidationResult(bool result, int points /*= 0*/, const std::string & transitionReplacement /*= ""*/)
	: result(result), points(points), transitionReplacement(transitionReplacement)
{

}
