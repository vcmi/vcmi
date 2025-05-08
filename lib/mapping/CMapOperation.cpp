/*
 * CMapOperation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMapOperation.h"

#include "../GameLibrary.h"
#include "../TerrainHandler.h"
#include "../mapObjects/CGObjectInstance.h"
#include "CMap.h"
#include "MapEditUtils.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

CMapOperation::CMapOperation(CMap* map) : map(map)
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

MapRect CMapOperation::extendTileAroundSafely(const int3& centerPos) const
{
	return extendTileAround(centerPos) & MapRect(int3(0, 0, centerPos.z), map->width, map->height);
}

CComposedOperation::CComposedOperation(CMap* map) : CMapOperation(map)
{

}

void CComposedOperation::execute()
{
	// FIXME: Only reindex objects at the end of composite operation

	for(auto & operation : operations)
	{
		operation->execute();
	}
}

void CComposedOperation::undo()
{
	//reverse order
	for(auto operation = operations.rbegin(); operation != operations.rend(); operation++)
	{
		operation->get()->undo();
	}
}

void CComposedOperation::redo()
{
	for(auto & operation : operations)
	{
		operation->redo();
	}
}

std::string CComposedOperation::getLabel() const
{
	std::string ret = "Composed operation: ";
	for(const auto & operation : operations)
	{
		ret.append(operation->getLabel() + ";");
	}
	return ret;
}

void CComposedOperation::addOperation(std::unique_ptr<CMapOperation>&& operation)
{
	operations.push_back(std::move(operation));
}

CDrawTerrainOperation::CDrawTerrainOperation(CMap * map, CTerrainSelection terrainSel, TerrainId terType, int decorationsPercentage, vstd::RNG * gen):
	CMapOperation(map),
	terrainSel(std::move(terrainSel)),
	terType(terType),
	decorationsPercentage(decorationsPercentage),
	gen(gen)
{

}

void CDrawTerrainOperation::execute()
{
	for(const auto & pos : terrainSel.getSelectedItems())
	{
		auto & tile = map->getTile(pos);
		tile.terrainType = terType;
		invalidateTerrainViews(pos);
	}

	updateTerrainTypes();
	updateTerrainViews();
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
		//logGlobal->debug("Set terrain tile at pos '%s' to type '%s'", centerPos, centerTile.terType);
		auto tiles = getInvalidTiles(centerPos);
		auto updateTerrainType = [&](const int3& pos)
		{
			map->getTile(pos).terrainType = centerTile.terrainType;
			positions.insert(pos);
			invalidateTerrainViews(pos);
			//logGlobal->debug("Set additional terrain tile at pos '%s' to type '%s'", pos, centerTile.terType);
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
			int invalidForeignTilesCnt = std::numeric_limits<int>::max();
			int invalidNativeTilesCnt = 0;
			bool centerPosValid = false;
			rect.forEach([&](const int3& posToTest)
				{
					auto & terrainTile = map->getTile(posToTest);
					if(centerTile.getTerrain() != terrainTile.getTerrain())
					{
						const auto formerTerType = terrainTile.terrainType;
						terrainTile.terrainType = centerTile.terrainType;
						auto testTile = getInvalidTiles(posToTest);

						int nativeTilesCntNorm = testTile.nativeTiles.empty() ? std::numeric_limits<int>::max() : static_cast<int>(testTile.nativeTiles.size());

						bool putSuitableTile = false;
						bool addToSuitableTiles = false;
						if(testTile.centerPosValid)
						{
							if(!centerPosValid)
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
						else if(!centerPosValid)
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

						if(putSuitableTile)
						{
							//if(!suitableTiles.empty())
							//{
							//	logGlobal->debug("Clear suitables tiles.");
							//}

							invalidNativeTilesCnt = nativeTilesCntNorm;
							invalidForeignTilesCnt = static_cast<int>(testTile.foreignTiles.size());
							suitableTiles.clear();
							addToSuitableTiles = true;
						}

						if(addToSuitableTiles)
						{
							suitableTiles.insert(posToTest);
						}

						terrainTile.terrainType = formerTerType;
					}
				});

			if(suitableTiles.size() == 1)
			{
				updateTerrainType(*suitableTiles.begin());
			}
			else
			{
				static const int3 directions[] = { int3(0, -1, 0), int3(-1, 0, 0), int3(0, 1, 0), int3(1, 0, 0),
											int3(-1, -1, 0), int3(-1, 1, 0), int3(1, 1, 0), int3(1, -1, 0) };
				for(const auto & direction : directions)
				{
					auto it = suitableTiles.find(centerPos + direction);
					if (it != suitableTiles.end())
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
				if (positions.find(nativeTile) == positions.end())
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
		const auto & patterns = LIBRARY->terviewh->getTerrainViewPatterns(map->getTile(pos).getTerrainID());

		// Detect a pattern which fits best
		int bestPattern = -1;
		ValidationResult valRslt(false);
		for(int k = 0; k < patterns.size(); ++k)
		{
			const auto & pattern = patterns[k];
			//(ETerrainGroup::ETerrainGroup terGroup, const std::string & id)
			valRslt = validateTerrainView(pos, &pattern);
			if (valRslt.result)
			{
				bestPattern = k;
				break;
			}
		}
		//assert(bestPattern != -1);
		if(bestPattern == -1)
		{
			// This shouldn't be the case
			logGlobal->warn("No pattern detected at pos '%s'.", pos.toString());
			CTerrainViewPatternUtils::printDebuggingInfoAboutTile(map, pos);
			continue;
		}

		// Get mapping
		const TerrainViewPattern& pattern = patterns[bestPattern][valRslt.flip];
		std::pair<int, int> mapping;

		mapping = pattern.mapping[0];

		if(pattern.decoration)
		{
			if (pattern.mapping.size() < 2 || gen->nextInt(100) > decorationsPercentage)
				mapping = pattern.mapping[0];
			else
				mapping = pattern.mapping[1];
		}

		if (!valRslt.transitionReplacement.empty())
			mapping = valRslt.transitionReplacement == TerrainViewPattern::RULE_DIRT ? pattern.mapping[0] : pattern.mapping[1];

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
			tile.extTileFlags = 0;
		}
	}
}

CDrawTerrainOperation::ValidationResult CDrawTerrainOperation::validateTerrainView(const int3& pos, const std::vector<TerrainViewPattern>* pattern, int recDepth) const
{
	for(int flip = 0; flip < 4; ++flip)
	{
		auto valRslt = validateTerrainViewInner(pos, pattern->at(flip), recDepth);
		if(valRslt.result)
		{
			valRslt.flip = flip;
			return valRslt;
		}
	}
	return ValidationResult(false);
}

CDrawTerrainOperation::ValidationResult CDrawTerrainOperation::validateTerrainViewInner(const int3& pos, const TerrainViewPattern& pattern, int recDepth) const
{
	const auto * centerTerType = map->getTile(pos).getTerrain();
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
		const TerrainType * terType = nullptr;
		if(!map->isInTheMap(currentPos))
		{
			// position is not in the map, so take the ter type from the neighbor tile
			bool widthTooHigh = currentPos.x >= map->width;
			bool widthTooLess = currentPos.x < 0;
			bool heightTooHigh = currentPos.y >= map->height;
			bool heightTooLess = currentPos.y < 0;

			if((widthTooHigh && heightTooHigh) || (widthTooHigh && heightTooLess) || (widthTooLess && heightTooHigh) || (widthTooLess && heightTooLess))
			{
				terType = centerTerType;
			}
			else if(widthTooHigh)
			{
				terType = map->getTile(int3(currentPos.x - 1, currentPos.y, currentPos.z)).getTerrain();
			}
			else if(heightTooHigh)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y - 1, currentPos.z)).getTerrain();
			}
			else if(widthTooLess)
			{
				terType = map->getTile(int3(currentPos.x + 1, currentPos.y, currentPos.z)).getTerrain();
			}
			else if(heightTooLess)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y + 1, currentPos.z)).getTerrain();
			}
		}
		else
		{
			terType = map->getTile(currentPos).getTerrain();
			if(terType != centerTerType && (terType->isPassable() || centerTerType->isPassable()))
			{
				isAlien = true;
			}
		}

		// Validate all rules per cell
		int topPoints = -1;
		for(const auto & elem : pattern.data[i])
		{
			TerrainViewPattern::WeightedRule rule = elem;
			if(!rule.isStandardRule())
			{
				if(recDepth == 0 && map->isInTheMap(currentPos))
				{
					if(terType->getId() == centerTerType->getId())
					{
						const auto patternForRule = LIBRARY->terviewh->getTerrainViewPatternsById(centerTerType->getId(), rule.name);
						if(auto p = patternForRule)
						{
							auto rslt = validateTerrainView(currentPos, &(p->get()), 1);
							if(rslt.result) topPoints = std::max(topPoints, rule.points);
						}
					}
					continue;
				}
				else
				{
					rule.setNative();
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
			bool nativeTestOk = false;
			bool nativeTestStrongOk = false;
			nativeTestOk = nativeTestStrongOk = (rule.isNativeStrong() || rule.isNativeRule()) && !isAlien;

			if(centerTerType->getId() == ETerrainId::DIRT)
			{
				nativeTestOk = rule.isNativeRule() && !terType->isTransitionRequired();
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& terType->isTransitionRequired();
				applyValidationRslt(rule.isAnyRule() || sandTestOk || nativeTestOk || nativeTestStrongOk);
			}
			else if(centerTerType->getId() == ETerrainId::SAND)
			{
				applyValidationRslt(true);
			}
			else if(centerTerType->isTransitionRequired()) //water, rock and some special terrains require sand transition
			{
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& isAlien;
				applyValidationRslt(rule.isAnyRule() || sandTestOk || nativeTestOk);
			}
			else
			{
				bool dirtTestOk = (rule.isDirtRule() || rule.isTransition())
					&& isAlien && !terType->isTransitionRequired();
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& terType->isTransitionRequired();

				if(transitionReplacement.empty() && rule.isTransition()
					&& (dirtTestOk || sandTestOk))
				{
					transitionReplacement = dirtTestOk ? TerrainViewPattern::RULE_DIRT : TerrainViewPattern::RULE_SAND;
				}
				if(rule.isTransition())
				{
					applyValidationRslt((dirtTestOk && transitionReplacement != TerrainViewPattern::RULE_SAND) ||
						(sandTestOk && transitionReplacement != TerrainViewPattern::RULE_DIRT));
				}
				else
				{
					applyValidationRslt(rule.isAnyRule() || dirtTestOk || sandTestOk || nativeTestOk);
				}
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

void CDrawTerrainOperation::invalidateTerrainViews(const int3& centerPos)
{
	auto rect = extendTileAroundSafely(centerPos);
	rect.forEach([&](const int3& pos)
		{
			invalidatedTerViews.insert(pos);
		});
}

CDrawTerrainOperation::InvalidTiles CDrawTerrainOperation::getInvalidTiles(const int3& centerPos) const
{
	//TODO: this is very expensive function for RMG, needs optimization
	InvalidTiles tiles;
	const auto * centerTerType = map->getTile(centerPos).getTerrain();
	auto rect = extendTileAround(centerPos);
	rect.forEach([&](const int3& pos)
		{
			if(map->isInTheMap(pos))
			{
				const auto * terType = map->getTile(pos).getTerrain();
				auto valid = validateTerrainView(pos, LIBRARY->terviewh->getTerrainTypePatternById("n1")).result;

				// Special validity check for rock & water
				if(valid && (terType->isWater() || !terType->isPassable()))
				{
					static const std::string patternIds[] = { "s1", "s2" };
					for(const auto & patternId : patternIds)
					{
						valid = !validateTerrainView(pos, LIBRARY->terviewh->getTerrainTypePatternById(patternId)).result;
						if(!valid) break;
					}
				}
				// Additional validity check for non rock OR water
				else if(!valid && (terType->isLand() && terType->isPassable()))
				{
					static const std::string patternIds[] = { "n2", "n3" };
					for(const auto & patternId : patternIds)
					{
						valid = validateTerrainView(pos, LIBRARY->terviewh->getTerrainTypePatternById(patternId)).result;
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

CDrawTerrainOperation::ValidationResult::ValidationResult(bool result, std::string transitionReplacement)
	: result(result)
	, transitionReplacement(std::move(transitionReplacement))
	, flip(0)
{

}

CClearTerrainOperation::CClearTerrainOperation(CMap* map, vstd::RNG* gen) : CComposedOperation(map)
{
	CTerrainSelection terrainSel(map);
	terrainSel.selectRange(MapRect(int3(0, 0, 0), map->width, map->height));
	addOperation(std::make_unique<CDrawTerrainOperation>(map, terrainSel, ETerrainId::WATER, 0, gen));
	if(map->twoLevel)
	{
		terrainSel.clearSelection();
		terrainSel.selectRange(MapRect(int3(0, 0, 1), map->width, map->height));
		addOperation(std::make_unique<CDrawTerrainOperation>(map, terrainSel, ETerrainId::ROCK, 0, gen));
	}
}

std::string CClearTerrainOperation::getLabel() const
{
	return "Clear Terrain";
}

CInsertObjectOperation::CInsertObjectOperation(CMap* map, std::shared_ptr<CGObjectInstance> obj)
	: CMapOperation(map), obj(obj)
{

}

void CInsertObjectOperation::execute()
{
	map->generateUniqueInstanceName(obj.get());
	map->addNewObject(obj);
}

void CInsertObjectOperation::undo()
{
	map->removeObject(obj->id);
}

void CInsertObjectOperation::redo()
{
	execute();
}

std::string CInsertObjectOperation::getLabel() const
{
	return "Insert Object";
}

CMoveObjectOperation::CMoveObjectOperation(CMap* map, CGObjectInstance * obj, const int3& targetPosition)
	: CMapOperation(map),
	obj(obj),
	initialPos(obj->anchorPos()),
	targetPos(targetPosition)
{
}

void CMoveObjectOperation::execute()
{
	map->moveObject(obj->id, targetPos);
}

void CMoveObjectOperation::undo()
{
	map->moveObject(obj->id, initialPos);
}

void CMoveObjectOperation::redo()
{
	execute();
}

std::string CMoveObjectOperation::getLabel() const
{
	return "Move Object";
}

CRemoveObjectOperation::CRemoveObjectOperation(CMap* map, CGObjectInstance * obj)
	: CMapOperation(map), targetedObject(obj)
{

}

void CRemoveObjectOperation::execute()
{
	removedObject = map->removeObject(targetedObject->id);
}

void CRemoveObjectOperation::undo()
{
	assert(removedObject != nullptr);
	map->addNewObject(removedObject);
}

void CRemoveObjectOperation::redo()
{
	execute();
}

std::string CRemoveObjectOperation::getLabel() const
{
	return "Remove Object";
}

VCMI_LIB_NAMESPACE_END
