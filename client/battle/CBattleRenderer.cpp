/*
 * CBattleFieldController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleRenderer.h"

#include "CBattleInterface.h"

#include "CBattleEffectsController.h"
#include "CBattleSiegeController.h"
#include "CBattleStacksController.h"
#include "CBattleObstacleController.h"

void CBattleRenderer::collectObjects()
{
	owner->collectRenderableObjects(*this);
	owner->effectsController->collectRenderableObjects(*this);
	owner->obstacleController->collectRenderableObjects(*this);
	owner->siegeController->collectRenderableObjects(*this);
	owner->stacksController->collectRenderableObjects(*this);
}

void CBattleRenderer::sortObjects()
{
	auto getRow = [](const RenderableInstance & object) -> int
	{
		if (object.tile.isValid())
			return object.tile.getY();

		if ( object.tile == BattleHex::HEX_BEFORE_ALL )
			return -1;

		if ( object.tile == BattleHex::HEX_AFTER_ALL )
			return GameConstants::BFIELD_HEIGHT;

		if ( object.tile == BattleHex::INVALID )
			return GameConstants::BFIELD_HEIGHT;

		assert(0);
		return GameConstants::BFIELD_HEIGHT;
	};

	std::stable_sort(objects.begin(), objects.end(), [&](const RenderableInstance & left, const RenderableInstance & right){
		if ( getRow(left) != getRow(right))
			return getRow(left) < getRow(right);
		return left.layer < right.layer;
	});
}

void CBattleRenderer::renderObjects(CBattleRenderer::RendererPtr targetCanvas)
{
	for (auto const & object : objects)
		object.functor(targetCanvas);
}

CBattleRenderer::CBattleRenderer(CBattleInterface * owner):
	owner(owner)
{
}

void CBattleRenderer::insert(EBattleFieldLayer layer, BattleHex tile, CBattleRenderer::RenderFunctor functor)
{
	objects.push_back({ functor, layer, tile });
}

void CBattleRenderer::execute(CBattleRenderer::RendererPtr targetCanvas)
{
	collectObjects();
	sortObjects();
	renderObjects(targetCanvas);
}
