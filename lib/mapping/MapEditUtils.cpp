/*
 * MapEditUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "MapEditUtils.h"

#include "../filesystem/Filesystem.h"
#include "../JsonNode.h"
#include "../TerrainHandler.h"
#include "CMap.h"
#include "CMapOperation.h"

VCMI_LIB_NAMESPACE_BEGIN

MapRect::MapRect() : x(0), y(0), z(0), width(0), height(0)
{

}

MapRect::MapRect(const int3 & pos, si32 width, si32 height): x(pos.x), y(pos.y), z(pos.z), width(width), height(height)
{
}

MapRect MapRect::operator&(const MapRect& rect) const
{
	bool intersect = right() > rect.left() && rect.right() > left()
		&& bottom() > rect.top() && rect.bottom() > top()
		&& z == rect.z;
	if (intersect)
	{
		MapRect ret;
		ret.x = std::max(left(), rect.left());
		ret.y = std::max(top(), rect.top());
		ret.z = rect.z;
		ret.width = std::min(right(), rect.right()) - ret.x;
		ret.height = std::min(bottom(), rect.bottom()) - ret.y;
		return ret;
	}
	else
	{
		return MapRect();
	}
}

si32 MapRect::left() const
{
	return x;
}

si32 MapRect::right() const
{
	return x + width;
}

si32 MapRect::top() const
{
	return y;
}

si32 MapRect::bottom() const
{
	return y + height;
}

int3 MapRect::topLeft() const
{
	return int3(x, y, z);
}

int3 MapRect::topRight() const
{
	return int3(right(), y, z);
}

int3 MapRect::bottomLeft() const
{
	return int3(x, bottom(), z);
}

int3 MapRect::bottomRight() const
{
	return int3(right(), bottom(), z);
}

CTerrainSelection::CTerrainSelection(CMap * map) : CMapSelection(map)
{

}

void CTerrainSelection::selectRange(const MapRect& rect)
{
	rect.forEach([this](const int3 & pos) 
	{
		this->select(pos); 
	});
}

void CTerrainSelection::deselectRange(const MapRect& rect)
{
	rect.forEach([this](const int3 & pos)
	{
		this->deselect(pos);
	});
}

void CTerrainSelection::setSelection(const std::vector<int3> & vec)
{
	for (const auto & pos : vec)
		this->select(pos);
}

void CTerrainSelection::selectAll()
{
	selectRange(MapRect(int3(0, 0, 0), getMap()->width, getMap()->height));
	selectRange(MapRect(int3(0, 0, 1), getMap()->width, getMap()->height));
}

void CTerrainSelection::clearSelection()
{
	deselectRange(MapRect(int3(0, 0, 0), getMap()->width, getMap()->height));
	deselectRange(MapRect(int3(0, 0, 1), getMap()->width, getMap()->height));
}

CObjectSelection::CObjectSelection(CMap * map) : CMapSelection(map)
{

}

const std::string TerrainViewPattern::FLIP_MODE_DIFF_IMAGES = "D";

const std::string TerrainViewPattern::RULE_DIRT = "D";
const std::string TerrainViewPattern::RULE_SAND = "S";
const std::string TerrainViewPattern::RULE_TRANSITION = "T";
const std::string TerrainViewPattern::RULE_NATIVE = "N";
const std::string TerrainViewPattern::RULE_NATIVE_STRONG = "N!";
const std::string TerrainViewPattern::RULE_ANY = "?";

TerrainViewPattern::TerrainViewPattern()
	: diffImages(false)
	, rotationTypesCount(0)
	, minPoints(0)
	, maxPoints(std::numeric_limits<int>::max())
{
}

TerrainViewPattern::WeightedRule::WeightedRule(std::string & Name)
	: points(0)
	, name(Name)
	, standardRule(TerrainViewPattern::RULE_ANY == Name || TerrainViewPattern::RULE_DIRT == Name || TerrainViewPattern::RULE_NATIVE == Name ||
				   TerrainViewPattern::RULE_SAND == Name || TerrainViewPattern::RULE_TRANSITION == Name || TerrainViewPattern::RULE_NATIVE_STRONG == Name)
	, anyRule(Name == TerrainViewPattern::RULE_ANY)
	, dirtRule(Name == TerrainViewPattern::RULE_DIRT)
	, sandRule(Name == TerrainViewPattern::RULE_SAND)
	, transitionRule(Name == TerrainViewPattern::RULE_TRANSITION)
	, nativeStrongRule(Name == TerrainViewPattern::RULE_NATIVE_STRONG)
	, nativeRule(Name == TerrainViewPattern::RULE_NATIVE)
{
}

void TerrainViewPattern::WeightedRule::setNative()
{
	nativeRule = true;
	standardRule = true;
	//TODO: would look better as a bitfield
	dirtRule = sandRule = transitionRule = nativeStrongRule = anyRule = false; //no idea what they mean, but look mutually exclusive
}

CTerrainViewPatternConfig::CTerrainViewPatternConfig()
{
	const JsonNode config(ResourcePath("config/terrainViewPatterns.json"));
	static const std::string patternTypes[] = { "terrainView", "terrainType" };
	for (int i = 0; i < std::size(patternTypes); ++i)
	{
		const auto& patternsVec = config[patternTypes[i]].Vector();
		for (const auto& ptrnNode : patternsVec)
		{
			TerrainViewPattern pattern;

			// Read pattern data
			const JsonVector& data = ptrnNode["data"].Vector();
			assert(data.size() == 9);
			for (int j = 0; j < data.size(); ++j)
			{
				std::string cell = data[j].String();
				boost::algorithm::erase_all(cell, " ");
				std::vector<std::string> rules;
				boost::split(rules, cell, boost::is_any_of(","));
				for (const std::string & ruleStr : rules)
				{
					std::vector<std::string> ruleParts;
					boost::split(ruleParts, ruleStr, boost::is_any_of("-"));
					TerrainViewPattern::WeightedRule rule(ruleParts[0]);
					assert(!rule.name.empty());
					if (ruleParts.size() > 1)
					{
						rule.points = boost::lexical_cast<int>(ruleParts[1]);
					}
					pattern.data[j].push_back(rule);
				}
			}

			// Read various properties
			pattern.id = ptrnNode["id"].String();
			assert(!pattern.id.empty());
			pattern.minPoints = static_cast<int>(ptrnNode["minPoints"].Float());
			pattern.maxPoints = static_cast<int>(ptrnNode["maxPoints"].Float());
			if (pattern.maxPoints == 0)
				pattern.maxPoints = std::numeric_limits<int>::max();

			// Read mapping
			if (i == 0)
			{
				const auto & mappingStruct = ptrnNode["mapping"].Struct();
				for (const auto & mappingPair : mappingStruct)
				{
					TerrainViewPattern terGroupPattern = pattern;
					auto mappingStr = mappingPair.second.String();
					boost::algorithm::erase_all(mappingStr, " ");
					auto colonIndex = mappingStr.find_first_of(':');
					const auto & flipMode = mappingStr.substr(0, colonIndex);
					terGroupPattern.diffImages = TerrainViewPattern::FLIP_MODE_DIFF_IMAGES == &(flipMode[flipMode.length() - 1]);
					if (terGroupPattern.diffImages)
					{
						terGroupPattern.rotationTypesCount = boost::lexical_cast<int>(flipMode.substr(0, flipMode.length() - 1));
						assert(terGroupPattern.rotationTypesCount == 2 || terGroupPattern.rotationTypesCount == 4);
					}
					mappingStr = mappingStr.substr(colonIndex + 1);
					std::vector<std::string> mappings;
					boost::split(mappings, mappingStr, boost::is_any_of(","));
					for (const std::string & mapping : mappings)
					{
						std::vector<std::string> range;
						boost::split(range, mapping, boost::is_any_of("-"));
						terGroupPattern.mapping.emplace_back(std::stoi(range[0]), std::stoi(range.size() > 1 ? range[1] : range[0]));
					}

					// Add pattern to the patterns map
					std::vector<TerrainViewPattern> terrainViewPatternFlips{terGroupPattern};

					for (int i = 1; i < 4; ++i)
					{
						//auto p = terGroupPattern;
						flipPattern(terGroupPattern, i); //FIXME: we flip in place - doesn't make much sense now, but used to work
						terrainViewPatternFlips.push_back(terGroupPattern);
					}

					terrainViewPatterns[mappingPair.first].push_back(terrainViewPatternFlips);
				}
			}
			else if (i == 1)
			{
				terrainTypePatterns[pattern.id].push_back(pattern);
				for (int i = 1; i < 4; ++i)
				{
					//auto p = pattern;
					flipPattern(pattern, i); ///FIXME: we flip in place - doesn't make much sense now
					terrainTypePatterns[pattern.id].push_back(pattern);
				}
			}
		}
	}
}

const std::vector<CTerrainViewPatternConfig::TVPVector> & CTerrainViewPatternConfig::getTerrainViewPatterns(TerrainId terrain) const
{
	auto iter = terrainViewPatterns.find(VLC->terrainTypeHandler->getById(terrain)->terrainViewPatterns);
	if (iter == terrainViewPatterns.end())
		return terrainViewPatterns.at("normal");
	return iter->second;
}

std::optional<const std::reference_wrapper<const TerrainViewPattern>> CTerrainViewPatternConfig::getTerrainViewPatternById(const std::string & patternId, const std::string & id) const
{
	auto iter = terrainViewPatterns.find(patternId);
	const auto & groupPatterns = (iter == terrainViewPatterns.end()) ? terrainViewPatterns.at("normal") : iter->second;

	for(const auto & patternFlips : groupPatterns)
	{
		const auto & pattern = patternFlips.front();

		if (id == pattern.id)
		{
			return {std::ref(pattern)};
		}
	}
	return {};
}

std::optional<const std::reference_wrapper<const CTerrainViewPatternConfig::TVPVector>> CTerrainViewPatternConfig::getTerrainViewPatternsById(TerrainId terrain, const std::string & id) const
{
	const auto & groupPatterns = getTerrainViewPatterns(terrain);
	for(const auto & patternFlips : groupPatterns)
	{
		const auto & pattern = patternFlips.front();
		if (id == pattern.id)
		{
			return {std::ref(patternFlips)};
		}
	}
	return {};
}


const CTerrainViewPatternConfig::TVPVector* CTerrainViewPatternConfig::getTerrainTypePatternById(const std::string& id) const
{
	auto it = terrainTypePatterns.find(id);
	assert(it != terrainTypePatterns.end());
	return &(it->second);
}

void CTerrainViewPatternConfig::flipPattern(TerrainViewPattern & pattern, int flip) const
{
	//flip in place to avoid expensive constructor. Seriously.

	if (flip == 0)
	{
		return;
	}

	//always flip horizontal
	for (int i = 0; i < 3; ++i)
	{
		int y = i * 3;
		std::swap(pattern.data[y], pattern.data[y + 2]);
	}
	//flip vertical only at 2nd step
	if (flip == CMapOperation::FLIP_PATTERN_VERTICAL)
	{
		for (int i = 0; i < 3; ++i)
		{
			std::swap(pattern.data[i], pattern.data[6 + i]);
		}
	}
}

void CTerrainViewPatternUtils::printDebuggingInfoAboutTile(const CMap * map, const int3 & pos)
{
	logGlobal->debug("Printing detailed info about nearby map tiles of pos '%s'", pos.toString());
	for (int y = pos.y - 2; y <= pos.y + 2; ++y)
	{
		std::string line;
		const int PADDED_LENGTH = 10;
		for (int x = pos.x - 2; x <= pos.x + 2; ++x)
		{
			auto debugPos = int3(x, y, pos.z);
			if (map->isInTheMap(debugPos))
			{
				auto debugTile = map->getTile(debugPos);

				std::string terType = debugTile.terType->shortIdentifier;
				line += terType;
				line.insert(line.end(), PADDED_LENGTH - terType.size(), ' ');
			}
			else
			{
				line += "X";
				line.insert(line.end(), PADDED_LENGTH - 1, ' ');
			}
		}

		logGlobal->debug(line);
	}
}

VCMI_LIB_NAMESPACE_END
