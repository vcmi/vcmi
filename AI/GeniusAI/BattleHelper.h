#ifndef __BATTLE_HELPER__
#define __BATTLE_HELPER__

#include "Common.h"

namespace GeniusAI { namespace BattleAI {

	class CBattleHelper
{
public:
	CBattleHelper();
	~CBattleHelper();

	int GetBattleFieldPosition(int x, int y);
	int DecodeXPosition(int battleFieldPosition);
	int DecodeYPosition(int battleFieldPosition);

	int GetShortestDistance(int pointA, int pointB);
	int GetDistanceWithObstacles(int pointA, int pointB);

	int GetVoteForMaxDamage() const { return m_voteForMaxDamage; }
	int GetVoteForMinDamage() const { return m_voteForMinDamage; }
	int GetVoteForMaxSpeed() const { return m_voteForMaxSpeed; }
	int GetVoteForDistance() const { return m_voteForDistance; }
	int GetVoteForDistanceFromShooters() const { return m_voteForDistanceFromShooters; }
	int GetVoteForHitPoints() const { return m_voteForHitPoints; }

	const int InfiniteDistance;
	const int BattlefieldWidth;
	const int BattlefieldHeight;
private:
	int m_voteForMaxDamage;
	int m_voteForMinDamage;
	int m_voteForMaxSpeed;
	int m_voteForDistance;
	int m_voteForDistanceFromShooters;
	int m_voteForHitPoints;

	CBattleHelper(const CBattleHelper &);
	CBattleHelper &operator=(const CBattleHelper &);
};

}}

#endif/*__BATTLE_HELPER__*/