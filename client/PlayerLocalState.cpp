/*
 * PlayerLocalState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PlayerLocalState.h"

#include "../lib/callback/CCallback.h"
#include "../lib/json/JsonNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "CPlayerInterface.h"
#include "adventureMap/AdventureMapInterface.h"

PlayerLocalState::PlayerLocalState(CPlayerInterface & owner)
	: owner(owner)
	, currentSelection(nullptr)
{
}

const PlayerSpellbookSetting & PlayerLocalState::getSpellbookSettings() const
{
	return spellbookSettings;
}

void PlayerLocalState::setSpellbookSettings(const PlayerSpellbookSetting & newSettings)
{
	spellbookSettings = newSettings;
}

void PlayerLocalState::setPath(const CGHeroInstance * h, const CGPath & path)
{
	paths[h] = path;
	syncronizeState();
}

const CGPath & PlayerLocalState::getPath(const CGHeroInstance * h) const
{
	assert(hasPath(h));
	return paths.at(h);
}

bool PlayerLocalState::hasPath(const CGHeroInstance * h) const
{
	return paths.count(h) > 0;
}

bool PlayerLocalState::setPath(const CGHeroInstance * h, const int3 & destination)
{
	CGPath path;
	if(!owner.getPathsInfo(h)->getPath(path, destination))
	{
		paths.erase(h); //invalidate previously possible path if selected (before other hero blocked only path / fly spell expired)
		syncronizeState();
		return false;
	}

	setPath(h, path);
	return true;
}

void PlayerLocalState::removeLastNode(const CGHeroInstance * h)
{
	assert(hasPath(h));
	if(!hasPath(h))
		return;

	auto & path = paths[h];
	path.nodes.pop_back();
	if(path.nodes.size() < 2) //if it was the last one, remove entire path and path with only one tile is not a real path
		erasePath(h);
}

void PlayerLocalState::erasePath(const CGHeroInstance * h)
{
	paths.erase(h);
	adventureInt->onHeroChanged(h);
	syncronizeState();
}

void PlayerLocalState::verifyPath(const CGHeroInstance * h)
{
	if(!hasPath(h))
		return;
	setPath(h, getPath(h).endPos());
}

const CGHeroInstance * PlayerLocalState::getCurrentHero() const
{
	if(currentSelection && currentSelection->ID == Obj::HERO)
		return dynamic_cast<const CGHeroInstance *>(currentSelection);
	else
		return nullptr;
}

const CGHeroInstance * PlayerLocalState::getNextWanderingHero(const CGHeroInstance * currentHero)
{
	bool currentHeroFound = false;
	const CGHeroInstance * firstSuitable = nullptr;
	const CGHeroInstance * nextSuitable = nullptr;

	for(const auto * hero : getWanderingHeroes())
	{
		if (hero == currentHero)
		{
			currentHeroFound = true;
			continue;
		}

		if (isHeroSleeping(hero))
			continue;

		if (hero->movementPointsRemaining() == 0)
			continue;

		if (!firstSuitable)
			firstSuitable = hero;

		if (!nextSuitable && currentHeroFound)
			nextSuitable = hero;
	}

	// if we found suitable hero after currently selected hero -> return this hero
	if (nextSuitable)
		return nextSuitable;

	// othervice -> loop over and return first suitable hero in the list (or null if none)
	return firstSuitable;
}

const CGTownInstance * PlayerLocalState::getCurrentTown() const
{
	if(currentSelection && currentSelection->ID == Obj::TOWN)
		return dynamic_cast<const CGTownInstance *>(currentSelection);
	else
		return nullptr;
}

const CArmedInstance * PlayerLocalState::getCurrentArmy() const
{
	if(currentSelection)
		return dynamic_cast<const CArmedInstance *>(currentSelection);
	else
		return nullptr;
}

void PlayerLocalState::setSelection(const CArmedInstance * selection)
{
	if (currentSelection == selection)
		return;

	currentSelection = selection;

	if (adventureInt && selection)
		adventureInt->onSelectionChanged(selection);
	syncronizeState();
}

bool PlayerLocalState::isHeroSleeping(const CGHeroInstance * hero) const
{
	return vstd::contains(sleepingHeroes, hero);
}

void PlayerLocalState::setHeroAsleep(const CGHeroInstance * hero)
{
	assert(hero);
	assert(vstd::contains(wanderingHeroes, hero));
	assert(!vstd::contains(sleepingHeroes, hero));

	sleepingHeroes.push_back(hero);
	syncronizeState();
}

void PlayerLocalState::setHeroAwaken(const CGHeroInstance * hero)
{
	assert(hero);
	assert(vstd::contains(wanderingHeroes, hero));
	assert(vstd::contains(sleepingHeroes, hero));

	vstd::erase(sleepingHeroes, hero);
	syncronizeState();
}

const std::vector<const CGHeroInstance *> & PlayerLocalState::getWanderingHeroes()
{
	return wanderingHeroes;
}

const CGHeroInstance * PlayerLocalState::getWanderingHero(size_t index)
{
	if(index < wanderingHeroes.size())
		return wanderingHeroes[index];
	throw std::runtime_error("No hero with index " + std::to_string(index));
}

void PlayerLocalState::addWanderingHero(const CGHeroInstance * hero)
{
	assert(hero);
	assert(!vstd::contains(wanderingHeroes, hero));
	wanderingHeroes.push_back(hero);

	if (currentSelection == nullptr)
		setSelection(hero);

	syncronizeState();
}

void PlayerLocalState::removeWanderingHero(const CGHeroInstance * hero)
{
	assert(hero);
	assert(vstd::contains(wanderingHeroes, hero));

	if (hero == currentSelection)
	{
		auto const * nextHero = getNextWanderingHero(hero);
		if (nextHero)
			setSelection(nextHero);
		else if (!ownedTowns.empty())
			setSelection(ownedTowns.front());
		else
			setSelection(nullptr);
	}

	vstd::erase(wanderingHeroes, hero);
	vstd::erase(sleepingHeroes, hero);

	if (currentSelection == nullptr && !wanderingHeroes.empty())
		setSelection(wanderingHeroes.front());

	if (currentSelection == nullptr && !ownedTowns.empty())
		setSelection(ownedTowns.front());

	syncronizeState();
}

void PlayerLocalState::swapWanderingHero(size_t pos1, size_t pos2)
{
	assert(wanderingHeroes[pos1] && wanderingHeroes[pos2]);
	std::swap(wanderingHeroes.at(pos1), wanderingHeroes.at(pos2));

	adventureInt->onHeroOrderChanged();

	syncronizeState();
}

const std::vector<const CGTownInstance *> & PlayerLocalState::getOwnedTowns()
{
	return ownedTowns;
}

const CGTownInstance * PlayerLocalState::getOwnedTown(size_t index)
{
	if(index < ownedTowns.size())
		return ownedTowns[index];
	throw std::runtime_error("No town with index " + std::to_string(index));
}

void PlayerLocalState::addOwnedTown(const CGTownInstance * town)
{
	assert(town);
	assert(!vstd::contains(ownedTowns, town));
	ownedTowns.push_back(town);

	if (currentSelection == nullptr)
		setSelection(town);

	syncronizeState();
}

void PlayerLocalState::removeOwnedTown(const CGTownInstance * town)
{
	assert(town);
	assert(vstd::contains(ownedTowns, town));
	vstd::erase(ownedTowns, town);

	if (town == currentSelection)
		setSelection(nullptr);

	if (currentSelection == nullptr && !wanderingHeroes.empty())
		setSelection(wanderingHeroes.front());

	if (currentSelection == nullptr && !ownedTowns.empty())
		setSelection(ownedTowns.front());

	syncronizeState();
}

void PlayerLocalState::swapOwnedTowns(size_t pos1, size_t pos2)
{
	assert(ownedTowns[pos1] && ownedTowns[pos2]);
	std::swap(ownedTowns.at(pos1), ownedTowns.at(pos2));

	syncronizeState();

	adventureInt->onTownOrderChanged();
}

void PlayerLocalState::syncronizeState()
{
	JsonNode data;
	serialize(data);
	owner.cb->saveLocalState(data);
}

void PlayerLocalState::serialize(JsonNode & dest) const
{
	dest.clear();

	for (auto const * town : ownedTowns)
	{
		JsonNode record;
		record["id"].Integer() = town->id.getNum();
		dest["towns"].Vector().push_back(record);
	}

	for (auto const * hero : wanderingHeroes)
	{
		JsonNode record;
		record["id"].Integer() = hero->id.getNum();
		if (vstd::contains(sleepingHeroes, hero))
			record["sleeping"].Bool() = true;

		if (paths.count(hero))
		{
			record["path"]["x"].Integer() = paths.at(hero).lastNode().coord.x;
			record["path"]["y"].Integer() = paths.at(hero).lastNode().coord.y;
			record["path"]["z"].Integer() = paths.at(hero).lastNode().coord.z;
		}
		dest["heroes"].Vector().push_back(record);
	}
	dest["spellbook"]["pageBattle"].Integer() = spellbookSettings.spellbookLastPageBattle;
	dest["spellbook"]["pageAdvmap"].Integer() = spellbookSettings.spellbookLastPageAdvmap;
	dest["spellbook"]["tabBattle"].Integer() = spellbookSettings.spellbookLastTabBattle;
	dest["spellbook"]["tabAdvmap"].Integer() = spellbookSettings.spellbookLastTabAdvmap;

	if (currentSelection)
		dest["currentSelection"].Integer() = currentSelection->id.getNum();
}

void PlayerLocalState::deserialize(const JsonNode & source)
{
	// this method must be called after player state has been initialized
	assert(currentSelection != nullptr);
	assert(!ownedTowns.empty() || !wanderingHeroes.empty());

	auto oldHeroes = wanderingHeroes;
	auto oldTowns = ownedTowns;

	paths.clear();
	sleepingHeroes.clear();
	wanderingHeroes.clear();
	ownedTowns.clear();

	for (auto const & town : source["towns"].Vector())
	{
		ObjectInstanceID objID(town["id"].Integer());
		const CGTownInstance * townPtr = owner.cb->getTown(objID);

		if (!townPtr)
			continue;

		if (!vstd::contains(oldTowns, townPtr))
			continue;

		ownedTowns.push_back(townPtr);
		vstd::erase(oldTowns, townPtr);
	}

	for (auto const & hero : source["heroes"].Vector())
	{
		ObjectInstanceID objID(hero["id"].Integer());
		const CGHeroInstance * heroPtr = owner.cb->getHero(objID);

		if (!heroPtr)
			continue;

		if (!vstd::contains(oldHeroes, heroPtr))
			continue;

		wanderingHeroes.push_back(heroPtr);
		vstd::erase(oldHeroes, heroPtr);

		if (hero["sleeping"].Bool())
			sleepingHeroes.push_back(heroPtr);

		if (hero["path"]["x"].isNumber() && hero["path"]["y"].isNumber() && hero["path"]["z"].isNumber())
		{
			int3 pathTarget(hero["path"]["x"].Integer(), hero["path"]["y"].Integer(), hero["path"]["z"].Integer());
			setPath(heroPtr, pathTarget);
		}
	}

	if (!source["spellbook"].isNull())
	{
		spellbookSettings.spellbookLastPageBattle = source["spellbook"]["pageBattle"].Integer();
		spellbookSettings.spellbookLastPageAdvmap = source["spellbook"]["pageAdvmap"].Integer();
		spellbookSettings.spellbookLastTabBattle = source["spellbook"]["tabBattle"].Integer();
		spellbookSettings.spellbookLastTabAdvmap = source["spellbook"]["tabAdvmap"].Integer();
	}

	// append any owned heroes / towns that were not present in loaded state
	wanderingHeroes.insert(wanderingHeroes.end(), oldHeroes.begin(), oldHeroes.end());
	ownedTowns.insert(ownedTowns.end(), oldTowns.begin(), oldTowns.end());

//FIXME: broken, anything that is selected in here will be overwritten on PlayerStartsTurn pack
//	ObjectInstanceID selectedObjectID(source["currentSelection"].Integer());
//	const CGObjectInstance * objectPtr = owner.cb->getObjInstance(selectedObjectID);
//	const CArmedInstance * armyPtr = dynamic_cast<const CArmedInstance*>(objectPtr);
//
//	if (armyPtr)
//		setSelection(armyPtr);
}
