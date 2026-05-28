/*
 * BattleHero.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleConstants.h"

#include "../gui/CIntObject.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
VCMI_LIB_NAMESPACE_END

class CAnimation;
class BattleInterface;
class BattleRenderer;

/// Hero battle animation
class BattleHero : public CIntObject
{
	bool defender;

	CFunctionList<void()> phaseFinishedCallback;

	std::shared_ptr<CAnimation> animation;
	std::shared_ptr<CAnimation> flagAnimation;

	const CGHeroInstance * hero; //this animation's hero instance
	const BattleInterface & owner; //battle interface to which this animation is assigned

	EHeroAnimType phase; //stage of animation
	EHeroAnimType nextPhase; //stage of animation to be set after current phase is fully displayed

	float currentSpeed;
	float currentFrame; //frame of animation
	float flagCurrentFrame;

	void switchToNextPhase();

	void render(Canvas & canvas); //prints next frame of animation to to
public:
	const CGHeroInstance * instance() const;

	void setPhase(EHeroAnimType newPhase); //sets phase of hero animation

	void collectRenderableObjects(BattleRenderer & renderer);
	void tick(uint32_t msPassed) override;

	float getFrame() const;
	void onPhaseFinished(const std::function<void()> &);

	void pause();
	void play();

	void heroLeftClicked();
	void heroRightClicked() const;

	BattleHero(const BattleInterface & owner, const CGHeroInstance * hero, bool defender);
};
