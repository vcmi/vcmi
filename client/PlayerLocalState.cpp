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

#include "../CCallback.h"
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

void PlayerLocalState::saveHeroPaths(std::map<const CGHeroInstance *, int3> & pathsMap)
{
	for(auto & p : paths)
	{
		if(p.second.nodes.size())
			pathsMap[p.first] = p.second.endPos();
		else
			logGlobal->debug("%s has assigned an empty path! Ignoring it...", p.first->getNameTranslated());
	}
}

void PlayerLocalState::loadHeroPaths(std::map<const CGHeroInstance *, int3> & pathsMap)
{
	if(owner.cb)
	{
		for(auto & p : pathsMap)
		{
			CGPath path;
			owner.cb->getPathsInfo(p.first)->getPath(path, p.second);
			paths[p.first] = path;
			logGlobal->trace("Restored path for hero %s leading to %s with %d nodes", p.first->nodeName(), p.second.toString(), path.nodes.size());
		}
	}
}

void PlayerLocalState::setPath(const CGHeroInstance * h, const CGPath & path)
{
	paths[h] = path;
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
	if(!owner.cb->getPathsInfo(h)->getPath(path, destination))
		return false;

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

	if (selection)
		adventureInt->onSelectionChanged(selection);
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
}

void PlayerLocalState::setHeroAwaken(const CGHeroInstance * hero)
{
	assert(hero);
	assert(vstd::contains(wanderingHeroes, hero));
	assert(vstd::contains(sleepingHeroes, hero));

	vstd::erase(sleepingHeroes, hero);
}

const std::vector<const CGHeroInstance *> & PlayerLocalState::getWanderingHeroes()
{
	return wanderingHeroes;
}

const CGHeroInstance * PlayerLocalState::getWanderingHero(size_t index)
{
	if(index < wanderingHeroes.size())
		return wanderingHeroes[index];
	return nullptr;
}

void PlayerLocalState::addWanderingHero(const CGHeroInstance * hero)
{
	assert(hero);
	assert(!vstd::contains(wanderingHeroes, hero));
	wanderingHeroes.push_back(hero);
}

void PlayerLocalState::removeWanderingHero(const CGHeroInstance * hero)
{
	assert(hero);
	assert(vstd::contains(wanderingHeroes, hero));

	if (hero == currentSelection)
	{
		auto const * nextHero = getNextWanderingHero(hero);
		setSelection(nextHero);
	}

	vstd::erase(wanderingHeroes, hero);
	vstd::erase(sleepingHeroes, hero);

	if (currentSelection == nullptr && !wanderingHeroes.empty())
		setSelection(wanderingHeroes.front());

	if (currentSelection == nullptr && !ownedTowns.empty())
		setSelection(ownedTowns.front());
}

const std::vector<const CGTownInstance *> & PlayerLocalState::getOwnedTowns()
{
	return ownedTowns;
}

const CGTownInstance * PlayerLocalState::getOwnedTown(size_t index)
{
	if(index < ownedTowns.size())
		return ownedTowns[index];
	return nullptr;
}

void PlayerLocalState::addOwnedTown(const CGTownInstance * town)
{
	assert(town);
	assert(!vstd::contains(ownedTowns, town));
	ownedTowns.push_back(town);
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
}
