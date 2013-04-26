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

const std::string TerrainViewPattern::FLIP_MODE_DIFF_IMAGES = "D";

const std::string TerrainViewPattern::RULE_DIRT = "D";
const std::string TerrainViewPattern::RULE_SAND = "S";
const std::string TerrainViewPattern::RULE_TRANSITION = "T";
const std::string TerrainViewPattern::RULE_NATIVE = "N";
const std::string TerrainViewPattern::RULE_ANY = "?";

TerrainViewPattern::TerrainViewPattern() : diffImages(false), rotationTypesCount(0), minPoints(0), terGroup(ETerrainGroup::NORMAL)
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
	const auto & patternsVec = config.Vector();
	BOOST_FOREACH(const auto & ptrnNode, patternsVec)
	{
		TerrainViewPattern pattern;

		// Read pattern data
		const JsonVector & data = ptrnNode["data"].Vector();
		assert(data.size() == 9);
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
				assert(!rule.name.empty());
				if(ruleParts.size() > 1)
				{
					rule.points = boost::lexical_cast<int>(ruleParts[1]);
				}
				pattern.data[i].push_back(rule);
			}
		}

		// Read various properties
		pattern.id = ptrnNode["id"].String();
		assert(!pattern.id.empty());
		pattern.minPoints = static_cast<int>(ptrnNode["minPoints"].Float());
		pattern.maxPoints = static_cast<int>(ptrnNode["maxPoints"].Float());
		if(pattern.maxPoints == 0) pattern.maxPoints = std::numeric_limits<int>::max();

		// Read mapping
		const auto & mappingStruct = ptrnNode["mapping"].Struct();
		BOOST_FOREACH(const auto & mappingPair, mappingStruct)
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
			BOOST_FOREACH(std::string mapping, mappings)
			{
				std::vector<std::string> range;
				boost::split(range, mapping, boost::is_any_of("-"));
				terGroupPattern.mapping.push_back(std::make_pair(boost::lexical_cast<int>(range[0]),
					boost::lexical_cast<int>(range.size() > 1 ? range[1] : range[0])));
			}
			const auto & terGroup = getTerrainGroup(mappingPair.first);
			terGroupPattern.terGroup = terGroup;
			patterns[terGroup].push_back(terGroupPattern);
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

boost::optional<const TerrainViewPattern &> CTerrainViewPatternConfig::getPatternById(ETerrainGroup::ETerrainGroup terGroup, const std::string & id) const
{
	const std::vector<TerrainViewPattern> & groupPatterns = getPatternsForGroup(terGroup);
	BOOST_FOREACH(const TerrainViewPattern & pattern, groupPatterns)
	{
		if(id == pattern.id)
		{
			return boost::optional<const TerrainViewPattern &>(pattern);
		}
	}
	return boost::optional<const TerrainViewPattern &>();
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
	for(int x = rect.x; x < rect.x + rect.width; ++x)
	{
		for(int y = rect.y; y < rect.y + rect.height; ++y)
		{
			const auto & patterns =
					CTerrainViewPatternConfig::get().getPatternsForGroup(getTerrainGroup(map->getTile(int3(x, y, rect.z)).terType));

			// Detect a pattern which fits best
			int bestPattern = -1;
			ValidationResult valRslt(false);
			for(int k = 0; k < patterns.size(); ++k)
			{
				const auto & pattern = patterns[k];
				valRslt = validateTerrainView(int3(x, y, rect.z), pattern);
				if(valRslt.result)
				{
					logGlobal->debugStream() << "Pattern detected at pos " << x << "x" << y << "x" << rect.z << ": P-Nr. " << pattern.id
						  << ", Flip " << valRslt.flip << ", Repl. " << valRslt.transitionReplacement;
					bestPattern = k;
					break;
				}
			}
			assert(bestPattern != -1);
			if(bestPattern == -1)
			{
				// This shouldn't be the case
				logGlobal->warnStream() << "No pattern detected at pos " << x << "x" << y << "x" << rect.z;
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
			auto & tile = map->getTile(int3(x, y, rect.z));
			if(!pattern.diffImages)
			{
				tile.terView = gen->getInteger(mapping.first, mapping.second);
				tile.extTileFlags = valRslt.flip;
			}
			else
			{
				const int framesPerRot = (mapping.second - mapping.first + 1) / pattern.rotationTypesCount;
				int flip = (pattern.rotationTypesCount == 2 && valRslt.flip == 2) ? 1 : valRslt.flip;
				int firstFrame = mapping.first + flip * framesPerRot;
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
	for(int flip = 0; flip < 4; ++flip)
	{
		auto valRslt = validateTerrainViewInner(pos, flip > 0 ? getFlippedPattern(pattern, flip) : pattern, recDepth);
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
		int3 currentPos(cx, cy, pos.z);
		bool isAlien = false;
		ETerrainType terType;
		if(!map->isInTheMap(currentPos))
		{
			terType = centerTerType;
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
		for(int j = 0; j < pattern.data[i].size(); ++j)
		{
			TerrainViewPattern::WeightedRule rule = pattern.data[i][j];
			if(!rule.isStandardRule())
			{
				if(recDepth == 0 && map->isInTheMap(currentPos))
				{
					const auto & patternForRule = CTerrainViewPatternConfig::get().getPatternById(getTerrainGroup(terType), rule.name);
					if(patternForRule)
					{
						auto rslt = validateTerrainView(currentPos, *patternForRule, 1);
						if(rslt.result) topPoints = std::max(topPoints, rule.points);
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
			if(pattern.terGroup == ETerrainGroup::NORMAL)
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
					bool nativeTestOk = rule.name == TerrainViewPattern::RULE_NATIVE && !isAlien;
					applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || dirtTestOk || sandTestOk || nativeTestOk);
				}
			}
			else if(pattern.terGroup == ETerrainGroup::DIRT)
			{
				bool nativeTestOk = rule.name == TerrainViewPattern::RULE_NATIVE && !isSandType(terType);
				bool sandTestOk = (rule.name == TerrainViewPattern::RULE_SAND || rule.name == TerrainViewPattern::RULE_TRANSITION)
						&& isSandType(terType);
				applyValidationRslt(rule.name == TerrainViewPattern::RULE_ANY || sandTestOk || nativeTestOk);
			}
			else if(pattern.terGroup == ETerrainGroup::SAND)
			{
				applyValidationRslt(true);
			}
			else if(pattern.terGroup == ETerrainGroup::WATER || pattern.terGroup == ETerrainGroup::ROCK)
			{
				bool nativeTestOk = rule.name == TerrainViewPattern::RULE_NATIVE && !isAlien;
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
