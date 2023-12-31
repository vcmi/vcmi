/*
 * QuestInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class CQuest;
class CGObjectInstance;

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	const CQuest * quest;
	const CGObjectInstance * obj; //related object, most likely Seer Hut
	int3 tile;

	QuestInfo()
		: quest(nullptr), obj(nullptr), tile(-1,-1,-1)
	{};
	QuestInfo (const CQuest * Quest, const CGObjectInstance * Obj, int3 Tile) :
		quest (Quest), obj (Obj), tile (Tile){};

	QuestInfo (const QuestInfo &qi) : quest(qi.quest), obj(qi.obj), tile(qi.tile)
	{};

	const QuestInfo& operator= (const QuestInfo &qi)
	{
		quest = qi.quest;
		obj = qi.obj;
		tile = qi.tile;
		return *this;
	}

	bool operator== (const QuestInfo & qi) const
	{
		return (quest == qi.quest && obj == qi.obj);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest;
		h & obj;
		h & tile;
	}
};

VCMI_LIB_NAMESPACE_END
