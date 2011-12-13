#include "StdInc.h"
#include "BattleHelper.h"

using namespace geniusai::BattleAI;
using namespace std;


CBattleHelper::CBattleHelper():
	InfiniteDistance(0xffff),
	BattlefieldWidth(GameConstants::BFIELD_WIDTH-2),
	BattlefieldHeight(GameConstants::BFIELD_HEIGHT),
	m_voteForMaxDamage(10),
	m_voteForMinDamage(10),
	m_voteForMaxSpeed(10),
	m_voteForDistance(10),
	m_voteForDistanceFromShooters(20),
	m_voteForHitPoints(10)
{
	// loads votes
	std::fstream f;
	f.open("AI\\CBattleHelper.txt", std::ios::in);
	if (f)
	{
		//char c_line[512];
		std::string line;
		while (std::getline(f, line, '\n'))
		{
			//f.getline(c_line, sizeof(c_line), '\n');
			//std::string line(c_line);
			//std::getline(f, line);
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::algorithm::is_any_of("="));
			if (parts.size() >= 2)
			{
				boost::algorithm::trim(parts[0]);
				boost::algorithm::trim(parts[1]);
				if (parts[0].compare("m_voteForDistance") == 0)
				{
					try
					{
						m_voteForDistance = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForDistanceFromShooters") == 0)
				{
					try
					{
						m_voteForDistanceFromShooters = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForHitPoints") == 0)
				{
					try
					{
						m_voteForHitPoints = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxDamage") == 0)
				{
					try
					{
						m_voteForMaxDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxSpeed") == 0)
				{
					try
					{
						m_voteForMaxSpeed = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMinDamage") == 0)
				{
					try
					{
						m_voteForMinDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
			}
		}
		f.close();
	}
}

CBattleHelper::~CBattleHelper()
{}


int CBattleHelper::GetBattleFieldPosition(int x, int y)
{
	return x + GameConstants::BFIELD_WIDTH * (y - 1);
}

int CBattleHelper::DecodeXPosition(int battleFieldPosition)
{
	int x = battleFieldPosition%GameConstants::BFIELD_WIDTH;
	assert( x > 0 && x < GameConstants::BFIELD_WIDTH-1 );
	return x;
}

int CBattleHelper::DecodeYPosition(int battleFieldPosition)
{

	int y=battleFieldPosition/GameConstants::BFIELD_WIDTH +1;
	assert( y > 0 && y <= GameConstants::BFIELD_HEIGHT);
	return y;
}

int CBattleHelper::StepRight(int pos){
  return pos+1;
}

int CBattleHelper::StepLeft(int pos){
  return pos-1;
}

int CBattleHelper::StepUpleft(int pos){
  if((pos/GameConstants::BFIELD_WIDTH)%2==0){
    return pos-GameConstants::BFIELD_WIDTH;
  }else{
    return pos-GameConstants::BFIELD_WIDTH-1;
  }
}

int CBattleHelper::StepUpright(int pos){
  if((pos/GameConstants::BFIELD_WIDTH)%2==0){
    return pos-GameConstants::BFIELD_WIDTH+1;
  }else{
    return pos-GameConstants::BFIELD_WIDTH;
  }
}

int CBattleHelper::StepDownleft(int pos){
  if((pos/GameConstants::BFIELD_WIDTH)%2==0){
    return pos+GameConstants::BFIELD_WIDTH;
  }else{
    return pos+GameConstants::BFIELD_WIDTH-1;
  }
}

int CBattleHelper::StepDownright(int pos){
  if((pos/GameConstants::BFIELD_WIDTH)%2==0){
    return pos+GameConstants::BFIELD_WIDTH+1;
 }else{
    return pos+GameConstants::BFIELD_WIDTH;
  }
}

int CBattleHelper::GetShortestDistance(int pointA, int pointB)
{
	int x1 = DecodeXPosition(pointA);
	int y1 = DecodeYPosition(pointA);
	int x2 = DecodeXPosition(pointB);
	int y2 = DecodeYPosition(pointB);
	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);
	if( dy%2==1 ){  
		if( y1%2==1 ){
			if( x2<=x1 ){dx++;}			
		}else{
			if( x2>=x1 ){dx++;}
		}
	}
	max(dy,dx+dy/2);
	
	return 0;
}

int CBattleHelper::GetDistanceWithObstacles(int pointA, int pointB)
{
	
	// TODO - implement this!
	return GetShortestDistance(pointA, pointB);
}
