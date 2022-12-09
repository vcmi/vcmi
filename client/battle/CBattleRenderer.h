/*
 * CBattleFieldController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

//class CStack;

VCMI_LIB_NAMESPACE_END

//struct SDL_Surface;
//struct Rect;
//struct Point;
//
//class CClickableHex;
class CCanvas;
//class IImage;
class BattleInterface;

enum class EBattleFieldLayer {
					   // confirmed ordering requirements:
	OBSTACLES     = 0,
	CORPSES       = 0,
	WALLS         = 1,
	HEROES        = 1,
	STACKS        = 1, // after corpses, obstacles
	BATTLEMENTS   = 2, // after stacks
	STACK_AMOUNTS = 2, // after stacks, obstacles, corpses
	EFFECTS       = 3, // after obstacles, battlements
};

class BattleRenderer
{
public:
	using RendererPtr = std::shared_ptr<CCanvas>;
	using RenderFunctor = std::function<void(RendererPtr)>;

private:
	BattleInterface * owner;

	struct RenderableInstance
	{
		RenderFunctor functor;
		EBattleFieldLayer layer;
		BattleHex tile;
	};
	std::vector<RenderableInstance> objects;

	void collectObjects();
	void sortObjects();
	void renderObjects(RendererPtr targetCanvas);
public:
	BattleRenderer(BattleInterface * owner);

	void insert(EBattleFieldLayer layer, BattleHex tile, RenderFunctor functor);
	void execute(RendererPtr targetCanvas);
};
