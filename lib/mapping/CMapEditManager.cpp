#include "StdInc.h"
#include "CMapEditManager.h"

#include "../JsonNode.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../VCMI_Lib.h"
#include "CDrawRoadsOperation.h"
#include "../mapping/CMap.h"

MapRect::MapRect() : x(0), y(0), z(0), width(0), height(0)
{

}

MapRect::MapRect(int3 pos, si32 width, si32 height) : x(pos.x), y(pos.y), z(pos.z), width(width), height(height)
{

}

MapRect MapRect::operator&(const MapRect & rect) const
{
	bool intersect = right() > rect.left() && rect.right() > left() &&
			bottom() > rect.top() && rect.bottom() > top() &&
			z == rect.z;
	if(intersect)
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

void CTerrainSelection::selectRange(const MapRect & rect)
{
	rect.forEach([this](const int3 pos)
	{
		this->select(pos);
	});
}

void CTerrainSelection::deselectRange(const MapRect & rect)
{
	rect.forEach([this](const int3 pos)
	{
		this->deselect(pos);
	});
}

void CTerrainSelection::setSelection(std::vector<int3> & vec)
{
	for (auto pos : vec)
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

CMapOperation::CMapOperation(CMap * map) : map(map)
{

}

std::string CMapOperation::getLabel() const
{
	return "";
}


MapRect CMapOperation::extendTileAround(const int3 & centerPos) const
{
	return MapRect(int3(centerPos.x - 1, centerPos.y - 1, centerPos.z), 3, 3);
}

MapRect CMapOperation::extendTileAroundSafely(const int3 & centerPos) const
{
	return extendTileAround(centerPos) & MapRect(int3(0, 0, centerPos.z), map->width, map->height);
}


CMapUndoManager::CMapUndoManager() : undoRedoLimit(10)
{

}

void CMapUndoManager::undo()
{
	doOperation(undoStack, redoStack, true);
}

void CMapUndoManager::redo()
{
	doOperation(redoStack, undoStack, false);
}

void CMapUndoManager::clearAll()
{
	undoStack.clear();
	redoStack.clear();
}

int CMapUndoManager::getUndoRedoLimit() const
{
	return undoRedoLimit;
}

void CMapUndoManager::setUndoRedoLimit(int value)
{
	assert(value >= 0);
	undoStack.resize(std::min(undoStack.size(), static_cast<TStack::size_type>(value)));
	redoStack.resize(std::min(redoStack.size(), static_cast<TStack::size_type>(value)));
}

const CMapOperation * CMapUndoManager::peekRedo() const
{
	return peek(redoStack);
}

const CMapOperation * CMapUndoManager::peekUndo() const
{
	return peek(undoStack);
}

void CMapUndoManager::addOperation(std::unique_ptr<CMapOperation> && operation)
{
	undoStack.push_front(std::move(operation));
	if(undoStack.size() > undoRedoLimit) undoStack.pop_back();
	redoStack.clear();
}

void CMapUndoManager::doOperation(TStack & fromStack, TStack & toStack, bool doUndo)
{
	if(fromStack.empty()) return;
	auto & operation = fromStack.front();
	if(doUndo)
	{
		operation->undo();
	}
	else
	{
		operation->redo();
	}
	toStack.push_front(std::move(operation));
	fromStack.pop_front();
}

const CMapOperation * CMapUndoManager::peek(const TStack & stack) const
{
	if(stack.empty()) return nullptr;
	return stack.front().get();
}

CMapEditManager::CMapEditManager(CMap * map)
	: map(map), terrainSel(map), objectSel(map)
{

}

CMap * CMapEditManager::getMap()
{
	return map;
}

void CMapEditManager::clearTerrain(CRandomGenerator * gen/* = nullptr*/)
{
	execute(make_unique<CClearTerrainOperation>(map, gen ? gen : &(this->gen)));
}

void CMapEditManager::drawTerrain(ETerrainType terType, CRandomGenerator * gen/* = nullptr*/)
{
	execute(make_unique<CDrawTerrainOperation>(map, terrainSel, terType, gen ? gen : &(this->gen)));
	terrainSel.clearSelection();
}

void CMapEditManager::drawRoad(ERoadType::ERoadType roadType, CRandomGenerator* gen)
{
	execute(make_unique<CDrawRoadsOperation>(map, terrainSel, roadType, gen ? gen : &(this->gen)));
	terrainSel.clearSelection();
}


void CMapEditManager::insertObject(CGObjectInstance * obj, const int3 & pos)
{
	execute(make_unique<CInsertObjectOperation>(map, obj, pos));
}

void CMapEditManager::execute(std::unique_ptr<CMapOperation> && operation)
{
	operation->execute();
	undoManager.addOperation(std::move(operation));
}

CTerrainSelection & CMapEditManager::getTerrainSelection()
{
	return terrainSel;
}

CObjectSelection & CMapEditManager::getObjectSelection()
{
	return objectSel;
}

CMapUndoManager & CMapEditManager::getUndoManager()
{
	return undoManager;
}

CComposedOperation::CComposedOperation(CMap * map) : CMapOperation(map)
{

}

void CComposedOperation::execute()
{
	for(auto & operation : operations)
	{
		operation->execute();
	}
}

void CComposedOperation::undo()
{
	for(auto & operation : operations)
	{
		operation->undo();
	}
}

void CComposedOperation::redo()
{
	for(auto & operation : operations)
	{
		operation->redo();
	}
}

void CComposedOperation::addOperation(std::unique_ptr<CMapOperation> && operation)
{
	operations.push_back(std::move(operation));
}

const std::string TerrainViewPattern::FLIP_MODE_DIFF_IMAGES = "D";

const std::string TerrainViewPattern::RULE_DIRT = "D";
const std::string TerrainViewPattern::RULE_SAND = "S";
const std::string TerrainViewPattern::RULE_TRANSITION = "T";
const std::string TerrainViewPattern::RULE_NATIVE = "N";
const std::string TerrainViewPattern::RULE_NATIVE_STRONG = "N!";
const std::string TerrainViewPattern::RULE_ANY = "?";

TerrainViewPattern::TerrainViewPattern() : diffImages(false), rotationTypesCount(0), minPoints(0)
{
	maxPoints = std::numeric_limits<int>::max();
}

TerrainViewPattern::WeightedRule::WeightedRule() : points(0)
{

}

bool TerrainViewPattern::WeightedRule::isStandardRule() const
{
	return TerrainViewPattern::RULE_ANY == name || TerrainViewPattern::RULE_DIRT == name
		|| TerrainViewPattern::RULE_NATIVE == name || TerrainViewPattern::RULE_SAND == name
		|| TerrainViewPattern::RULE_TRANSITION == name || TerrainViewPattern::RULE_NATIVE_STRONG == name;
}

CTerrainViewPatternConfig::CTerrainViewPatternConfig()
{
	const JsonNode config(ResourceID("config/terrainViewPatterns.json"));
	static const std::string patternTypes[] = { "terrainView", "terrainType" };
	for(int i = 0; i < ARRAY_COUNT(patternTypes); ++i)
	{
		const auto & patternsVec = config[patternTypes[i]].Vector();
		for(const auto & ptrnNode : patternsVec)
		{
			TerrainViewPattern pattern;

			// Read pattern data
			const JsonVector & data = ptrnNode["data"].Vector();
			assert(data.size() == 9);
			for(int j = 0; j < data.size(); ++j)
			{
				std::string cell = data[j].String();
				boost::algorithm::erase_all(cell, " ");
				std::vector<std::string> rules;
				boost::split(rules, cell, boost::is_any_of(","));
				for(std::string ruleStr : rules)
				{
					std::vector<std::string> ruleParts;
					boost::split(ruleParts, ruleStr, boost::is_any_of("-"));
					TerrainViewPattern::WeightedRule rule;
					rule.name = ruleParts[0];
					assert(!rule.name.empty());
					if(ruleParts.size() > 1)
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
			if(pattern.maxPoints == 0) pattern.maxPoints = std::numeric_limits<int>::max();

			// Read mapping
			if(i == 0)
			{
				const auto & mappingStruct = ptrnNode["mapping"].Struct();
				for(const auto & mappingPair : mappingStruct)
				{
					TerrainViewPattern terGroupPattern = pattern;
					auto mappingStr = mappingPair.second.String();
					boost::algorithm::erase_all(mappingStr, " ");
					auto colonIndex = mappingStr.find_first_of(":");
					const auto & flipMode = mappingStr.substr(0, colonIndex);
					terGroupPattern.diffImages = TerrainViewPattern::FLIP_MODE_DIFF_IMAGES == &(flipMode[flipMode.length() - 1]);
					if(terGroupPattern.diffImages)
					{
						terGroupPattern.rotationTypesCount = boost::lexical_cast<int>(flipMode.substr(0, flipMode.length() - 1));
						assert(terGroupPattern.rotationTypesCount == 2 || terGroupPattern.rotationTypesCount == 4);
					}
					mappingStr = mappingStr.substr(colonIndex + 1);
					std::vector<std::string> mappings;
					boost::split(mappings, mappingStr, boost::is_any_of(","));
					for(std::string mapping : mappings)
					{
						std::vector<std::string> range;
						boost::split(range, mapping, boost::is_any_of("-"));
						terGroupPattern.mapping.push_back(std::make_pair(boost::lexical_cast<int>(range[0]),
							boost::lexical_cast<int>(range.size() > 1 ? range[1] : range[0])));
					}

					// Add pattern to the patterns map
					const auto & terGroup = getTerrainGroup(mappingPair.first);
					terrainViewPatterns[terGroup].push_back(terGroupPattern);
				}
			}
			else if(i == 1)
			{
				terrainTypePatterns[pattern.id] = pattern;
			}
		}
	}
}

CTerrainViewPatternConfig::~CTerrainViewPatternConfig()
{

}

ETerrainGroup::ETerrainGroup CTerrainViewPatternConfig::getTerrainGroup(const std::string & terGroup) const
{
	static const std::map<std::string, ETerrainGroup::ETerrainGroup> terGroups =
	{
		{"normal", ETerrainGroup::NORMAL},
		{"dirt", ETerrainGroup::DIRT},
		{"sand", ETerrainGroup::SAND},
		{"water", ETerrainGroup::WATER},
		{"rock", ETerrainGroup::ROCK},
	};
	auto it = terGroups.find(terGroup);
	if(it == terGroups.end()) throw std::runtime_error(boost::str(boost::format("Terrain group '%s' does not exist.") % terGroup));
	return it->second;
}

const std::vector<TerrainViewPattern> & CTerrainViewPatternConfig::getTerrainViewPatternsForGroup(ETerrainGroup::ETerrainGroup terGroup) const
{
	return terrainViewPatterns.find(terGroup)->second;
}

boost::optional<const TerrainViewPattern &> CTerrainViewPatternConfig::getTerrainViewPatternById(ETerrainGroup::ETerrainGroup terGroup, const std::string & id) const
{
	const std::vector<TerrainViewPattern> & groupPatterns = getTerrainViewPatternsForGroup(terGroup);
	for(const TerrainViewPattern & pattern : groupPatterns)
	{
		if(id == pattern.id)
		{
			return boost::optional<const TerrainViewPattern &>(pattern);
		}
	}
	return boost::optional<const TerrainViewPattern &>();
}

const TerrainViewPattern & CTerrainViewPatternConfig::getTerrainTypePatternById(const std::string & id) const
{
	auto it = terrainTypePatterns.find(id);
	assert(it != terrainTypePatterns.end());
	return it->second;
}

CDrawTerrainOperation::CDrawTerrainOperation(CMap * map, const CTerrainSelection & terrainSel, ETerrainType terType, CRandomGenerator * gen)
	: CMapOperation(map), terrainSel(terrainSel), terType(terType), gen(gen)
{

}

void CDrawTerrainOperation::execute()
{
	for(const auto & pos : terrainSel.getSelectedItems())
	{
		auto & tile = map->getTile(pos);
		tile.terType = terType;
		invalidateTerrainViews(pos);
	}

	updateTerrainTypes();
	updateTerrainViews();
	//TODO add coastal bit to extTileFlags appropriately
}

void CDrawTerrainOperation::undo()
{
	//TODO
}

void CDrawTerrainOperation::redo()
{
	//TODO
}

std::string CDrawTerrainOperation::getLabel() const
{
	return "Draw Terrain";
}

void CDrawTerrainOperation::updateTerrainTypes()
{
	auto positions = terrainSel.getSelectedItems();
	while(!positions.empty())
	{
		const auto & centerPos = *(positions.begin());
		auto centerTile = map->getTile(centerPos);
		//logGlobal->debugStream() << boost::format("Set terrain tile at pos '%s' to type '%s'") % centerPos % centerTile.terType;
		auto tiles = getInvalidTiles(centerPos);
		auto updateTerrainType = [&](const int3 & pos)
		{
			map->getTile(pos).terType = centerTile.terType;
			positions.insert(pos);
			invalidateTerrainViews(pos);
			//logGlobal->debugStream() << boost::format("Set additional terrain tile at pos '%s' to type '%s'") % pos % centerTile.terType;
		};

		// Fill foreign invalid tiles
		for(const auto & tile : tiles.foreignTiles)
		{
			updateTerrainType(tile);
		}

		tiles = getInvalidTiles(centerPos);
		if(tiles.nativeTiles.find(centerPos) != tiles.nativeTiles.end())
		{
			// Blow up
			auto rect = extendTileAroundSafely(centerPos);
			std::set<int3> suitableTiles;
			int invalidForeignTilesCnt = std::numeric_limits<int>::max(), invalidNativeTilesCnt = 0;
			bool centerPosValid = false;
			rect.forEach([&](const int3 & posToTest)
			{
				auto & terrainTile = map->getTile(posToTest);
				if(centerTile.terType != terrainTile.terType)
				{
					auto formerTerType = terrainTile.terType;
					terrainTile.terType = centerTile.terType;
					auto testTile = getInvalidTiles(posToTest);

					int nativeTilesCntNorm = testTile.nativeTiles.empty() ? std::numeric_limits<int>::max() : testTile.nativeTiles.size();

					bool putSuitableTile = false;
					bool addToSuitableTiles = false;
					if(testTile.centerPosValid)
					{
						if (!centerPosValid)
						{
							centerPosValid = true;
							putSuitableTile = true;
						}
						else
						{
							if(testTile.foreignTiles.size() < invalidForeignTilesCnt)
							{
								putSuitableTile = true;
							}
							else
							{
								addToSuitableTiles = true;
							}
						}
					}
					else if (!centerPosValid)
					{
						if((nativeTilesCntNorm > invalidNativeTilesCnt) ||
								(nativeTilesCntNorm == invalidNativeTilesCnt && testTile.foreignTiles.size() < invalidForeignTilesCnt))
						{
							putSuitableTile = true;
						}
						else if(nativeTilesCntNorm == invalidNativeTilesCnt && testTile.foreignTiles.size() == invalidForeignTilesCnt)
						{
							addToSuitableTiles = true;
						}
					}

					if (putSuitableTile)
					{
						//if(!suitableTiles.empty())
						//{
						//	logGlobal->debugStream() << "Clear suitables tiles.";
						//}

						invalidNativeTilesCnt = nativeTilesCntNorm;
						invalidForeignTilesCnt = testTile.foreignTiles.size();
						suitableTiles.clear();
						addToSuitableTiles = true;
					}

					if (addToSuitableTiles)
					{
						suitableTiles.insert(posToTest);
						//logGlobal->debugStream() << boost::format(std::string("Found suitable tile '%s' for main tile '%s': ") +
						//		"Invalid native tiles '%i', invalid foreign tiles '%i', centerPosValid '%i'") % posToTest % centerPos % testTile.nativeTiles.size() %
						//		testTile.foreignTiles.size() % testTile.centerPosValid;
					}

					terrainTile.terType = formerTerType;
				}
			});

			if(suitableTiles.size() == 1)
			{
				updateTerrainType(*suitableTiles.begin());
			}
			else
			{
				static const int3 directions[] = { int3(0, -1, 0), int3(-1, 0, 0), int3(0, 1, 0), int3(1, 0, 0),
											int3(-1, -1, 0), int3(-1, 1, 0), int3(1, 1, 0), int3(1, -1, 0)};
				for(auto & direction : directions)
				{
					auto it = suitableTiles.find(centerPos + direction);
					if(it != suitableTiles.end())
					{
						updateTerrainType(*it);
						break;
					}
				}
			}
		}
		else
		{
			// add invalid native tiles which are not in the positions list
			for(const auto & nativeTile : tiles.nativeTiles)
			{
				if(positions.find(nativeTile) == positions.end())
				{
					positions.insert(nativeTile);
				}
			}

			positions.erase(centerPos);
		}
	}
}

void CDrawTerrainOperation::updateTerrainViews()
{
	for(const auto & pos : invalidatedTerViews)
	{
		const auto & patterns =
				VLC->terviewh->getTerrainViewPatternsForGroup(getTerrainGroup(map->getTile(pos).terType));

		// Detect a pattern which fits best
		int bestPattern = -1;
		ValidationResult valRslt(false);
		for(int k = 0; k < patterns.size(); ++k)
		{
			const auto & pattern = patterns[k];
			valRslt = validateTerrainView(pos, pattern);
			if(valRslt.result)
			{
				/*logGlobal->debugStream() << boost::format("Pattern detected at pos '%s': Pattern '%s', Flip '%i', Repl. '%s'.") %
											pos % pattern.id % valRslt.flip % valRslt.transitionReplacement;*/
				bestPattern = k;
				break;
			}
		}
		//assert(bestPattern != -1);
		if(bestPattern == -1)
		{
			// This shouldn't be the case
			logGlobal->warnStream() << boost::format("No pattern detected at pos '%s'.") % pos;
			CTerrainViewPatternUtils::printDebuggingInfoAboutTile(map, pos);
			continue;
		}

		// Get mapping
		const TerrainViewPattern & pattern = patterns[bestPattern];
		std::pair<int, int> mapping;
		if(valRslt.transitionReplacement.empty())
		{
			mapping = pattern.mapping[0];
		}
		else
		{
			mapping = valRslt.transitionReplacement == TerrainViewPattern::RULE_DIRT ? pattern.mapping[0] : pattern.mapping[1];
		}

		// Set terrain view
		auto & tile = map->getTile(pos);
		if(!pattern.diffImages)
		{
			tile.terView = gen->nextInt(mapping.first, mapping.second);
			tile.extTileFlags = valRslt.flip;
		}
		else
		{
			const int framesPerRot = (mapping.second - mapping.first + 1) / pattern.rotationTypesCount;
			int flip = (pattern.rotationTypesCount == 2 && valRslt.flip == 2) ? 1 : valRslt.flip;
			int firstFrame = mapping.first + flip * framesPerRot;
			tile.terView = gen->nextInt(firstFrame, firstFrame + framesPerRot - 1);
			tile.extTileFlags =	0;
		}
	}
}

ETerrainGroup::ETerrainGroup CDrawTerrainOperation::getTerrainGroup(ETerrainType terType) const
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

CDrawTerrainOperation::ValidationResult CDrawTerrainOperation::validateTerrainView(const int3 & pos, const TerrainViewPattern & pattern, int recDepth /*= 0*/) const
{
	//constructor for pattern object is very expensive, but we can't manipulate const object :(

	auto flippedPattern = pattern;
	for(int flip = 0; flip < 4; ++flip)
	{
		if (flip > 0)
			flipPattern (flippedPattern, flip);
		auto valRslt = validateTerrainViewInner(pos, flippedPattern, recDepth);
		if(valRslt.result)
		{
			valRslt.flip = flip;
			return valRslt;
		}
	}
	return ValidationResult(false);
}

CDrawTerrainOperation::ValidationResult CDrawTerrainOperation::validateTerrainViewInner(const int3 & pos, const TerrainViewPattern & pattern, int recDepth /*= 0*/) const
{
	auto centerTerType = map->getTile(pos).terType;
	auto centerTerGroup = getTerrainGroup(centerTerType);
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
		int cx = pos.x + (i % 3) - 1;
		int cy = pos.y + (i / 3) - 1;
		int3 currentPos(cx, cy, pos.z);
		bool isAlien = false;
		ETerrainType terType;
		if(!map->isInTheMap(currentPos))
		{
			// position is not in the map, so take the ter type from the neighbor tile
			bool widthTooHigh = currentPos.x >= map->width;
			bool widthTooLess = currentPos.x < 0;
			bool heightTooHigh = currentPos.y >= map->height;
			bool heightTooLess = currentPos.y < 0;

			if ((widthTooHigh && heightTooHigh) || (widthTooHigh && heightTooLess) || (widthTooLess && heightTooHigh) || (widthTooLess && heightTooLess))
			{
				terType = centerTerType;
			}
			else if(widthTooHigh)
			{
				terType = map->getTile(int3(currentPos.x - 1, currentPos.y, currentPos.z)).terType;
			}
			else if(heightTooHigh)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y - 1, currentPos.z)).terType;
			}
			else if (widthTooLess)
			{
				terType = map->getTile(int3(currentPos.x + 1, currentPos.y, currentPos.z)).terType;
			}
			else if (heightTooLess)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y + 1, currentPos.z)).terType;
			}
		}
		else
		{
			terType = map->getTile(currentPos).terType;
			if(terType != centerTerType)
			{
				isAlien = true;
			}
		}

		// Validate all rules per cell
		int topPoints = -1;
		for(auto & elem : pattern.data[i])
		{
			TerrainViewPattern::WeightedRule rule = elem;
			if(!rule.isStandardRule())
			{
				if(recDepth == 0 && map->isInTheMap(currentPos))
				{
					if(terType == centerTerType)
					{
						const auto & patternForRule = VLC->terviewh->getTerrainViewPatternById(getTerrainGroup(centerTerType), rule.name);
						if(patternForRule)
						{
							auto rslt = validateTerrainView(currentPos, *patternForRule, 1);
							if(rslt.result) topPoints = std::max(topPoints, rule.points);
						}
					}
					continue;
				}
				else
				{
					rule.name = TerrainViewPattern::RULE_NATIVE;
				}
			}

			auto applyValidationRslt = [&](bool rslt)
			{
				if(rslt)
				{
					topPoints = std::max(topPoints, rule.points);
				}
			};

			// Validate cell with the ruleset of the pattern
			bool nativeTestOk, nativeTestStrongOk;
			nativeTestOk = nativeTestStrongOk = (rule.name == TerrainViewPattern::RULE_NATIVE_STRONG || rule.name == TerrainViewPattern::RULE_NATIVE)  && !isAlien;
			if(centerTerGroup == ETerrainGroup::NORMAL)
			{
				bool dirtTestOk = (rule.name == TerrainViewPattern::RULE_DIRT || rule.name == TerrainViewPattern::RULE_TRANSITION)
						&& isAlien && !isSandType(terType);
				bool sandTestOk = (rule.name == TerrainViewPattern::RULE_SAND || rule.name == TerrainViewPattern::RULE_TRANSITION)
						&& isSandType(terType);

				if(transitionReplacement.empty() && rule.name == TerrainViewPattern::RULE_TRANSITION
						&& (dirtTestOk || sandTestOk))
				{
					transitionReplacement = dirtTestOk ? TerrainViewPattern::RULE_DIRT : TerrainViewPattern::RULE_SAND;
				}
				if(rule.name == TerrainViewPattern::RULE_TRANSITION)
				{
					applyValidationRslt((dirtTestOk && transitionReplacement != TerrainViewPattern::RULE_SAND) ||
							(sandTestOk && transitionReplacement != TerrainViewPattern::RULE_DIRT));
				}
				else
				{
					applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || dirtTestOk || sandTestOk || nativeTestOk);
				}
			}
			else if(centerTerGroup == ETerrainGroup::DIRT)
			{
				nativeTestOk = rule.name == TerrainViewPattern::RULE_NATIVE && !isSandType(terType);
				bool sandTestOk = (rule.name == TerrainViewPattern::RULE_SAND || rule.name == TerrainViewPattern::RULE_TRANSITION)
						&& isSandType(terType);
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || nativeTestOk || nativeTestStrongOk);
			}
			else if(centerTerGroup == ETerrainGroup::SAND)
			{
				applyValidationRslt(true);
			}
			else if(centerTerGroup == ETerrainGroup::WATER || centerTerGroup == ETerrainGroup::ROCK)
			{
				bool sandTestOk = (rule.name == TerrainViewPattern::RULE_SAND || rule.name == TerrainViewPattern::RULE_TRANSITION)
						&& isAlien;
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

	if(totalPoints >= pattern.minPoints && totalPoints <= pattern.maxPoints)
	{
		return ValidationResult(true, transitionReplacement);
	}
	else
	{
		return ValidationResult(false);
	}
}

