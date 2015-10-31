#pragma once

/*
 * CGameStateFwd.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CQuest;
class CGObjectInstance;

class DLL_LINKAGE EVictoryLossCheckResult
{
public:
	static EVictoryLossCheckResult victory(std::string toSelf, std::string toOthers)
	{
		return EVictoryLossCheckResult(VICTORY, toSelf, toOthers);
	}

	static EVictoryLossCheckResult defeat(std::string toSelf, std::string toOthers)
	{
		return EVictoryLossCheckResult(DEFEAT, toSelf, toOthers);
	}

	EVictoryLossCheckResult():
	intValue(0)
	{
	}

	bool operator==(EVictoryLossCheckResult const & other) const
	{
		return intValue == other.intValue;
	}

	bool operator!=(EVictoryLossCheckResult const & other) const
	{
		return intValue != other.intValue;
	}

	bool victory() const
	{
		return intValue == VICTORY;
	}
	bool loss() const
	{
		return intValue == DEFEAT;
	}

	EVictoryLossCheckResult invert()
	{
		return EVictoryLossCheckResult(-intValue, messageToOthers, messageToSelf);
	}

	std::string messageToSelf;
	std::string messageToOthers;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & intValue & messageToSelf & messageToOthers;
	}
private:
	enum EResult
	{
		DEFEAT = -1,
		INGAME =  0,
		VICTORY= +1
	};

	EVictoryLossCheckResult(si32 intValue, std::string toSelf, std::string toOthers):
		messageToSelf(toSelf),
		messageToOthers(toOthers),
		intValue(intValue)
	{
	}

	si32 intValue; // uses EResult
};

/*static std::ostream & operator<<(std::ostream & os, const EVictoryLossCheckResult & victoryLossCheckResult)
{
	os << victoryLossCheckResult.messageToSelf;
	return os;
}*/

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	const CQuest * quest;
	const CGObjectInstance * obj; //related object, most likely Seer Hut
	int3 tile;

	QuestInfo(){};
	QuestInfo (const CQuest * Quest, const CGObjectInstance * Obj, int3 Tile) :
		quest (Quest), obj (Obj), tile (Tile){};

	//FIXME: assignment operator should return QuestInfo &
	bool operator= (const QuestInfo &qi)
	{
		quest = qi.quest;
		obj = qi.obj;
		tile = qi.tile;
		return true;
	}

	bool operator== (const QuestInfo & qi) const
	{
		return (quest == qi.quest && obj == qi.obj);
	}

	//std::vector<std::string> > texts //allow additional info for quest log?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest & obj & tile;
	}
};

struct DLL_LINKAGE CGPathNode
{
	enum EAccessibility
	{
		NOT_SET = 0,
		ACCESSIBLE = 1, //tile can be entered and passed
		VISITABLE, //tile can be entered as the last tile in path
		BLOCKVIS,  //visitable from neighbouring tile but not passable
		BLOCKED //tile can't be entered nor visited
	};

	EAccessibility accessible;
	ui8 land;
	ui8 turns; //how many turns we have to wait before reachng the tile - 0 means current turn
	ui32 moveRemains; //remaining tiles after hero reaches the tile
	CGPathNode * theNodeBefore;
	int3 coord; //coordinates

	CGPathNode();
	bool reachable() const;
};

struct DLL_LINKAGE CGPath
{
	std::vector<CGPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
	void convert(ui8 mode); //mode=0 -> from 'manifest' to 'object'
};

struct DLL_LINKAGE CPathsInfo
{
	mutable boost::mutex pathMx;

	const CGHeroInstance *hero;
	int3 hpos;
	int3 sizes;
	CGPathNode ***nodes; //[w][h][level]

	const CGPathNode * getPathInfo( const int3& tile ) const;
	bool getPath(const int3 &dst, CGPath &out) const;
	int getDistance( int3 tile ) const;
	CPathsInfo(const int3 &Sizes);
	~CPathsInfo();
};
