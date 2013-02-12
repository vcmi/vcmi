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

TerrainViewPattern::WeightedRule::WeightedRule() : points(0)
{

}

bool TerrainViewPattern::WeightedRule::isStandardRule() const
{
	return TerrainViewPattern::RULE_ANY == name || TerrainViewPattern::RULE_DIRT == name
		|| TerrainViewPattern::RULE_NATIVE == name || TerrainViewPattern::RULE_SAND == name
		|| TerrainViewPattern::RULE_TRANSITION == name;
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
					std::vector<std::string> ruleParts;
					boost::split(ruleParts, ruleStr, boost::is_any_of("-"));
					TerrainViewPattern::WeightedRule rule;
					rule.name = ruleParts[0];
					if(ruleParts.size() > 1)
					{
						rule.points = boost::lexical_cast<int>(ruleParts[1]);
					}
					pattern.data[i].push_back(rule);
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

const TerrainViewPattern & CTerrainViewPatternConfig::getPatternById(ETerrainGroup::ETerrainGroup terGroup, const std::string & id) const
{
	const std::vector<TerrainViewPattern> & groupPatterns = getPatternsForGroup(terGroup);
	BOOST_FOREACH(const TerrainViewPattern & pattern, groupPatterns)
	{
		if(id == pattern.id)
		{
			return pattern;
		}
	}
	throw std::runtime_error("Pattern with ID not found: " + id);
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

void CMapEditManager::drawTerrain(ETerrainType terType, int posx, int posy, int width, int height, bool underground)
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

	updateTerrainViews(posx - 1, posy - 1, width + 2, height + 2, mapLevel);
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
			int bestPattern = -1, bestFlip = -1;
			std::string transitionReplacement;
			for(int k = 0; k < patterns.size(); ++k)
			{
				const TerrainViewPattern & pattern = patterns[k];

				for(int flip = 0; flip < 4; ++flip)
				{
					ValidationResult valRslt = validateTerrainView(i, j, mapLevel, flip > 0 ? getFlippedPattern(pattern, flip) : pattern);
					if(valRslt.result)
					{
						tlog5 << "Pattern detected at pos " << i << "x" << j << "x" << mapLevel << ": P-Nr. " << k
							  << ", Flip " << flip << ", Repl. " << valRslt.transitionReplacement << std::endl;

						bestPattern = k;
						bestFlip = flip;
						transitionReplacement = valRslt.transitionReplacement;
						break;
					}
				}
			}
			if(bestPattern == -1)
			{
				// This shouldn't be the case
				tlog2 << "No pattern detected at pos " << i << "x" << j << "x" << mapLevel << std::endl;
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

ETerrainGroup::ETerrainGroup CMapEditManager::getTerrainGroup(ETerrainType terType) const
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

CMapEditManager::ValidationResult CMapEditManager::validateTerrainView(int posx, int posy, int mapLevel, const TerrainViewPattern & pattern, int recDepth /*= 0*/) const
{
	ETerrainType centerTerType = map->terrain[posx][posy][mapLevel].terType;
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
		ETerrainType terType;
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
			TerrainViewPattern::WeightedRule rule = pattern.data[i][j];
			if(!rule.isStandardRule())
			{
				if(recDepth == 0)
				{
					const TerrainViewPattern & patternForRule = terViewPatternConfig->getPatternById(pattern.terGroup, rule.name);
					ValidationResult rslt = validateTerrainView(cx, cy, mapLevel, patternForRule, 1);
					if(!rslt.result)
					{
						return ValidationResult(false);
					}
					else
					{
						topPoints = std::max(topPoints, rule.points);
						continue;
					}
				}
				else
				{
					rule.name = TerrainViewPattern::RULE_NATIVE;
				}
			}

			bool nativeTestOk = (rule.name == TerrainViewPattern::RULE_NATIVE || rule.name == TerrainViewPattern::RULE_ANY) && !isAlien;
			auto applyValidationRslt = [&](bool rslt)
			{
				if(rslt)
				{
					topPoints = std::max(topPoints, rule.points);
				}
			};

			// Validate cell with the ruleset of the pattern
			if(pattern.terGroup == ETerrainGroup::NORMAL)
			{
				bool dirtTestOk = (rule.name == TerrainViewPattern::RULE_DIRT
						|| rule.name == TerrainViewPattern::RULE_TRANSITION || rule.name == TerrainViewPattern::RULE_ANY)
						&& isAlien && !isSandType(terType);
				bool sandTestOk = (rule.name == TerrainViewPattern::RULE_SAND || rule.name == TerrainViewPattern::RULE_TRANSITION
						|| rule.name == TerrainViewPattern::RULE_ANY)
						&& isSandType(terType);

				if(transitionReplacement.empty() && (rule.name == TerrainViewPattern::RULE_TRANSITION
					|| rule.name == TerrainViewPattern::RULE_ANY) && (dirtTestOk || sandTestOk))
				{
					transitionReplacement = dirtTestOk ? TerrainViewPattern::RULE_DIRT : TerrainViewPattern::RULE_SAND;
				}
				applyValidationRslt((dirtTestOk && transitionReplacement != TerrainViewPattern::RULE_SAND)
						|| (sandTestOk && transitionReplacement != TerrainViewPattern::RULE_DIRT)
						|| nativeTestOk);
			}
			else if(pattern.terGroup == ETerrainGroup::DIRT)
			{
				bool sandTestOk = rule.name == TerrainViewPattern::RULE_SAND && isSandType(terType);
				bool dirtTestOk = rule.name == TerrainViewPattern::RULE_DIRT && !isSandType(terType) && !nativeTestOk;
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || dirtTestOk || nativeTestOk);
			}
			else if(pattern.terGroup == ETerrainGroup::SAND)
			{
				bool sandTestOk = rule.name == TerrainViewPattern::RULE_SAND && isAlien;
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || nativeTestOk);
			}
			else if(pattern.terGroup == ETerrainGroup::WATER)
			{
				bool sandTestOk = rule.name == TerrainViewPattern::RULE_SAND && terType != ETerrainType::DIRT
						&& terType != ETerrainType::WATER;
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || nativeTestOk);
			}
			else if(pattern.terGroup == ETerrainGroup::ROCK)
			{
				bool sandTestOk = rule.name == TerrainViewPattern::RULE_SAND && terType != ETerrainType::DIRT
						&& terType != ETerrainType::ROCK;
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || nativeTestOk);
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

	return ValidationResult(true, transitionReplacement);
}

bool CMapEditManager::isSandType(ETerrainType terType) const
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

CMapEditManager::ValidationResult::ValidationResult(bool result, const std::string & transitionReplacement /*= ""*/)
	: result(result), transitionReplacement(transitionReplacement)
{

}
