#include "StdInc.h"
#include "CMapEditManager.h"

#include "../JsonNode.h"
#include "../filesystem/CResourceLoader.h"
#include "../CDefObjInfoHandler.h"

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
	execute(make_unique<DrawTerrainOperation>(map, rect, terType, gen));
}

void CMapEditManager::insertObject(const int3 & pos, CGObjectInstance * obj)
{
	execute(make_unique<InsertObjectOperation>(map, pos, obj));
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

CTerrainViewPatternConfig::~CTerrainViewPatternConfig()
{

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

DrawTerrainOperation::DrawTerrainOperation(CMap * map, const MapRect & rect, ETerrainType terType, CRandomGenerator * gen)
	: CMapOperation(map), rect(rect), terType(terType), gen(gen)
{

}

void DrawTerrainOperation::execute()
{
	for(int i = rect.pos.x; i < rect.pos.x + rect.width; ++i)
	{
		for(int j = rect.pos.y; j < rect.pos.y + rect.height; ++j)
		{
			map->getTile(int3(i, j, rect.pos.z)).terType = terType;
		}
	}

	//TODO there are situations where more tiles are affected implicitely
	//TODO add coastal bit to extTileFlags appropriately

	updateTerrainViews(MapRect(int3(rect.pos.x - 1, rect.pos.y - 1, rect.pos.z), rect.width + 2, rect.height + 2));
}

void DrawTerrainOperation::undo()
{
	//TODO
}

void DrawTerrainOperation::redo()
{
	//TODO
}

std::string DrawTerrainOperation::getLabel() const
{
	return "Draw Terrain";
}

void DrawTerrainOperation::updateTerrainViews(const MapRect & rect)
{
	for(int i = rect.pos.x; i < rect.pos.x + rect.width; ++i)
	{
		for(int j = rect.pos.y; j < rect.pos.y + rect.height; ++j)
		{
			const auto & patterns =
					CTerrainViewPatternConfig::get().getPatternsForGroup(getTerrainGroup(map->getTile(int3(i, j, rect.pos.z)).terType));

			// Detect a pattern which fits best
			int bestPattern = -1, bestFlip = -1;
			std::string transitionReplacement;
			for(int k = 0; k < patterns.size(); ++k)
			{
				const auto & pattern = patterns[k];

				for(int flip = 0; flip < 4; ++flip)
				{
					auto valRslt = validateTerrainView(int3(i, j, rect.pos.z), flip > 0 ? getFlippedPattern(pattern, flip) : pattern);
					if(valRslt.result)
					{
						logGlobal->debugStream() << "Pattern detected at pos " << i << "x" << j << "x" << rect.pos.z << ": P-Nr. " << k
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
				logGlobal->warnStream() << "No pattern detected at pos " << i << "x" << j << "x" << rect.pos.z;
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
			auto & tile = map->getTile(int3(i, j, rect.pos.z));
			if(pattern.flipMode == TerrainViewPattern::FLIP_MODE_SAME_IMAGE)
			{
				tile.terView = gen->getInteger(mapping.first, mapping.second);
				tile.extTileFlags = bestFlip;
			}
			else
			{
				int range = (mapping.second - mapping.first) / 4;
				tile.terView = gen->getInteger(mapping.first + bestFlip * range,
					mapping.first + (bestFlip + 1) * range - 1);
				tile.extTileFlags =	0;
			}
		}
	}
}

ETerrainGroup::ETerrainGroup DrawTerrainOperation::getTerrainGroup(ETerrainType terType) const
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

DrawTerrainOperation::ValidationResult DrawTerrainOperation::validateTerrainView(const int3 & pos, const TerrainViewPattern & pattern, int recDepth /*= 0*/) const
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

bool DrawTerrainOperation::isSandType(ETerrainType terType) const
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

TerrainViewPattern DrawTerrainOperation::getFlippedPattern(const TerrainViewPattern & pattern, int flip) const
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

DrawTerrainOperation::ValidationResult::ValidationResult(bool result, const std::string & transitionReplacement /*= ""*/)
	: result(result), transitionReplacement(transitionReplacement)
{

}

InsertObjectOperation::InsertObjectOperation(CMap * map, const int3 & pos, CGObjectInstance * obj)
	: CMapOperation(map), pos(pos), obj(obj)
{

}

void InsertObjectOperation::execute()
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

void InsertObjectOperation::undo()
{
	//TODO
}

void InsertObjectOperation::redo()
{
	execute();
}

std::string InsertObjectOperation::getLabel() const
{
	return "Insert Object";
}