bool CDrawTerrainOperation::isSandType(ETerrainType terType) const
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

void CDrawTerrainOperation::flipPattern(TerrainViewPattern & pattern, int flip) const
{
	//flip in place to avoid expensive constructor. Seriously.

	if(flip == 0)
	{
		return;
	}

	//always flip horizontal
	for(int i = 0; i < 3; ++i)
	{
		int y = i * 3;
		std::swap(pattern.data[y], pattern.data[y + 2]);
	}
	//flip vertical only at 2nd step
	if(flip == FLIP_PATTERN_VERTICAL)
	{
		for(int i = 0; i < 3; ++i)
		{
			std::swap(pattern.data[i], pattern.data[6 + i]);
		}
	}
}

void CDrawTerrainOperation::invalidateTerrainViews(const int3 & centerPos)
{
	auto rect = extendTileAroundSafely(centerPos);
	rect.forEach([&](const int3 & pos)
	{
		invalidatedTerViews.insert(pos);
	});
}

CDrawTerrainOperation::InvalidTiles CDrawTerrainOperation::getInvalidTiles(const int3 & centerPos) const
{
	InvalidTiles tiles;
	auto centerTerType = map->getTile(centerPos).terType;
	auto rect = extendTileAround(centerPos);
	rect.forEach([&](const int3 & pos)
	{
		if(map->isInTheMap(pos))
		{
			auto ptrConfig = VLC->terviewh;
			auto terType = map->getTile(pos).terType;
			auto valid = validateTerrainView(pos, ptrConfig->getTerrainTypePatternById("n1")).result;

			// Special validity check for rock & water
			if(valid && (terType == ETerrainType::WATER || terType == ETerrainType::ROCK))
			{
				static const std::string patternIds[] = { "s1", "s2" };
				for(auto & patternId : patternIds)
				{
					valid = !validateTerrainView(pos, ptrConfig->getTerrainTypePatternById(patternId)).result;
					if(!valid) break;
				}
			}
			// Additional validity check for non rock OR water
			else if(!valid && (terType != ETerrainType::WATER && terType != ETerrainType::ROCK))
			{
				static const std::string patternIds[] = { "n2", "n3" };
				for(auto & patternId : patternIds)
				{
					valid = validateTerrainView(pos, ptrConfig->getTerrainTypePatternById(patternId)).result;
					if(valid) break;
				}
			}

			if(!valid)
			{
				if(terType == centerTerType) tiles.nativeTiles.insert(pos);
				else tiles.foreignTiles.insert(pos);
			}
			else if(centerPos == pos)
			{
				tiles.centerPosValid = true;
			}
		}
	});
	return tiles;
}

