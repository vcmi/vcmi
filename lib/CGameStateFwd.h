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

