/*
 * CBattleEffectsController.h, part of VCMI engine
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

class BattleAction;
struct CustomEffectInfo;
struct BattleTriggerEffect;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class CAnimation;
class Canvas;
class BattleInterface;
class BattleRenderer;
class CPointEffectAnimation;

namespace EBattleEffect
{
	enum EBattleEffect
	{
		// list of battle effects that have hardcoded triggers
		FEAR         = 15,
		GOOD_LUCK    = 18,
		GOOD_MORALE  = 20,
		BAD_MORALE   = 30,
		BAD_LUCK     = 48,
		RESURRECT    = 50,
		DRAIN_LIFE   = 52, // hardcoded constant in CGameHandler
		POISON       = 67,
		DEATH_BLOW   = 73,
		REGENERATION = 74,
		MANA_DRAIN   = 77,

		INVALID      = -1,
	};
}

/// Struct for battle effect animation e.g. morale, prayer, armageddon, bless,...
struct BattleEffect
{
	int x, y; //position on the screen
	float currentFrame;
	std::shared_ptr<CAnimation> animation;
	int effectID; //uniqueID equal ot ID of appropriate CSpellEffectAnim
	BattleHex position; //Indicates if effect which hex the effect is drawn on
};

class BattleEffectsController
{
	BattleInterface * owner;

	/// list of current effects that are being displayed on screen (spells & creature abilities)
	std::vector<BattleEffect> battleEffects;

public:
	BattleEffectsController(BattleInterface * owner);

	void startAction(const BattleAction* action);

	void displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects);

	//displays custom effect on the battlefield
	void displayEffect(EBattleEffect::EBattleEffect effect, const BattleHex & destTile);
	void displayEffect(EBattleEffect::EBattleEffect effect, uint32_t soundID, const BattleHex & destTile);
	//void displayEffects(EBattleEffect::EBattleEffect effect, uint32_t soundID, const std::vector<BattleHex> & destTiles);

	void battleTriggerEffect(const BattleTriggerEffect & bte);

	void collectRenderableObjects(BattleRenderer & renderer);

	friend class CPointEffectAnimation;
};
