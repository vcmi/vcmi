#pragma once

#include "CBattleStackAnimation.h"
#include "SStackAttackedInfo.h"

class CStack;

/*
 * CDefenceAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Animation of a defending unit
class CDefenceAnimation : public CBattleStackAnimation
{
private:
	//std::vector<SSStackAttackedInfo> attackedInfos;
	int dmg; //damage dealt
	int amountKilled; //how many creatures in stack has been killed
	const CStack * attacker; //attacking stack
	bool byShooting; //if true, stack has been attacked by shooting
	bool killed; //if true, stack has been killed
public:
	bool init();
	void nextFrame();
	void endAnim();

	CDefenceAnimation(SStackAttackedInfo _attackedInfo, CBattleInterface * _owner);
};