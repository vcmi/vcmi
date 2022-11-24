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

struct SDL_Surface;
class BattleAction;
class CAnimation;
class CBattleInterface;
struct BattleObjectsByHex;
struct CustomEffectInfo;
struct BattleTriggerEffect;
class CEffectAnimation;

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

class CBattleEffectsController
{
	CBattleInterface * owner;

	std::vector<BattleEffect> battleEffects; //different animations to display on the screen like spell effects
public:
	CBattleEffectsController(CBattleInterface * owner);

	void startAction(const BattleAction* action);
	void sortObjectsByHex(BattleObjectsByHex & sorted);

	void displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects);

	void displayEffect(EBattleEffect::EBattleEffect effect, const BattleHex & destTile); //displays custom effect on the battlefield
	void displayEffect(EBattleEffect::EBattleEffect effect, uint32_t soundID, const BattleHex & destTile); //displays custom effect on the battlefield
	void battleTriggerEffect(const BattleTriggerEffect & bte);
	void showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects);


	friend class CEffectAnimation; // currently, battleEffects is largely managed by CEffectAnimation, TODO: move this logic into CBattleEffectsController
};
