/*
 * BattleFieldController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"

class Canvas;
class BattleInterface;

enum class EBattleFieldLayer {
					   // confirmed ordering requirements:
	OBSTACLES_BG  = 0,
	CORPSES       = 0,
	WALLS         = 1,
	HEROES        = 2,
	STACKS        = 2, // after corpses, obstacles, walls
	OBSTACLES_FG  = 3, // after stacks
	STACK_AMOUNTS = 3, // after stacks, obstacles, corpses
	EFFECTS       = 4, // after obstacles, battlements
};

class BattleRenderer
{
public:
	using RendererRef = Canvas &;
	using RenderFunctor = std::function<void(RendererRef)>;

private:
	BattleInterface & owner;

	struct RenderableInstance
	{
		RenderFunctor functor;
		EBattleFieldLayer layer;
		BattleHex tile;
	};
	std::vector<RenderableInstance> objects;

	void collectObjects();
	void sortObjects();
	void renderObjects(RendererRef targetCanvas);
public:
	BattleRenderer(BattleInterface & owner);

	void insert(EBattleFieldLayer layer, BattleHex tile, RenderFunctor functor);
	void execute(RendererRef targetCanvas);
};