CDrawTerrainOperation::ValidationResult::ValidationResult(bool result, const std::string & transitionReplacement /*= ""*/)
	: result(result), transitionReplacement(transitionReplacement)
{

}

void CTerrainViewPatternUtils::printDebuggingInfoAboutTile(const CMap * map, int3 pos)
{
	logGlobal->debugStream() << "Printing detailed info about nearby map tiles of pos '" << pos << "'";
	for(int y = pos.y - 2; y <= pos.y + 2; ++y)
	{
		std::string line;
		const int PADDED_LENGTH = 10;
		for(int x = pos.x - 2; x <= pos.x + 2; ++x)
		{
			auto debugPos = int3(x, y, pos.z);
			if(map->isInTheMap(debugPos))
			{
				auto debugTile = map->getTile(debugPos);

				std::string terType = debugTile.terType.toString().substr(0, 6);
				line += terType;
				line.insert(line.end(), PADDED_LENGTH - terType.size(), ' ');
			}
			else
			{
				line += "X";
				line.insert(line.end(), PADDED_LENGTH - 1, ' ');
			}
		}

		logGlobal->debugStream() << line;
	}
}

CClearTerrainOperation::CClearTerrainOperation(CMap * map, CRandomGenerator * gen) : CComposedOperation(map)
{
	CTerrainSelection terrainSel(map);
	terrainSel.selectRange(MapRect(int3(0, 0, 0), map->width, map->height));
	addOperation(make_unique<CDrawTerrainOperation>(map, terrainSel, ETerrainType::WATER, gen));
	if(map->twoLevel)
	{
		terrainSel.clearSelection();
		terrainSel.selectRange(MapRect(int3(0, 0, 1), map->width, map->height));
		addOperation(make_unique<CDrawTerrainOperation>(map, terrainSel, ETerrainType::ROCK, gen));
	}
}

std::string CClearTerrainOperation::getLabel() const
{
	return "Clear Terrain";
}

CInsertObjectOperation::CInsertObjectOperation(CMap * map, CGObjectInstance * obj, const int3 & pos)
	: CMapOperation(map), pos(pos), obj(obj)
{

}

void CInsertObjectOperation::execute()
{
	obj->pos = pos;
	obj->id = ObjectInstanceID(map->objects.size());
	map->objects.push_back(obj);
	if(obj->ID == Obj::TOWN)
	{
		map->towns.push_back(static_cast<CGTownInstance *>(obj));
	}
	if(obj->ID == Obj::HERO)
	{
		map->heroesOnMap.push_back(static_cast<CGHeroInstance*>(obj));
	}
	map->addBlockVisTiles(obj);
}

void CInsertObjectOperation::undo()
{
	//TODO
}

void CInsertObjectOperation::redo()
{
	execute();
}

std::string CInsertObjectOperation::getLabel() const
{
	return "Insert Object";
}
