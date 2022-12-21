/*
 * BattleEffectsController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"
#include "BattleConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleAction;
struct BattleTriggerEffect;

VCMI_LIB_NAMESPACE_END

struct ColorMuxerEffect;
class CAnimation;
class Canvas;
class BattleInterface;
class BattleRenderer;
class PointEffectAnimation;

/// Struct for battle effect animation e.g. morale, prayer, armageddon, bless,...
struct BattleEffect
{
	int x, y; //position on the screen
	float currentFrame;
	std::shared_ptr<CAnimation> animation;
	int effectID; //uniqueID equal ot ID of appropriate CSpellEffectAnim
	BattleHex position; //Indicates if effect which hex the effect is drawn on
};

/// Controls rendering of effects in battle, e.g. from spells, abilities and various other actions like morale
class BattleEffectsController
{
	BattleInterface & owner;

	/// list of current effects that are being displayed on screen (spells & creature abilities)
	std::vector<BattleEffect> battleEffects;

	std::map<std::string, ColorMuxerEffect> colorMuxerEffects;

	void loadColorMuxers();
public:
	const ColorMuxerEffect &getMuxerEffect(const std::string & name);

	BattleEffectsController(BattleInterface & owner);

	void startAction(const BattleAction* action);

	//displays custom effect on the battlefield
	void displayEffect(EBattleEffect effect, const BattleHex & destTile);
	void displayEffect(EBattleEffect effect, uint32_t soundID, const BattleHex & destTile);

	void battleTriggerEffect(const BattleTriggerEffect & bte);

	void collectRenderableObjects(BattleRenderer & renderer);

	friend class PointEffectAnimation;
};
