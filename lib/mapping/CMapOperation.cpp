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

#include "../VCMI_Lib.h"
#include "CMap.h"
#include "MapEditUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapOperation::CMapOperation(CMap* map) : map(map)
{

}

std::string CMapOperation::getLabel() const
{
	return "";
}


MapRect CMapOperation::extendTileAround(const int3& centerPos) const
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
	for(auto & operation : operations)
	{
		ret.append(operation->getLabel() + ";");
	}
	return ret;
}

void CComposedOperation::addOperation(std::unique_ptr<CMapOperation>&& operation)
{
	operations.push_back(std::move(operation));
}

CDrawTerrainOperation::CDrawTerrainOperation(CMap* map, const CTerrainSelection& terrainSel, Terrain terType, CRandomGenerator* gen)
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
			map->getTile(pos).terType = centerTile.terType;
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
			int invalidForeignTilesCnt = std::numeric_limits<int>::max(), invalidNativeTilesCnt = 0;
			bool centerPosValid = false;
			rect.forEach([&](const int3& posToTest)
				{
					auto & terrainTile = map->getTile(posToTest);
					if(centerTile.terType != terrainTile.terType)
					{
						auto formerTerType = terrainTile.terType;
						terrainTile.terType = centerTile.terType;
						auto testTile = getInvalidTiles(posToTest);

						int nativeTilesCntNorm = testTile.nativeTiles.empty() ? std::numeric_limits<int>::max() : (int)testTile.nativeTiles.size();

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
											int3(-1, -1, 0), int3(-1, 1, 0), int3(1, 1, 0), int3(1, -1, 0) };
				for(auto & direction : directions)
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
		const auto & patterns = VLC->terviewh->getTerrainViewPatterns(map->getTile(pos).terType);

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
	auto centerTerType = map->getTile(pos).terType;
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
		Terrain terType;
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
				terType = map->getTile(int3(currentPos.x - 1, currentPos.y, currentPos.z)).terType;
			}
			else if(heightTooHigh)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y - 1, currentPos.z)).terType;
			}
			else if(widthTooLess)
			{
				terType = map->getTile(int3(currentPos.x + 1, currentPos.y, currentPos.z)).terType;
			}
			else if(heightTooLess)
			{
				terType = map->getTile(int3(currentPos.x, currentPos.y + 1, currentPos.z)).terType;
			}
		}
		else
		{
			terType = map->getTile(currentPos).terType;
			if(terType != centerTerType && (terType.isPassable() || centerTerType.isPassable()))
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
						const auto & patternForRule = VLC->terviewh->getTerrainViewPatternsById(centerTerType, rule.name);
						if(auto p = patternForRule)
						{
							auto rslt = validateTerrainView(currentPos, &(*p), 1);
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
			bool nativeTestOk, nativeTestStrongOk;
			nativeTestOk = nativeTestStrongOk = (rule.isNativeStrong() || rule.isNativeRule()) && !isAlien;

			if(centerTerType == Terrain("dirt"))
			{
				nativeTestOk = rule.isNativeRule() && !terType.isTransitionRequired();
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& terType.isTransitionRequired();
				applyValidationRslt(rule.isAnyRule() || sandTestOk || nativeTestOk || nativeTestStrongOk);
			}
			else if(centerTerType == Terrain("sand"))
			{
				applyValidationRslt(true);
			}
			else if(centerTerType.isTransitionRequired()) //water, rock and some special terrains require sand transition
			{
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& isAlien;
				applyValidationRslt(rule.isAnyRule() || sandTestOk || nativeTestOk);
			}
			else
			{
				bool dirtTestOk = (rule.isDirtRule() || rule.isTransition())
					&& isAlien && !terType.isTransitionRequired();
				bool sandTestOk = (rule.isSandRule() || rule.isTransition())
					&& terType.isTransitionRequired();

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
	auto centerTerType = map->getTile(centerPos).terType;
	auto rect = extendTileAround(centerPos);
	rect.forEach([&](const int3& pos)
		{
			if(map->isInTheMap(pos))
			{
				auto ptrConfig = VLC->terviewh;
				auto terType = map->getTile(pos).terType;
				auto valid = validateTerrainView(pos, ptrConfig->getTerrainTypePatternById("n1")).result;

				// Special validity check for rock & water
				if(valid && (terType.isWater() || !terType.isPassable()))
				{
					static const std::string patternIds[] = { "s1", "s2" };
					for(auto & patternId : patternIds)
					{
						valid = !validateTerrainView(pos, ptrConfig->getTerrainTypePatternById(patternId)).result;
						if(!valid) break;
					}
				}
				// Additional validity check for non rock OR water
				else if(!valid && (terType.isLand() && terType.isPassable()))
				{
					static const std::string patternIds[] = { "n2", "n3" };
					for (auto & patternId : patternIds)
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

CDrawTerrainOperation::ValidationResult::ValidationResult(bool result, const std::string& transitionReplacement)
	: result(result), transitionReplacement(transitionReplacement), flip(0)
{

}

CClearTerrainOperation::CClearTerrainOperation(CMap* map, CRandomGenerator* gen) : CComposedOperation(map)
{
	CTerrainSelection terrainSel(map);
	terrainSel.selectRange(MapRect(int3(0, 0, 0), map->width, map->height));
	addOperation(make_unique<CDrawTerrainOperation>(map, terrainSel, Terrain("water"), gen));
	if(map->twoLevel)
	{
		terrainSel.clearSelection();
		terrainSel.selectRange(MapRect(int3(0, 0, 1), map->width, map->height));
		addOperation(make_unique<CDrawTerrainOperation>(map, terrainSel, Terrain("rock"), gen));
	}
}

std::string CClearTerrainOperation::getLabel() const
{
	return "Clear Terrain";
}

CInsertObjectOperation::CInsertObjectOperation(CMap* map, CGObjectInstance* obj)
	: CMapOperation(map), obj(obj)
{

}

void CInsertObjectOperation::execute()
{
	obj->id = ObjectInstanceID(map->objects.size());

	do
	{
		map->setUniqueInstanceName(obj);
	} while(vstd::contains(map->instanceNames, obj->instanceName));

	map->addNewObject(obj);
}

void CInsertObjectOperation::undo()
{
	map->removeObject(obj);
}

void CInsertObjectOperation::redo()
{
	execute();
}

std::string CInsertObjectOperation::getLabel() const
{
	return "Insert Object";
}

CMoveObjectOperation::CMoveObjectOperation(CMap* map, CGObjectInstance* obj, const int3& targetPosition)
	: CMapOperation(map),
	obj(obj),
	initialPos(obj->pos),
	targetPos(targetPosition)
{
}

void CMoveObjectOperation::execute()
{
	map->moveObject(obj, targetPos);
}

void CMoveObjectOperation::undo()
{
	map->moveObject(obj, initialPos);
}

void CMoveObjectOperation::redo()
{
	execute();
}

std::string CMoveObjectOperation::getLabel() const
{
	return "Move Object";
}

CRemoveObjectOperation::CRemoveObjectOperation(CMap* map, CGObjectInstance* obj)
	: CMapOperation(map), obj(obj)
{

}

CRemoveObjectOperation::~CRemoveObjectOperation()
{
	//when operation is destroyed and wasn't undone, the object is lost forever

	if(!obj)
	{
		return;
	}

	//do not destroy an object that belongs to map
	if(!vstd::contains(map->instanceNames, obj->instanceName))
	{
		delete obj;
		obj = nullptr;
	}
}

void CRemoveObjectOperation::execute()
{
	map->removeObject(obj);
}

void CRemoveObjectOperation::undo()
{
	try
	{
		//set new id, but do not rename object
		obj->id = ObjectInstanceID((si32)map->objects.size());
		map->addNewObject(obj);
	}
	catch(const std::exception& e)
	{
		logGlobal->error(e.what());
	}
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
