#include "StdInc.h"
#include "CMapEditManager.h"

#include "../JsonNode.h"
#include "../filesystem/CResourceLoader.h"
#include "../CDefObjInfoHandler.h"

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

CMapOperation::CMapOperation(CMap * map) : map(map)
{

}

std::string CMapOperation::getLabel() const
{
	return "";
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
	if(value < 0) throw std::runtime_error("Cannot set a negative value for the undo redo limit.");
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

void CMapUndoManager::addOperation(unique_ptr<CMapOperation> && operation)
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
	: map(map)
{

}

void CMapEditManager::clearTerrain(CRandomGenerator * gen)
{
	for(int i = 0; i < map->width; ++i)
	{
		for(int j = 0; j < map->height; ++j)
		{
			map->getTile(int3(i, j, 0)).terType = ETerrainType::WATER;
			map->getTile(int3(i, j, 0)).terView = gen->getInteger(20, 32);

			if(map->twoLevel)
			{
				map->getTile(int3(i, j, 1)).terType = ETerrainType::ROCK;
				map->getTile(int3(i, j, 1)).terView = 0;
			}
		}
	}
}

void CMapEditManager::drawTerrain(const MapRect & rect, ETerrainType terType, CRandomGenerator * gen)
{
	execute(make_unique<CDrawTerrainOperation>(map, rect, terType, gen));
}

void CMapEditManager::insertObject(const int3 & pos, CGObjectInstance * obj)
{
	execute(make_unique<CInsertObjectOperation>(map, pos, obj));
}

void CMapEditManager::execute(unique_ptr<CMapOperation> && operation)
{
	operation->execute();
	undoManager.addOperation(std::move(operation));
}

void CMapEditManager::undo()
{
	undoManager.undo();
}

void CMapEditManager::redo()
{
	undoManager.redo();
}

CMapUndoManager & CMapEditManager::getUndoManager()
{
	return undoManager;
}

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

boost::mutex CTerrainViewPatternConfig::smx;

CTerrainViewPatternConfig & CTerrainViewPatternConfig::get()
{
	TLockGuard _(smx);
	static CTerrainViewPatternConfig instance;
	return instance;
}

CTerrainViewPatternConfig::CTerrainViewPatternConfig()
{
	const JsonNode config(ResourceID("config/terrainViewPatterns.json"));
	const auto & groupMap = config.Struct();
	BOOST_FOREACH(const auto & groupPair, groupMap)
	{
		auto terGroup = getTerrainGroup(groupPair.first);
		BOOST_FOREACH(const JsonNode & ptrnNode, groupPair.second.Vector())
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

			pattern.terGroup = terGroup;
			patterns[terGroup].push_back(pattern);
		}
	}
}

CTerrainViewPatternConfig::~CTerrainViewPatternConfig()
{

}

ETerrainGroup::ETerrainGroup CTerrainViewPatternConfig::getTerrainGroup(const std::string & terGroup) const
{
	static const std::map<std::string, ETerrainGroup::ETerrainGroup> terGroups
			= boost::assign::map_list_of("normal", ETerrainGroup::NORMAL)("dirt", ETerrainGroup::DIRT)
			("sand", ETerrainGroup::SAND)("water", ETerrainGroup::WATER)("rock", ETerrainGroup::ROCK);
	auto it = terGroups.find(terGroup);
	if(it == terGroups.end()) throw std::runtime_error(boost::str(boost::format("Terrain group '%s' does not exist.") % terGroup));
	return it->second;
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

CDrawTerrainOperation::CDrawTerrainOperation(CMap * map, const MapRect & rect, ETerrainType terType, CRandomGenerator * gen)
	: CMapOperation(map), rect(rect), terType(terType), gen(gen)
{

}

void CDrawTerrainOperation::execute()
{
	for(int i = rect.x; i < rect.x + rect.width; ++i)
	{
		for(int j = rect.y; j < rect.y + rect.height; ++j)
		{
			map->getTile(int3(i, j, rect.z)).terType = terType;
		}
	}

	//TODO there are situations where more tiles are affected implicitely
	//TODO add coastal bit to extTileFlags appropriately

	MapRect viewRect(int3(rect.x - 1, rect.y - 1, rect.z), rect.width + 2, rect.height + 2); // Has to overlap 1 tile around
	updateTerrainViews(viewRect & MapRect(int3(0, 0, viewRect.z), map->width, map->height)); // Rect should not overlap map dimensions
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

void CDrawTerrainOperation::updateTerrainViews(const MapRect & rect)
{
	for(int i = rect.x; i < rect.x + rect.width; ++i)
	{
		for(int j = rect.y; j < rect.y + rect.height; ++j)
		{
			const auto & patterns =
					CTerrainViewPatternConfig::get().getPatternsForGroup(getTerrainGroup(map->getTile(int3(i, j, rect.z)).terType));

			// Detect a pattern which fits best
			int bestPattern = -1, bestFlip = -1;
			std::string transitionReplacement;
			for(int k = 0; k < patterns.size(); ++k)
			{
				const auto & pattern = patterns[k];

				for(int flip = 0; flip < 4; ++flip)
				{
					auto valRslt = validateTerrainView(int3(i, j, rect.z), flip > 0 ? getFlippedPattern(pattern, flip) : pattern);
					if(valRslt.result)
					{
						logGlobal->debugStream() << "Pattern detected at pos " << i << "x" << j << "x" << rect.z << ": P-Nr. " << k
							  << ", Flip " << flip << ", Repl. " << valRslt.transitionReplacement;

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
				logGlobal->warnStream() << "No pattern detected at pos " << i << "x" << j << "x" << rect.z;
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
			auto & tile = map->getTile(int3(i, j, rect.z));
			if(pattern.flipMode == TerrainViewPattern::FLIP_MODE_SAME_IMAGE)
			{
				tile.terView = gen->getInteger(mapping.first, mapping.second);
				tile.extTileFlags = bestFlip;
			}
			else
			{
				const int framesPerRot = 2;
				int firstFrame = mapping.first + bestFlip * framesPerRot;
				tile.terView = gen->getInteger(firstFrame, firstFrame + framesPerRot - 1);
				tile.extTileFlags =	0;
			}
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
	ETerrainType centerTerType = map->getTile(pos).terType;
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
		bool isAlien = false;
		ETerrainType terType;
		if(cx < 0 || cx >= map->width || cy < 0 || cy >= map->height)
		{
			terType = centerTerType;
		}
		else
		{
			terType = map->getTile(int3(cx, cy, pos.z)).terType;
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
					const auto & patternForRule = CTerrainViewPatternConfig::get().getPatternById(pattern.terGroup, rule.name);
					auto rslt = validateTerrainView(int3(cx, cy, pos.z), patternForRule, 1);
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

TerrainViewPattern CDrawTerrainOperation::getFlippedPattern(const TerrainViewPattern & pattern, int flip) const
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

CDrawTerrainOperation::ValidationResult::ValidationResult(bool result, const std::string & transitionReplacement /*= ""*/)
	: result(result), transitionReplacement(transitionReplacement)
{

}

CInsertObjectOperation::CInsertObjectOperation(CMap * map, const int3 & pos, CGObjectInstance * obj)
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
		map->heroes.push_back(static_cast<CGHeroInstance*>(obj));
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
