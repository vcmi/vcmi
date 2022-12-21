/*
 * BattleFieldController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleRenderer.h"

#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleEffectsController.h"
#include "BattleControlPanel.h"
#include "BattleSiegeController.h"
#include "BattleStacksController.h"
#include "BattleObstacleController.h"

void BattleRenderer::collectObjects()
{
	owner.effectsController->collectRenderableObjects(*this);
	owner.obstacleController->collectRenderableObjects(*this);
	owner.stacksController->collectRenderableObjects(*this);
	if (owner.siegeController)
		owner.siegeController->collectRenderableObjects(*this);
	if (owner.defendingHero)
		owner.defendingHero->collectRenderableObjects(*this);
	if (owner.attackingHero)
		owner.attackingHero->collectRenderableObjects(*this);
}

void BattleRenderer::sortObjects()
{
	auto getRow = [](const RenderableInstance & object) -> int
	{
		if (object.tile.isValid())
			return object.tile.getY();

		if ( object.tile == BattleHex::HEX_BEFORE_ALL )
			return -1;

		assert( object.tile == BattleHex::HEX_AFTER_ALL || object.tile == BattleHex::INVALID);
		return GameConstants::BFIELD_HEIGHT;
	};

	std::stable_sort(objects.begin(), objects.end(), [&](const RenderableInstance & left, const RenderableInstance & right){
		if ( getRow(left) != getRow(right))
			return getRow(left) < getRow(right);
		return left.layer < right.layer;
	});
}

void BattleRenderer::renderObjects(BattleRenderer::RendererRef targetCanvas)
{
	for (auto const & object : objects)
		object.functor(targetCanvas);
}

BattleRenderer::BattleRenderer(BattleInterface & owner):
	owner(owner)
{
}

void BattleRenderer::insert(EBattleFieldLayer layer, BattleHex tile, BattleRenderer::RenderFunctor functor)
{
	objects.push_back({functor, layer, tile});
}

void BattleRenderer::execute(BattleRenderer::RendererRef targetCanvas)
{
	collectObjects();
	sortObjects();
	renderObjects(targetCanvas);
}
